//
// Created by ruut on 6/13/26.
//

#ifndef SPHLASH_VERTEX_H
#define SPHLASH_VERTEX_H

#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    float _pad0{};

    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);
    float _pad1{};

    glm::vec2 texCoord = glm::vec2(0.0f, 0.0f);
    float _pad2[2]{};

    Vertex() = default;
    Vertex(const glm::vec3& pos, const glm::vec3&norm, const glm::vec2& uv)
        : position(pos), normal(norm), texCoord(uv) {};
};

static_assert(sizeof(Vertex) == 48);

#endif //SPHLASH_VERTEX_H
