//
// Created by ruut on 3/26/26.
//

#ifndef SPHLASH_SPHERECREATOR_H
#define SPHLASH_SPHERECREATOR_H

#include "glm/glm.hpp"

inline void createSphereData(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    int sectorCount = 16,
    int stackCount = 16
) {
    float radius = 1.0f;

    for (int i = 0; i <= stackCount; ++i) {
        float stackAngle = std::numbers::pi / 2 - i * std::numbers::pi/ stackCount;
        float xy = radius * cos(stackAngle);
        float z = radius * sin(stackAngle);

        for (int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * 2 * std::numbers::pi / sectorCount;

            float x = xy * cos(sectorAngle);
            float y = xy * sin(sectorAngle);

            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));

            Vertex v;
            v.position = { x, y, z };
            v.normal   = { normal.x, normal.y, normal.z };

            vertices.push_back(v);
        }
    }

    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

#endif //FLUIDSIMSPH_SPHERECREATOR_H