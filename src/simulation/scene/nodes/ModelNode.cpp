//
// Created by ruut on 6/1/26.
//

#include "simulation/scene/nodes/ModelNode.h"
#include "../../../../include/simulation/scene/nodes/BoundingBoxNode.h"
#include "assets/Model.h"

ModelNode::ModelNode(const std::string& name, const std::shared_ptr<Model>& model) {
    this->name = name;
    this->model = model;

    ModelNode::updateTransform();

    // Add bounding box node as child
    auto bbNode = std::make_unique<BoundingBoxNode>("Bounding Box", glm::vec4(1.0f));
    boundingBox = bbNode.get();
    boundingBox->setTarget(this);
    boundingBox->setVisible(false);
    children.push_back(std::move(bbNode));
}

ModelNode::ModelNode(const nlohmann::json json) : SceneNode(json) {
    // Transform
    auto pos = json.value("position", std::vector{position.x, position.y, position.z});
    position = { pos[0], pos[1], pos[2] };

    auto rot = json.value("rotation", std::vector{rotation.x, rotation.y, rotation.z, rotation.w});
    rotation = { rot[3], rot[0], rot[1], rot[2] };

    auto scl = json.value("scale", std::vector{scale.x, scale.y, scale.z});
    scale = { scl[0], scl[1], scl[2] };

    // ModelNode
    boundary = json.value("boundary", boundary);
    model = std::make_shared<Model>(json.value("path", "assets/models/suzanne.obj"));

    // Add bounding box node as child
    auto bbNode = std::make_unique<BoundingBoxNode>("Bounding Box", glm::vec4(1.0f));
    boundingBox = bbNode.get();
    boundingBox->setTarget(this);
    boundingBox->setVisible(false);
    children.push_back(std::move(bbNode));

    ModelNode::updateTransform();
}

Model* ModelNode::getModel() const {
    return model.get();
}

std::vector<Triangle> ModelNode::getWorldSpaceTriangles() const {
    return model->getWorldSpaceTriangles(transform);
}

AABB ModelNode::getBoundingBox() const {
    return model->getBoundingBox();
}

AABB ModelNode::getWorldBoundingBox() const {
    return worldBoundingBox;
}

void ModelNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);
    worldBoundingBox = model->getWorldBoundingBox(transform);
}

void ModelNode::updateTransform() {
    Transform::updateTransform();
    worldBoundingBox = model->getWorldBoundingBox(transform);
}

bool ModelNode::isBoundary() const {
    return boundary;
}

void ModelNode::setBoundary(bool on) {
    this->boundary = on;
}

void ModelNode::update(float deltaTime) { }

void ModelNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    model->draw(view, projection, transform);
}

NodeProperties ModelNode::buildProperties() {
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

    properties.addSection("Boundary");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Enable Boundary",
        [this] () { return boundary; },
        [this] (bool on) { boundary = on; }
    ));

    properties.addSection("Debug");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Show Bounding Box",
        [this] () { return boundingBox->isVisible(); },
        [this] (bool on) { boundingBox->setVisible(on); }
    ));

    return properties;
}

nlohmann::json ModelNode::toJson() const {
    nlohmann::json j = SceneNode::toJson();

    // Transform
    j["position"] = { position.x, position.y, position.z };
    j["rotation"] = { rotation.x, rotation.y, rotation.z, rotation.w };
    j["scale"] = { scale.x, scale.y, scale.z };

    // Model
    j["boundary"] = boundary;
    j["path"] = model->getPath();

    return j;
}

std::string ModelNode::getTypeName() const {
    return "ModelNode";
}
