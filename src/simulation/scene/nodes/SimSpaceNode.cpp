#include "simulation/scene/nodes/SimSpaceNode.h"

#include "simulation/Properties.h"
#include "simulation/scene/Scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

SimSpaceNode::SimSpaceNode(const std::string& name, glm::vec3 color)
    : BoundingBoxNode(name, color) {
}

SimSpaceNode::SimSpaceNode(const nlohmann::json json) : BoundingBoxNode(json) {
}

void SimSpaceNode::update(float deltaTime) {
    const int w = PROPERTY_I(WIDTH_3D);
    const int h = PROPERTY_I(HEIGHT_3D);
    const int d = PROPERTY_I(DEPTH_3D);
    const auto dim = glm::vec3(w, h, d);

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);
}

void SimSpaceNode::draw(const glm::mat4 &view, const glm::mat4 &projection) const {
    material->bind();
    material->getShader()->setMat4("view", view);
    material->getShader()->setMat4("projection", projection);

    const auto mat = scene->getSimulationDomain()->getTransform() * voxel_transform();
    material->getShader()->setMat4("model", mat);

    mesh->draw();
}

void SimSpaceNode::setTransform(const AABB &boundingBox) {}

void SimSpaceNode::setTransform(glm::mat4 transform) {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    transform = glm::translate(glm::mat4(1.f), -translation_offset()) * transform;
    Transform::transform = transform;

    glm::vec3 skew;
    glm::vec4 perspective;
    glm::vec3 s;
    glm::vec3 p;

    glm::decompose(
        transform,
        scale,
        rotation,
        p,
        skew,
        perspective);

    auto inv = inv_voxel_transform();
    position = inv * glm::vec4(p, 1.f);

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);
}

glm::mat4 SimSpaceNode::getTransform() const {
    return glm::translate(glm::mat4(1.f), translation_offset()) * transform;
}

void SimSpaceNode::updateTransform() {
    const glm::vec3 dim(
        PROPERTY_I(WIDTH_3D),
        PROPERTY_I(HEIGHT_3D),
        PROPERTY_I(DEPTH_3D));

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);

    glm::mat4 M(1.0f);

    M = glm::scale(M, 2.f / dim);
    M = glm::translate(M, position);
    M[0][0] = scale.x;
    M[1][1] = scale.y;
    M[2][2] = scale.z;
    transform = M;
}

void SimSpaceNode::setSimulationData(void *data) {
    _sim_data = data;
}

NodeProperties SimSpaceNode::buildProperties() {
    NodeProperties properties = SceneNode::buildProperties();

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<IVec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));
    properties.addProperty(std::make_unique<IVec3Property>(
        "Scale",
        [this] () { return getScale(); },
        [this] (glm::vec3 scale) { setScale(scale); }
    ));

    return properties;
}

glm::mat4 SimSpaceNode::voxel_transform() const {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    glm::mat4 M(1.0f);

    M = glm::scale(M, 1.f / dim);
    M = glm::translate(M, -dim * .5f + (glm::vec3(.5) * scale));
    M = glm::translate(M, position);
    M = glm::scale(M, scale);

    return M;
}

glm::mat4 SimSpaceNode::inv_voxel_transform() const {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    glm::mat4 M(1.0f);
    M = glm::scale(M, dim);
    M = glm::inverse(scene->getSimulationDomain()->getTransform()) * M;
    return M;
}

glm::vec3 SimSpaceNode::translation_offset() const {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    return (scene->getSimulationDomain()->getScale() * (1.f - dim)) / (2.f * dim) + scene->getSimulationDomain()->getPosition();
}

#undef GLM_ENABLE_EXPERIMENTAL