//
// Created by ruut on 5/4/26.
//

#include "simulation/scene/nodes/BoundingBoxNode.h"
#include "assets/Material.h"
#include "assets/Mesh.h"
#include "assets/Shader.h"
#include "simulation/scene/NodeProperties.h"
#include "simulation/scene/Scene.h"

BoundingBoxNode::BoundingBoxNode(const std::string& name, glm::vec3 color) {
    this->name = name;
    this->color = color;

    mesh = Mesh::createWireframeBox(name);
    material = std::make_unique<Material>("Wireframe", Shader::createBox());
    material->setVec3("color", color);
}

BoundingBoxNode::BoundingBoxNode(const nlohmann::json json) : SceneNode(json) {
    // Transform
    position = {
        json["position"][0].get<float>(),
        json["position"][1].get<float>(),
        json["position"][2].get<float>()
    };
    scale = {
        json["scale"][0].get<float>(),
        json["scale"][1].get<float>(),
        json["scale"][2].get<float>()
    };

    Transform::updateTransform();

    mesh = Mesh::createWireframeBox(name);
    material = std::make_unique<Material>("Wireframe", Shader::createBox());
    material->setVec3("color", { json["color"][0], json["color"][1], json["color"][2] });
}

void BoundingBoxNode::update(float deltaTime) { }

void BoundingBoxNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    // Setup wireframe shader
    material->bind();
    material->getShader()->setMat4("view", view);
    material->getShader()->setMat4("projection", projection);

    material->getShader()->setMat4("model", target ? target->getWorldBoundingBox().getTransformMatrix() : transform );

    mesh->draw();
}

void BoundingBoxNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);

    // Force rotation to zeros
    setRotation({0.0, 0.0, 0.0, 0.0});
}

void BoundingBoxNode::setTransform(const AABB& boundingBox) {
    position = boundingBox.center;
    rotation = {0.0, 0.0, 0.0, 0.0};
    scale = boundingBox.size;

    updateTransform();
}

void BoundingBoxNode::setTarget(IBounded* target) {
    this->target = target;
}

void BoundingBoxNode::clearTarget() {
    this->target = nullptr;
}

AABB BoundingBoxNode::getWorldBoundingBox() const {
    return { position, scale };
}

NodeProperties BoundingBoxNode::buildProperties() {
    NodeProperties properties;

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<Vec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));
    properties.addProperty(std::make_unique<Vec3Property>(
        "Scale",
        [this] () { return getScale(); },
        [this] (glm::vec3 scale) { setScale(scale); }
    ));

    return properties;
}

nlohmann::json BoundingBoxNode::toJson() const {
    nlohmann::json j = SceneNode::toJson();

    j["position"] = { position.x, position.y, position.z };
    j["scale"] = { scale.x, scale.y, scale.z };
    j["color"] = { color.x, color.y, color.z };

    return j;
}

std::string BoundingBoxNode::getTypeName() const {
    return "BoundingBoxNode";
}
