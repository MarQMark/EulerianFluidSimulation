//
// Created by ruut on 6/30/26.
//

#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H

#include "BoundingBoxNode.h"

struct SimulationDomain {
    glm::ivec3 resolution = glm::ivec3(10, 10, 10);
};

class SimulationDomainNode : public BoundingBoxNode {
public:
    SimulationDomainNode();
    explicit SimulationDomainNode(const nlohmann::json& json);
    NodeProperties buildProperties() override;
    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    SimulationDomain _domain;
};

#endif //EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H
