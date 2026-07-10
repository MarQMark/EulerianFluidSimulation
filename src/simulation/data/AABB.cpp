//
// Created by ruut on 5/2/26.
//

#define GLM_ENABLE_EXPERIMENTAL

#include "simulation/data/AABB.h"
#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"

void AABB::setCenter(glm::vec3 center) {
    this->center = center;
}

void AABB::setSize(glm::vec3 size) {
    this->size = size;
}

glm::vec3 AABB::min() const {
    return center - 0.5f * size;
}

glm::vec3 AABB::max() const {
    return center + 0.5f * size;
}

float AABB::volume() const {
    return size.x * size.y * size.z;
}

bool AABB::contains(const glm::vec3& point) const {
    glm::vec3 min = center - size * 0.5f;
    glm::vec3 max = center + size * 0.5f;

    return
        point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

glm::mat4 AABB::getTransformMatrix() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, center);
    m = glm::scale(m, size);
    return m;
}
