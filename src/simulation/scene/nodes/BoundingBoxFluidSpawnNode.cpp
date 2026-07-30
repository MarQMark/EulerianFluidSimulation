//
// Created by ruut on 6/1/26.
//

#include "simulation/scene/nodes/BoundingBoxFluidSpawnNode.h"
#include "assets/Material.h"
#include "simulation/Properties.h"
#include "simulation/scene/NodeProperties.h"

BoundingBoxFluidSpawnNode::BoundingBoxFluidSpawnNode(const std::string& name)
    : SimSpaceNode(name, glm::vec3(1.0f, 0.0, 0.0))
{
    // Init particle spawn
    _mesh = Mesh::createCube("Box");
    _fluidSpawn.shape = _mesh->getWorldSpaceTriangles(glm::mat4(1.0f));
}

BoundingBoxFluidSpawnNode::BoundingBoxFluidSpawnNode(const nlohmann::json json) : SimSpaceNode(json) {
    _mesh = Mesh::createCube("Box");
    _fluidSpawn.shape = _mesh->getWorldSpaceTriangles(glm::mat4(1.0f));
}

FluidSpawn BoundingBoxFluidSpawnNode::getFluidSpawn() {
    // Update bounding box
    //_fluidSpawn.shape = _mesh->getWorldSpaceTriangles(transform);
    _fluidSpawn.area.size = scale;
    _fluidSpawn.area.center = position;

    return _fluidSpawn;
}

NodeProperties BoundingBoxFluidSpawnNode::buildProperties() {
    NodeProperties properties = SimSpaceNode::buildProperties();
    return properties;
}

nlohmann::json BoundingBoxFluidSpawnNode::toJson() const {
    return BoundingBoxNode::toJson();
}

std::string BoundingBoxFluidSpawnNode::getTypeName() const {
    return "BoundingBoxFluidSpawnNode";
}