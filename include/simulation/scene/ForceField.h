//
// Created by ruut on 6/19/26.
//

#ifndef EULERIANFLUIDSIMULATION_FORCEFIELD_H
#define EULERIANFLUIDSIMULATION_FORCEFIELD_H

#include <glm/glm.hpp>

struct ForceField {
    glm::vec3 position     = { 0.0f, 0.0f, 0.0f };
    glm::vec3 halfExtents  = { 0.2f, 0.2f, 0.2f };

    glm::vec3 right        = { 1.0f, 0.0f, 0.0f };
    glm::vec3 up           = { 0.0f, 1.0f, 0.0f };
    glm::vec3 forward      = { 0.0f, 0.0f, 1.0f };

    bool active         = true;
    float strength      = 5.0f;
};

#endif //EULERIANFLUIDSIMULATION_FORCEFIELD_H
