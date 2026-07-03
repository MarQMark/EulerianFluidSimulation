//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "simulation/scene/nodes/BoundingBoxFluidSpawnNode.h"
#include "simulation/scene/nodes/ModelFluidSpawnNode.h"
#include <memory>
#include "simulation/Grid3D.h"
#include "simulation/Properties.h"

void Editor::initSimulation(Grid3D* grid) {
    // --- Init Fluid ---
    Shader initFluid({{ "assets/shaders/fluid_init.comp", Shader::COMPUTE }});
    StorageBuffer rhoBuffer(grid->rho().size() * sizeof(float));
    rhoBuffer.clear();

    std::vector<Triangle> triangles;

    for (const auto& s : scene->getFluidSpawns()) {
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

        initFluid.bind();
        initFluid.setInt("uWidth",  grid->rho().width());
        initFluid.setInt("uHeight", grid->rho().height());
        initFluid.setInt("uDepth",  grid->rho().depth());

        initFluid.setInt("uTriangleCount", triangles.size());

        auto domain = scene->getSimulationDomain()->getWorldBoundingBox();
        initFluid.setVec3("uDomainSize", domain.size);
        initFluid.setVec3("uDomainCenter", domain.center);

        triangleBuffer.bind(0);
        rhoBuffer.bind(1);

        const int gx = (grid->rho().width() + 7) / 8;
        const int gy = (grid->rho().height() + 7) / 8;
        const int gz = (grid->rho().depth() + 7) / 8;
        initFluid.dispatch(gx, gy, gz);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        rhoBuffer.download(grid->rho().data());
    }

    // --- Init Boundaries ---
    // TODO - Boundaries
}

void Editor::setScene(Scene* scene) {
    this->scene = scene;
}

Scene* Editor::getScene() const {
    return scene;
}

void Editor::addForceField() {
    auto forceFieldNode = std::make_unique<ForceFieldNode>(std::string("Force Field"));

    selectedNode = forceFieldNode.get();
    scene->addSceneNode(std::move(forceFieldNode));
    // simulation->rebuildForceFieldBuffer();
}

void Editor::removeForceField(const ForceFieldNode* forceFieldNode) const {
    scene->removeSceneNode(forceFieldNode);
    // simulation->rebuildForceFieldBuffer();
}

void Editor::addParticleSpawn(const std::string& path) {
    if (path.empty()) {
        auto bbParticleSpawnNode = std::make_unique<BoundingBoxFluidSpawnNode>(std::string("Fluid Spawn"));

        selectedNode = bbParticleSpawnNode.get();
        scene->addSceneNode(std::move(bbParticleSpawnNode));
        return;
    }

    auto model = std::make_shared<Model>(path);
    auto modelParticleSpawnNode = std::make_unique<ModelFluidSpawnNode>(std::string("Fluid Spawn"), model);
    selectedNode = modelParticleSpawnNode.get();
    scene->addSceneNode(std::move(modelParticleSpawnNode));
}

void Editor::updateRenderingSize(int width, int height) const {
    for (auto* node : scene->getSceneNodes())
        node->updateSize(width, height);
}

void Editor::addBoundary(const std::string& path) {
    auto model = std::make_shared<Model>(path);
    auto modelNode = std::make_unique<ModelNode>(model->getName(), model);

    selectedNode = modelNode.get();
    scene->addSceneNode(std::move(modelNode));
}

void Editor::removeBoundary(const ModelNode* boundaryNode) const {
    scene->removeSceneNode(boundaryNode);
}

void Editor::setSelected(SceneNode* node) {
    selectedNode = node;
}

SceneNode* Editor::getSelected() const {
    return selectedNode;
}

void Editor::removeSceneNode(SceneNode* node) const {
    scene->removeSceneNode(node);
}

// BoundaryBuilder* Editor::getBoundaryBuilder() {
//     return &boundryBuilder;
// }