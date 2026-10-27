//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "simulation/scene/nodes/BoundingBoxFluidSpawnNode.h"
#include "simulation/scene/nodes/ModelFluidSpawnNode.h"
#include <memory>
#include "simulation/data/Grid3D.h"
#include "simulation/Properties.h"
#include "simulation/data/OpenGLGrid3D.h"
#include "simulation/scene/nodes/DeletionNode.h"
#include "simulation/scene/nodes/QueryNode.h"

Editor::Editor() {
    _sdfShader = std::make_unique<Shader>("assets/shaders/sdf.comp", Shader::COMPUTE);
    _baseFluidFillShader = std::make_unique<Shader>("assets/shaders/baseFluidFill.comp", Shader::COMPUTE);
    _forceFieldCellsShader = std::make_unique<Shader>("assets/shaders/forcefield_cells.comp", Shader::COMPUTE);
}

void Editor::init(IGrid3D* grid) {
    // --- Init Fluid ---
    StorageBuffer* gpuRho = nullptr;
    if (const auto* openGLGrid = dynamic_cast<OpenGLGrid3D*>(grid))
        gpuRho = &openGLGrid->gpuRho();
    initFluid(grid->rho(), gpuRho);

    // --- Init Boundaries ---
    initBoundaries(grid->boundary(), grid->rho());

    // --- Clear Force Field Data ---
    for (auto ff : _scene->getForceFields())
        ff->resetCells();

    // Call init function of each node
    for (const auto& node : _scene->getSceneNodes())
        node->init();
}

void Editor::setScene(Scene* scene) {
    this->_scene = scene;
}

Scene* Editor::getScene() const {
    return _scene;
}

void Editor::addForceField() {
    auto forceFieldNode = std::make_unique<ForceFieldNode>(std::string("Force Field"));
    _selectedNode = forceFieldNode.get();
    _scene->addSceneNode(std::move(forceFieldNode));
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    _scene->removeSceneNode(forceFieldNode);
}

void Editor::addParticleSpawn(const std::string& path) {
    if (path.empty()) {
        auto bbParticleSpawnNode = std::make_unique<BoundingBoxFluidSpawnNode>(std::string("Fluid Spawn"));

        _selectedNode = bbParticleSpawnNode.get();
        _scene->addSceneNode(std::move(bbParticleSpawnNode));
        return;
    }

    auto model = std::make_shared<Model>(path);
    auto modelParticleSpawnNode = std::make_unique<ModelFluidSpawnNode>(std::string("Fluid Spawn"), model);
    _selectedNode = modelParticleSpawnNode.get();
    _scene->addSceneNode(std::move(modelParticleSpawnNode));
}

void Editor::addQuery() {
    auto queryNode = std::make_unique<QueryNode>();
    _selectedNode = queryNode.get();
    _scene->addSceneNode(std::move(queryNode));
}

void Editor::addDeletionArea() {
    auto deletionNode = std::make_unique<DeletionNode>();
    _selectedNode = deletionNode.get();
    _scene->addSceneNode(std::move(deletionNode));
}

void Editor::updateRenderingSize(int width, int height) const {
    for (auto* node : _scene->getSceneNodes())
        node->updateSize(width, height);
}

