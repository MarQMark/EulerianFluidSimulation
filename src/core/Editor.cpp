//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "simulation/scene/nodes/BoundingBoxFluidSpawnNode.h"
#include "simulation/scene/nodes/ModelFluidSpawnNode.h"
#include <memory>
#include "simulation/data/Grid3D.h"
#include "simulation/Properties.h"

Editor::Editor() {
    _sdfShader = std::make_unique<Shader>("assets/shaders/sdf.comp", Shader::COMPUTE);
    _baseFluidFillShader = std::make_unique<Shader>("assets/shaders/baseFluidFill.comp", Shader::COMPUTE);
}

void Editor::initSimulation(Grid3D* grid) {
    // --- Init Fluid ---
    initFluid(grid->rho());

    // --- Init Boundaries ---
    initBoundaries(grid->boundary());
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
    // simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    _scene->removeSceneNode(forceFieldNode);
    // simulation->rebuildForceFieldBuffer();
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

void Editor::updateRenderingSize(int width, int height) const {
    for (auto* node : _scene->getSceneNodes())
        node->updateSize(width, height);
}

void Editor::initFluid(ScalField3D& rho) const {
    SimulationDomainNode* simulationDomainNode = _scene->getSimulationDomain();
    SimulationDomainSettings domainSettings = simulationDomainNode->getDomainSettings();

    const int width = rho.width();
    const int height = rho.height();
    const int depth = rho.depth();

    const int gx = (width + 7) / 8;
    const int gy = (height + 7) / 8;
    const int gz = (depth + 7) / 8;

    StorageBuffer rhoBuffer(rho.size() * sizeof(float));
    rhoBuffer.clear();

    // Fluid Spawns Init
    std::vector<Triangle> triangles;

    for (const auto& s : _scene->getFluidSpawns()) {
        const auto& shape = s->getFluidSpawn().shape;
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
        rhoBuffer.bind(1);

        _sdfShader->dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        rhoBuffer.download(rho.data());
    }

    // Percentage Filled Init
    if (domainSettings.percentageFilled != 0.0f) {
        _baseFluidFillShader->bind();

        _baseFluidFillShader->setInt("uFillHeight",  domainSettings.percentageFilled * rho.height());

        _baseFluidFillShader->setInt("uWidth",  rho.width());
        _baseFluidFillShader->setInt("uHeight", rho.height());
        _baseFluidFillShader->setInt("uDepth",  rho.depth());

        auto domain = simulationDomainNode->getWorldBoundingBox();
        _sdfShader->setVec3("uDomainSize", domain.size);
        _sdfShader->setVec3("uDomainCenter", domain.center);
        rhoBuffer.bind(0);

        _sdfShader->dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        rhoBuffer.download(rho.data());
    }
}

void Editor::initBoundaries(ScalField3D& boundary) const {
    SimulationDomainNode* simulationDomainNode = _scene->getSimulationDomain();

    const int width = boundary.width();
    const int height = boundary.height();
    const int depth = boundary.depth();

    const int gx = (width + 7) / 8;
    const int gy = (height + 7) / 8;
    const int gz = (depth + 7) / 8;

    StorageBuffer boundaryBuffer(boundary.size() * sizeof(float));
    boundaryBuffer.clear();

    // Fluid Spawns Init
    std::vector<Triangle> triangles;

    for (const auto& model : _scene->getModels()) {
        if (!model->isBoundary())
            return;

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

// BoundaryBuilder* Editor::getBoundaryBuilder() {
//     return &boundryBuilder;
// }