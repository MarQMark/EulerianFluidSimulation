//
// Created by ruut on 6/28/26.
//

#include "simulation/scene/nodes/FluidNode.h"
#include "renderer/MarchingCubes.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"

FluidNode::FluidNode() {
    // --- Model ---
    auto fluidMesh = std::make_unique<Mesh>("FluidMesh", 0, 0);
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
    ScalField3D scalarField(100, 100, 100);

    for (int z = 0; z < 100; z++) {
        for (int y = 0; y < 20; y++) {
            for (int x = 0; x < 100; x++) {
                scalarField(x, y, z) = 1.0f;
            }
        }
    }

    AABB domain = scene->getSimulationDomain()->getWorldBoundingBox();

    _cubes->generateMesh(scalarField, domain);
}

void FluidNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    _fluidModel->draw(view, projection, glm::mat4(1.0f));
}

NodeProperties FluidNode::buildProperties() {
    return {};
}