void Editor::initFluid(ScalField3D& rho, StorageBuffer* gpuRho) const {
    SimulationDomainNode* simulationDomainNode = _scene->getSimulationDomain();
    SimulationDomainSettings domainSettings = simulationDomainNode->getDomainSettings();

    const int width = rho.width();
    const int height = rho.height();
    const int depth = rho.depth();

    const int gx = (width + 7) / 8;
    const int gy = (height + 7) / 8;
    const int gz = (depth + 7) / 8;

    StorageBuffer* rhoBuffer = nullptr;

    if (gpuRho)
        rhoBuffer = gpuRho;
    else
        rhoBuffer = new StorageBuffer(rho.size() * sizeof(float));
    rhoBuffer->clear();

    // Fluid Spawns Init
    std::vector<Triangle> triangles;
    std::vector<AABB> spNodes;

    for (const auto& s : _scene->getFluidSpawns()) {
        if (const auto spNode = dynamic_cast<SimSpaceNode*>(s)) {
            spNodes.push_back(s->getFluidSpawn().area);
        }
        else {
            const auto& shape = s->getFluidSpawn().shape;
            triangles.insert(
                triangles.end(),
                shape.begin(),
                shape.end()
            );
        }
    }

    if (!triangles.empty()) {
        StorageBuffer triangleBuffer(triangles.size() * sizeof(Triangle));
        triangleBuffer.upload(triangles.data());

        _sdfShader->bind();
        _sdfShader->setInt("uWidth",  rho.width());
        _sdfShader->setInt("uHeight", rho.height());
        _sdfShader->setInt("uDepth",  rho.depth());

        _sdfShader->setInt("uTriangleCount", triangles.size());

        _sdfShader->setFloat("uValueInside", 1.0f);
        _sdfShader->setFloat("uValueOutside", 0.0f);

        auto domain = simulationDomainNode->getWorldBoundingBox();
        _sdfShader->setVec3("uDomainSize", domain.size);
        _sdfShader->setVec3("uDomainCenter", domain.center);

        triangleBuffer.bind(0);
        rhoBuffer->bind(1);

        _sdfShader->dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        rhoBuffer->download(rho.data());
    }

    // Percentage Filled Init
    if (domainSettings.percentageFilled != 0.0f) {
        _baseFluidFillShader->bind();

        _baseFluidFillShader->setInt("uFillHeight",  domainSettings.percentageFilled * rho.height());

        _baseFluidFillShader->setInt("uWidth",  rho.width());
        _baseFluidFillShader->setInt("uHeight", rho.height());
        _baseFluidFillShader->setInt("uDepth",  rho.depth());

        auto domain = simulationDomainNode->getWorldBoundingBox();
        _baseFluidFillShader->setVec3("uDomainSize", domain.size);
        _baseFluidFillShader->setVec3("uDomainCenter", domain.center);
        rhoBuffer->bind(0);

        _baseFluidFillShader->dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        rhoBuffer->download(rho.data());
    }

    if (!gpuRho) delete rhoBuffer;

    for (const auto [position, size] : spNodes) {
        for (int z = (int)position.z; z < (int)(position.z + size.z); z++)
        for (int y = (int)position.y; y < (int)(position.y + size.y); y++)
        for (int x = (int)position.x; x < (int)(position.x + size.x); x++) {
            rho(x, y, z) = 1.f;
        }
    }
    if (gpuRho) gpuRho->upload(rho.data());

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    for (unsigned int i = 0; i < rho.size(); i++) {
        ADD_PROPERTY_F(TOTAL_RHO, rho[i]);
    }
}

void Editor::initBoundaries(ScalField3D& boundary, ScalField3D& rho) const {
    SimulationDomainNode* simulationDomainNode = _scene->getSimulationDomain();

    const int width = boundary.width();
    const int height = boundary.height();
    const int depth = boundary.depth();

    const int gx = (width + 7) / 8;
    const int gy = (height + 7) / 8;
    const int gz = (depth + 7) / 8;

    StorageBuffer boundaryBuffer(boundary.size() * sizeof(float));
    boundaryBuffer.clear();
    boundary.clear();

    // Fluid Spawns Init
    std::vector<Triangle> triangles;

    for (const auto& model : _scene->getModels()) {
        if (!model->isBoundary())
            continue;

        const auto& shape = model->getWorldSpaceTriangles();
        triangles.insert(
            triangles.end(),
            shape.begin(),
            shape.end()
        );
    }

    if (!triangles.empty()) {
        StorageBuffer triangleBuffer(triangles.size() * sizeof(Triangle));
        triangleBuffer.upload(triangles.data());

        _sdfShader->bind();
        _sdfShader->setInt("uWidth",  boundary.width());
        _sdfShader->setInt("uHeight", boundary.height());
        _sdfShader->setInt("uDepth",  boundary.depth());

        _sdfShader->setInt("uTriangleCount", triangles.size());

        _sdfShader->setFloat("uValueInside", 1.0f);
        _sdfShader->setFloat("uValueOutside", 0.0f);

        auto domain = simulationDomainNode->getWorldBoundingBox();
        _sdfShader->setVec3("uDomainSize", domain.size);
        _sdfShader->setVec3("uDomainCenter", domain.center);

        triangleBuffer.bind(0);
        boundaryBuffer.bind(1);

        _sdfShader->dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        boundaryBuffer.download(boundary.data());
    }

    for (int z = 0; z < boundary.depth()  - 0; z++)
    for (int y = 0; y < boundary.height() - 0; y++)
    for (int x = 0; x < boundary.width()  - 0; x++) {
        if (z == 0) boundary(x, y, z) = 1.f;
        if (y == 0) boundary(x, y, z) = 1.f;
        if (x == 0) boundary(x, y, z) = 1.f;
        if (x == boundary.width()  - 1) boundary(x, y, z) = 1.f;
        if (y == boundary.height() - 1) boundary(x, y, z) = 1.f;
        if (z == boundary.depth()  - 1) boundary(x, y, z) = 1.f;
        if (boundary(x, y, z) == 1) rho(x, y, z) = 0;
    }
}

