//
// Created by ruut on 6/1/26.
//

#include "simulation/scene/nodes/BoundingBoxFluidSpawnNode.h"
#include "assets/Material.h"
#include "simulation/Properties.h"
#include "simulation/scene/NodeProperties.h"

BoundingBoxFluidSpawnNode::BoundingBoxFluidSpawnNode(const std::string& name)
    : BoundingBoxNode(name, glm::vec3(1.0f, 0.0, 0.0))
{
    // Init particle spawn
    _mesh = Mesh::createBox("Box");
    _fluidSpawn.shape = _mesh->getWorldSpaceTriangles(glm::mat4(1.0f));
}

BoundingBoxFluidSpawnNode::BoundingBoxFluidSpawnNode(const nlohmann::json json) : BoundingBoxNode(json) {
    _mesh = Mesh::createBox("Box");
    _fluidSpawn.shape = _mesh->getWorldSpaceTriangles(glm::mat4(1.0f));
}

FluidSpawn BoundingBoxFluidSpawnNode::getFluidSpawn() {
    // Update bounding box
    _fluidSpawn.shape = _mesh->getWorldSpaceTriangles(transform);
    _fluidSpawn.area = getWorldBoundingBox();

    return _fluidSpawn;
}

NodeProperties BoundingBoxFluidSpawnNode::buildProperties() {
    NodeProperties properties = BoundingBoxNode::buildProperties();
    return properties;
}

nlohmann::json BoundingBoxFluidSpawnNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    // j["restDensity"] = fluidSpawn.restDensity;
    // j["particleSpacing"] = fluidSpawn.particleSpacing;
    // j["color"] = { fluidSpawn.color.x, fluidSpawn.color.y, fluidSpawn.color.z };

    return j;
}

std::string BoundingBoxFluidSpawnNode::getTypeName() const {
    return "BoundingBoxFluidSpawnNode";
}
