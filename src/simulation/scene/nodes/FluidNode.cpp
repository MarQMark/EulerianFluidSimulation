//
// Created by ruut on 6/28/26.
//

#include "simulation/scene/nodes/FluidNode.h"
#include "renderer/MarchingCubes.h"
#include "simulation/Grid3D.h"
#include "simulation/Properties.h"
#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"

FluidNode::FluidNode() {
    savable = false;

    // --- Model ---
    auto fluidMesh = std::make_unique<Mesh>("FluidMesh", 1, 1);
    _cubes = new MarchingCubes(fluidMesh.get());
    _fluidModel = std::make_shared<Model>("Fluid", std::move(fluidMesh));

    setSceneGraphVisible(false);
    setDeletable(false);

    Material fluidMaterial("Fluid", Shader::createDefaultShaded());
    fluidMaterial.setVec3("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
    _fluidModel->addMaterial(fluidMaterial);
}

FluidNode::~FluidNode() {
    delete _cubes;
}

void FluidNode::setRenderData(const SimulationDisplay3D::RenderData3D* data) {
    _renderData = data;
}

void FluidNode::update(float deltaTime) {
    if (!_renderData)
        return;

    _cubes->updateMesh(
        _renderData->grid->rho(),
        scene->getSimulationDomain()->getWorldBoundingBox());
}

void FluidNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    if (!_renderData)
        return;

    _fluidModel->draw(view, projection, glm::mat4(1.0f));
}

NodeProperties FluidNode::buildProperties() {
    return {};
}
