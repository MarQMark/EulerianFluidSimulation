//
// Created by ruut on 5/4/26.
//

#include "simulation/scene/nodes/ForceFieldNode.h"
#include "assets/Model.h"

ForceFieldNode::ForceFieldNode(const std::string& name) {
    this->name = name;

    // Setup Force Field Arrow Model
    _model = std::make_unique<Model>("assets/models/forceField.obj", false);

    // Setup Wireframe Box Model
    _wireframeModel = std::make_unique<Model>("Wireframe Model", Mesh::createWireframeCube("Wireframe Mesh"));

    Material wireframeMat("Wireframe", Shader::createBox());
    wireframeMat.setVec3("color", glm::vec3(0.2f, 0.9f, 0.2f));
    _wireframeModel->addMaterial(wireframeMat);

    ForceFieldNode::setScale({ 0.25, 0.25, 0.25 });
}

ForceFieldNode::ForceFieldNode(const nlohmann::json json) : SceneNode(json) {
    // Transform
    auto pos = json.value("position", std::vector{position.x, position.y, position.z});
    position = { pos[0], pos[1], pos[2] };

    auto rot = json.value("rotation", std::vector{rotation.x, rotation.y, rotation.z, rotation.w});
    rotation = { rot[3], rot[0], rot[1], rot[2] };

    auto scl = json.value("scale", std::vector{scale.x, scale.y, scale.z});
    scale = { scl[0], scl[1], scl[2] };

    ForceFieldNode::updateTransform();

    // Forcefield
    forceField.active = json.value("active", forceField.active);
    forceField.strength = json.value("strength", forceField.strength);

    _model = std::make_unique<Model>("assets/models/forceField.obj", false);

    // Setup Wireframe Box Model
    _wireframeModel = std::make_unique<Model>("Wireframe Model", Mesh::createWireframeCube("Wireframe Mesh"));

    Material wireframeMat("Wireframe", Shader::createBox());
    wireframeMat.setVec3("color", glm::vec3(0.2f, 0.9f, 0.2f));
    _wireframeModel->addMaterial(wireframeMat);
}

ForceField ForceFieldNode::getForceField() const {
    return forceField;
}

bool ForceFieldNode::updatePending() const {
    return pendingUpdate;
}

void ForceFieldNode::resetCells() {
    forceField.cells = {};
    pendingUpdate = true;
}

void ForceFieldNode::setCells(const std::vector<int>& cells) {
    forceField.cells = cells;
    pendingUpdate = false;
}

void ForceFieldNode::update(float deltaTime) { }

void ForceFieldNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    _wireframeModel->draw(view, projection, transform);

    // Draw arrow of force field with removed scaling transform
    glm::mat4 result = transform;
    result[0] = glm::normalize(result[0]);
    result[1] = glm::normalize(result[1]);
    result[2] = glm::normalize(result[2]);

    _model->draw(view, projection, result);
}

void ForceFieldNode::setTransform(glm::mat4 transform) {
    Transform::setTransform(transform);
    forceField.direction = glm::normalize(glm::vec3(transform[0]));
    pendingUpdate = true;
}

void ForceFieldNode::updateTransform() {
    Transform::updateTransform();
    forceField.direction = glm::normalize(glm::vec3(transform[0]));
    pendingUpdate = true;
}

std::vector<Triangle> ForceFieldNode::getWorldSpaceTriangles() const {
    auto mesh = Mesh::createCube("tmp");
    return mesh->getWorldSpaceTriangles(transform);
}

NodeProperties ForceFieldNode::buildProperties() {
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

    properties.addSection("Force Field");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Enabled",
        [this] () { return forceField.active; },
        [this] (bool on) { forceField.active = on; pendingUpdate = true; }
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Strength",
        [this] () { return forceField.strength; },
        [this] (float s) { forceField.strength = s; pendingUpdate = true; },
        -INFINITY, INFINITY, 0.01f
    ));

    return properties;
}

nlohmann::json ForceFieldNode::toJson() const {
    nlohmann::json j = SceneNode::toJson();

    // Transform
    j["position"] = { position.x, position.y, position.z };
    j["rotation"] = { rotation.x, rotation.y, rotation.z, rotation.w };
    j["scale"] = { scale.x, scale.y, scale.z };

    // Forcefield
    j["active"] = forceField.active;
    j["strength"] = forceField.strength;

    return j;
}

std::string ForceFieldNode::getTypeName() const {
    return "ForceFieldNode";
}