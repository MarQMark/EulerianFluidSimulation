//
// Created by ruut on 5/2/26.
//

#ifndef SPHLASH_AABB_H
#define SPHLASH_AABB_H

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

struct AABB {
    glm::vec3 center    = {};
    glm::vec3 size      = {};

    void setCenter(glm::vec3 center);
    void setSize(glm::vec3 size);

    glm::vec3 min() const;
    glm::vec3 max() const;

    float volume() const;
    bool contains(const glm::vec3& point) const;

    glm::mat4 getTransformMatrix() const;
};

#endif //SPHLASH_AABB_H
