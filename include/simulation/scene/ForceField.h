//
// Created by ruut on 6/19/26.
//

#ifndef EULERIANFLUIDSIMULATION_FORCEFIELD_H
#define EULERIANFLUIDSIMULATION_FORCEFIELD_H

#include <glm/glm.hpp>

struct ForceField {
    bool active         = true;
    float strength      = 5.0f;

    glm::vec3 direction = { 0.0, 0.0, 0.0 };
    std::vector<int> cells = {};
};

#endif //EULERIANFLUIDSIMULATION_FORCEFIELD_H
