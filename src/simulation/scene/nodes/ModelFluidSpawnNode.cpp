//
// Created by ruut on 6/11/26.
//

#include "simulation/scene/nodes/ModelFluidSpawnNode.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"

ModelFluidSpawnNode::ModelFluidSpawnNode(const std::string& name, const std::shared_ptr<Model>& model)
    : ModelNode(name, model)
{
    this->name = name;
    this->boundary = false;

    init();
}

ModelFluidSpawnNode::ModelFluidSpawnNode(const nlohmann::json json) : ModelNode(json) {
    init();
}

FluidSpawn ModelFluidSpawnNode::getFluidSpawn() {
    // Get current world space triangles of model
    particleSpawn.shape = this->model->getWorldSpaceTriangles(transform);
    particleSpawn.area = getWorldBoundingBox();
    return particleSpawn;
}

NodeProperties ModelFluidSpawnNode::buildProperties() {
    NodeProperties properties = SceneNode::buildProperties();

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

    return properties;
}

std::string ModelFluidSpawnNode::getTypeName() const {
    return "ModelFluidSpawnNode";
}

void ModelFluidSpawnNode::init() {
    this->model->clearMaterials();

    this->model->addMaterial(Material::createWireframe());
    for (const auto m : this->model->getMeshes())
        m->setMaterialIndex(0);

    particleSpawn.shape = this->model->getWorldSpaceTriangles(glm::mat4(1.0f));
}
