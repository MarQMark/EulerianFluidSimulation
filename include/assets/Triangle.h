//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_TRIANGLE_H
#define SPHLASH_TRIANGLE_H

#include <glm/glm.hpp>

struct Triangle {
    glm::vec3 v0;
    float _pad0;

    glm::vec3 v1;
    float _pad1;

    glm::vec3 v2;
    float _pad2;

    Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        : v0(a), _pad0(0), v1(b), _pad1(0), v2(c), _pad2(0) { }
};

#endif //SPHLASH_TRIANGLE_H
