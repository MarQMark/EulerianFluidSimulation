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

    SET_PROPERTY_I(WIDTH_3D, _settings.resolution.x);
    SET_PROPERTY_I(HEIGHT_3D, _settings.resolution.y);
    SET_PROPERTY_I(DEPTH_3D, _settings.resolution.z);
}

SimulationDomainNode::SimulationDomainNode(const nlohmann::json& json) : BoundingBoxNode(json) {
    addTag("simulation_Domain");
    setDeletable(false);

    auto res = json.value("resolution", std::vector{_settings.resolution.x, _settings.resolution.y, _settings.resolution.z});
    _settings.resolution = { res[0], res[1], res[2] };

    SET_PROPERTY_I(WIDTH_3D, _settings.resolution.x);
    SET_PROPERTY_I(HEIGHT_3D, _settings.resolution.y);
    SET_PROPERTY_I(DEPTH_3D, _settings.resolution.z);
}

NodeProperties SimulationDomainNode::buildProperties() {
    NodeProperties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Simulation Domain");
    properties.addProperty(std::make_unique<IVec3Property>(
        "Resolution",
        [this] () { return _settings.resolution; },
        [this] (glm::ivec3 res) {
            _settings.resolution = res;

            SET_PROPERTY_I(WIDTH_3D, _settings.resolution.x);
            SET_PROPERTY_I(HEIGHT_3D, _settings.resolution.y);
            SET_PROPERTY_I(DEPTH_3D, _settings.resolution.z);

            SET_PROPERTY_B(PAUSED, true);
            SET_PROPERTY_B(SIMULATION_RESET, true);
        }
    ));

    properties.addProperty(std::make_unique<FloatProperty>(
        "Initial Fill (%)",
        [this] () { return _settings.percentageFilled * 100; },
        [this] (float percentage) {
            _settings.percentageFilled = percentage / 100.0f;

            SET_PROPERTY_B(PAUSED, true);
            SET_PROPERTY_B(SIMULATION_RESET, true);
        },
        0,
        100,
        0.1f
    ));

    return properties;
}

nlohmann::json SimulationDomainNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["resolution"] = { _settings.resolution.x, _settings.resolution.y, _settings.resolution.z };

    return j;
}

std::string SimulationDomainNode::getTypeName() const {
    return "SimulationDomainNode";
}

SimulationDomainSettings SimulationDomainNode::getDomainSettings() const {
    return _settings;
}

std::string SimulationDomainNode::getDefaultName() const {
    return "Simulation Domain";
}
