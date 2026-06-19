//
// Created by ruut on 6/11/26.
//

#include "simulation/scene/nodes/ModelParticleSpawnNode.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"

ModelParticleSpawnNode::ModelParticleSpawnNode(const std::string& name, const std::shared_ptr<Model>& model)
    : ModelNode(name, model)
{
    this->name = name;
    this->boundary = false;

    init();
}

ModelParticleSpawnNode::ModelParticleSpawnNode(const nlohmann::json json) : ModelNode(json) {
    // Particle spawn
    particleSpawn.restDensity = json["restDensity"];
    particleSpawn.particleSpacing = json["particleSpacing"];
    particleSpawn.color = glm::vec3(
        json["color"][0].get<float>(),
        json["color"][1].get<float>(),
        json["color"][2].get<float>()
    );

    init();
}

ParticleSpawn ModelParticleSpawnNode::getParticleSpawn() {
    // Get current world space triangles of model
    particleSpawn.shapeTriangles = this->model->getWorldSpaceTriangles(transform);
    particleSpawn.area = getWorldBoundingBox();
    return particleSpawn;
}

NodeProperties ModelParticleSpawnNode::buildProperties() {
    NodeProperties properties;

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<Vec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Rotation",
        [this] () { return getEulerRotation(); },
        [this] (glm::vec3 rotation) { setEulerRotation(rotation); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Scale",
        [this] () { return getScale(); },
        [this] (glm::vec3 scale) { setScale(scale); }
    ));

    properties.addSection("Debug");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Show Bounding Box",
        [this] () { return boundingBox->isVisible(); },
        [this] (bool on) { boundingBox->setVisible(on); }
    ));

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
            particleSpawn.color = glm::vec3(color.x, color.y, color.z);

            model->getMaterial(0)->bind();
            model->getMaterial(0)->setVec3("albedo", color);
        }
    ));

    return properties;
}

nlohmann::json ModelParticleSpawnNode::toJson() const {
    nlohmann::json j = ModelNode::toJson();

    j["restDensity"] = particleSpawn.restDensity;
    j["particleSpacing"] = particleSpawn.particleSpacing;
    j["color"] = { particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z };

    return j;
}

std::string ModelParticleSpawnNode::getTypeName() const {
    return "ModelParticleSpawnNode";
}

void ModelParticleSpawnNode::init() {
    this->model->clearMaterials();

    // Add unshaded material
    Material wireframe("Wireframe", Shader::createDefaultUnshaded());
    wireframe.setFillMode(FillMode::Wireframe);
    wireframe.setCullFaces(false);
    wireframe.setVec3("albedo", glm::vec3(particleSpawn.color.x, particleSpawn.color.y, particleSpawn.color.z));

    this->model->addMaterial(wireframe);
    for (const auto m : this->model->getMeshes())
        m->setMaterialIndex(0);

    particleSpawn.shapeTriangles = this->model->getWorldSpaceTriangles(glm::mat4(1.0f));
}
