//
// Created by ruut on 7/24/26.
//

#include "simulation/scene/nodes/QueryNode.h"
#include <cuda_runtime_api.h>
#include <c++/16/fstream>
#include <cub/device/device_reduce.cuh>
#include "util/Logger.h"

QueryNode::QueryNode()
    : BoundingBoxNode("QueryNode", glm::vec3(0.0, 0.0, 0.7))
{
    QueryNode::init();
}

QueryNode::QueryNode(const nlohmann::json json) : BoundingBoxNode(json) {
    filter = json.value("filter", filter);

    QueryNode::init();
}

void QueryNode::init() {
    // TODO - Step 3. Setup init. Is called on node creation and on simulation start
}

void QueryNode::update(float deltaTime) {
    // TODO - Step 4. Query data from simulation. Probably in kernel > Voxelate, get data, reduce to avg
    // At the end append a Metrics object with the current data to the history vector
}

void QueryNode::reset() {
    history.clear();
}

void QueryNode::exportData(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Could not open destination file.");
        return;
    }

    // TODO - Step 5. Update CSV header
    // file << "frame,"
    //         "count,"
    //         "pos_x,pos_y,pos_z,"
    //         "vel_x,vel_y,vel_z,"
    //         "pressure_force_x,pressure_force_y,pressure_force_z,"
    //         "viscosity_force_x,viscosity_force_y,viscosity_force_z,"
    //         "surface_tension_force_x,surface_tension_force_y,surface_tension_force_z,"
    //         "density,"
    //         "densityError,"
    //         "pressure\n";

    for (int i = 0 ; i < history.size(); i++)
        file << i << "," << history[i] << "\n";
}

NodeProperties QueryNode::buildProperties() {
    NodeProperties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Query");

    properties.addProperty(std::make_unique<EnumProperty<int>>(
        "Filter",
        [this] () { return filter; },
        [this] (int index) { filter = index; },
        std::vector<std::pair<int, std::string>>{{ 0, "All Particles" }, { 1, "Top Surface Particles"} }
    ));

    properties.addProperty(std::make_unique<ButtonProperty>(
        "Clear Data",
        [this] {
            history.clear();
        }
    ));

    properties.addProperty(std::make_unique<DialogProperty>(
        "Export Data",
        [this](const std::string& path) {
            exportData(path);
        },
        ".csv"
    ));

    return properties;
}

std::vector<Metrics>& QueryNode::getHistory() {
    return history;
}

nlohmann::json QueryNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["filter"] = filter;

    return j;
}

std::string QueryNode::getTypeName() const {
    return "QueryNode";
}
