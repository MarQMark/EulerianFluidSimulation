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
    auto pos = json.value("position", std::vector{position.x, position.y, position.z});
    position = { pos[0], pos[1], pos[2] };

    auto scl = json.value("scale", std::vector{scale.x, scale.y, scale.z});
    scale = { scl[0], scl[1], scl[2] };

    Transform::updateTransform();

    mesh = Mesh::createWireframeBox(name);
    material = std::make_unique<Material>("Wireframe", Shader::createBox());

    auto clr = json.value("color", std::vector{color.x, color.y, color.z});
    color = { clr[0], clr[1], clr[2] };
    material->setVec3("color", { clr[0], clr[1], clr[2] });
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
    NodeProperties properties = SceneNode::buildProperties();

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