void Editor::updateForceFields(VecField3D& forces) const {
    bool clearForces = false;

    // Check if any forcefield is dirty e.g. was moved or transformed
    for (auto& forceFieldNode : _scene->getForceFields()) {
        if (forceFieldNode->updatePending()) {
            // Note: As only a single force field might change it is fine to define the variables inside the loop.
            const int width = forces.width();
            const int height = forces.height();
            const int depth = forces.depth();

            const int gx = (width + 7) / 8;
            const int gy = (height + 7) / 8;
            const int gz = (depth + 7) / 8;

            std::vector<Triangle> triangles = forceFieldNode->getWorldSpaceTriangles();

            StorageBuffer triangleBuffer(triangles.size() * sizeof(Triangle));
            triangleBuffer.upload(triangles.data());

            StorageBuffer affectedCellsBuffer(PROPERTY_I(MAX_FORCEFIELD_CELLS) * sizeof(int));
            StorageBuffer affectedCellCount(sizeof(int));

            _forceFieldCellsShader->bind();
            _forceFieldCellsShader->setInt("uWidth",  width);
            _forceFieldCellsShader->setInt("uHeight", height);
            _forceFieldCellsShader->setInt("uDepth",  depth);

            _forceFieldCellsShader->setInt("uTriangleCount", triangles.size());

            auto domain =  _scene->getSimulationDomain()->getWorldBoundingBox();
            _forceFieldCellsShader->setVec3("uDomainSize", domain.size);
            _forceFieldCellsShader->setVec3("uDomainCenter", domain.center);

            triangleBuffer.bind(0);
            affectedCellsBuffer.bind(1);
            affectedCellCount.bind(2);

            _forceFieldCellsShader->dispatch(gx, gy, gz);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            int cellCount;
            affectedCellCount.download(&cellCount);

            if (cellCount > PROPERTY_I(MAX_FORCEFIELD_CELLS)) {
                LOG_WARN("ForceField exceeded maximum allowed cells!")
                continue;
            }

            if (cellCount > 0) {
                std::vector<int> cells(cellCount);
                affectedCellsBuffer.download(cells.data(), cellCount * sizeof(int));
                forceFieldNode->setCells(cells);
            }

            clearForces = true;
            break;
        }
    }

    if (clearForces)
        forces.clear();

    for (const auto& forceFieldNode : _scene->getForceFields()) {
        const auto& forceField = forceFieldNode->getForceField();

        if (!forceField.active)
            continue;

        for (const auto c : forceField.cells)
            forces[c] = forceField.direction * forceField.strength;
    }
}

void Editor::addBoundary(const std::string& path) {
    auto model = std::make_shared<Model>(path);
    auto modelNode = std::make_unique<ModelNode>(model->getName(), model);

    _selectedNode = modelNode.get();
    _scene->addSceneNode(std::move(modelNode));
}

void Editor::removeBoundary(const ModelNode* boundaryNode) const {
    _scene->removeSceneNode(boundaryNode);
}

void Editor::setSelected(SceneNode* node) {
    _selectedNode = node;
}

SceneNode* Editor::getSelected() const {
    return _selectedNode;
}

void Editor::removeSceneNode(SceneNode* node) const {
    _scene->removeSceneNode(node);
}