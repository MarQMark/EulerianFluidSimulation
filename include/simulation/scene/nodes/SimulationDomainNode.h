//
// Created by ruut on 6/30/26.
//

#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H

#include "BoundingBoxNode.h"

struct SimulationDomainSettings {
    glm::ivec3 resolution = glm::ivec3(10, 10, 10);
    float percentageFilled = 0.0f;
};

class SimulationDomainNode : public BoundingBoxNode {
public:
    SimulationDomainNode();
    explicit SimulationDomainNode(const nlohmann::json& json);
    NodeProperties buildProperties() override;
    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

    SimulationDomainSettings getDomainSettings() const;
private:
    SimulationDomainSettings _settings;
};

#endif //EULERIANFLUIDSIMULATION_SIMULATIONDOMAINNODE_H
