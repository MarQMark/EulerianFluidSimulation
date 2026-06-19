//
// Created by ruut on 6/1/26.
//

#include "simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include "assets/Material.h"
#include "simulation/scene/NodeProperties.h"

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const std::string& name)
    : BoundingBoxNode(name, glm::vec3(1.0f, 0.0, 0.0))
{
    // Init particle spawn
    particleSpawn.shapeTriangles = {};
}

BoundingBoxParticleSpawnNode::BoundingBoxParticleSpawnNode(const nlohmann::json json) : BoundingBoxNode(json) {
    particleSpawn.shapeTriangles = {};

    // Particle spawn
    particleSpawn.restDensity = json["restDensity"];
    particleSpawn.particleSpacing = json["particleSpacing"];
    particleSpawn.color = glm::vec3(
        json["color"][0].get<float>(),
        json["color"][1].get<float>(),
        json["color"][2].get<float>()
    );

    // Update shader color variable
    material->bind();
    material->setVec3("color", glm::vec3(
        json["color"][0].get<float>(),
        json["color"][1].get<float>(),
        json["color"][2].get<float>()
    ));
}

ParticleSpawn BoundingBoxParticleSpawnNode::getParticleSpawn() {
    // Update bounding box
    particleSpawn.area = getWorldBoundingBox();

    return particleSpawn;
}

NodeProperties BoundingBoxParticleSpawnNode::buildProperties() {
    NodeProperties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Particle Spawn");
    properties.addProperty(std::make_unique<FloatProperty>(
        "Rest Density",
        [this] () { return particleSpawn.restDensity; },
        [this] (float mass) { particleSpawn.restDensity = mass; },
        0.0, INFINITY, 1.0f
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Particle Spacing",
        [this] () { return particleSpawn.particleSpacing; },
        [this] (float spacing) { particleSpawn.particleSpacing = spacing; },
        0.001f, INFINITY, 0.0001f
    ));
    properties.addProperty(std::make_unique<ColorProperty>(
        "Particle Color",
        [this] () { return glm::vec3(particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z); },
        [this] (glm::vec3 color)
        {
            particleSpawn.color = { color.x, color.y, color.z };

            material->bind();
            material->setVec3("color", color);
        }
    ));

    return properties;
}

nlohmann::json BoundingBoxParticleSpawnNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["restDensity"] = particleSpawn.restDensity;
    j["particleSpacing"] = particleSpawn.particleSpacing;
    j["color"] = { particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z };

    return j;
}

std::string BoundingBoxParticleSpawnNode::getTypeName() const {
    return "BoundingBoxParticleSpawnNode";
}
