//
// Created by ruut on 4/29/26.
//

#include "core/Editor.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include "simulation/scene/nodes/ModelParticleSpawnNode.h"
#include <memory>

#include "simulation/Grid3D.h"
#include "simulation/Properties.h"

void Editor::initSimulation(Grid3D* grid) {
    for (int z = 0 ; z < grid->rho().depth() ; z++) {
        for (int y = 0 ; y < grid->rho().height() * 0.5 ; y++) {
            for (int x = 0 ; x < grid->rho().width() ; x++) {
                (grid->rho())(x, y, z) = 1.0f;
            }
        }
    }

    SET_PROPERTY_B(SIMULATION_INITIALIZED, true);
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
        auto bbParticleSpawnNode = std::make_unique<BoundingBoxParticleSpawnNode>(std::string("Particle Spawn"));

        selectedNode = bbParticleSpawnNode.get();
        scene->addSceneNode(std::move(bbParticleSpawnNode));
        return;
    }

    auto model = std::make_shared<Model>(path);
    auto modelParticleSpawnNode = std::make_unique<ModelParticleSpawnNode>(std::string("Particle Spawn"), model);
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