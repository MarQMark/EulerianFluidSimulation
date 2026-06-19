//
// Created by ruut on 5/31/26.
//

#define GLM_ENABLE_EXPERIMENTAL

#include "simulation/scene/Transform.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

glm::mat4 Transform::getTransform() const {
    return transform;
}

void Transform::setTransform(glm::mat4 transform) {
    this->transform = transform;

    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(
        transform,
        scale,
        rotation,
        position,
        skew,
        perspective
    );
}

glm::vec3 Transform::getPosition() {
    return position;
}

void Transform::setPosition(glm::vec3 position) {
    this->position = position;
    updateTransform();
}

glm::quat Transform::getRotation() const {
    return rotation;
}

glm::vec3 Transform::getEulerRotation() const {
    return glm::degrees(glm::eulerAngles(rotation));
}

void Transform::setRotation(glm::quat rotation) {
    this->rotation = rotation;
    updateTransform();
}

void Transform::setEulerRotation(glm::vec3 rotation) {
    this->rotation = glm::quat(glm::radians(rotation));
    updateTransform();
}

glm::vec3 Transform::getScale() const {
    return scale;
}

void Transform::setScale(glm::vec3 scale) {
    this->scale = scale;
    updateTransform();
}

void Transform::updateTransform() {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    transform = T * R * S;
}
