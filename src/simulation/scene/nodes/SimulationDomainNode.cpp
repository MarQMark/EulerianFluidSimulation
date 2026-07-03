//
// Created by ruut on 6/30/26.
//

#include "simulation/scene/nodes/SimulationDomainNode.h"
#include "simulation/Properties.h"

SimulationDomainNode::SimulationDomainNode()
    : BoundingBoxNode("Simulation Domain", glm::vec4(0.2f, 0.6f, 1.0f, 1.0f))
{
    BoundingBoxNode::setScale({ 2.0f, 2.0f, 2.0f });
    addTag("simulation_Domain");
    setDeletable(false);

    SET_PROPERTY_I(WIDTH_3D, _domain.resolution.x);
    SET_PROPERTY_I(HEIGHT_3D, _domain.resolution.y);
    SET_PROPERTY_I(DEPTH_3D, _domain.resolution.z);
}

SimulationDomainNode::SimulationDomainNode(const nlohmann::json& json) : BoundingBoxNode(json) {
    // TODO - Read from json
}

NodeProperties SimulationDomainNode::buildProperties() {
    NodeProperties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Simulation Domain");
    properties.addProperty(std::make_unique<IVec3Property>(
        "Resolution",
        [this] () { return _domain.resolution; },
        [this] (glm::ivec3 res) {
            _domain.resolution = res;

            SET_PROPERTY_I(WIDTH_3D, _domain.resolution.x);
            SET_PROPERTY_I(HEIGHT_3D, _domain.resolution.y);
            SET_PROPERTY_I(DEPTH_3D, _domain.resolution.z);

            SET_PROPERTY_B(PAUSED, true);
            SET_PROPERTY_B(SIMULATION_INITIALIZED, false);
        }
    ));

    return properties;
}
