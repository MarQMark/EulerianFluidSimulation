//
// Created by ruut on 7/24/26.
//

#include "simulation/scene/nodes/QueryNode.h"

#include <fstream>

#include "simulation/Properties.h"
#include "simulation/scene/Scene.h"
#include "simulation/data/IGrid.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "util/Logger.h"

QueryNode::QueryNode()
    : SimSpaceNode("QueryNode", glm::vec3(0.0, 0.0, 0.7)) {
}

QueryNode::QueryNode(const nlohmann::json json) : SimSpaceNode(json) {
}

void QueryNode::update(float deltaTime) {
    SimSpaceNode::update(deltaTime);

    if (!_sim_data)
        return;

    const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(_sim_data);
    auto* grid = data->grid;
    if (!grid)
        return;

    if (PROPERTY_B(PAUSED))
        return;

    const int w = PROPERTY_I(WIDTH_3D);
    const int h = PROPERTY_I(HEIGHT_3D);
    const int d = PROPERTY_I(DEPTH_3D);

    glm::vec3 maxPos(-std::numeric_limits<float>::infinity());
    glm::vec3 minPos(std::numeric_limits<float>::infinity());
    glm::vec3 avgPos(0.f);
    glm::vec3 vel(0.f);
    float pres = 0;
    int count = 0;
    for (int z = std::max(0, (int)position.z); z < std::min(d - 1, (int)(position.z + scale.z)); z++) {
    for (int y = std::max(0, (int)position.y); y < std::min(h - 1, (int)(position.y + scale.y)); y++) {
    for (int x = std::max(0, (int)position.x); x < std::min(w - 1, (int)(position.x + scale.x)); x++) {
        if (grid->rho()(x, y, z) < 0.5f)
            continue;

        maxPos = glm::max(maxPos, glm::vec3(x, y, z));
        minPos = glm::min(minPos, glm::vec3(x, y, z));

        count++;
        avgPos += glm::vec3(x, y, z);
        vel += grid->vel()(x, y, z);
        pres += grid->pressure()(x, y, z);
    }}}

    if (count != 0) {
        avgPos /= count;
        vel /= count;
        pres /= count;
    }
    else {
        maxPos = glm::vec3(0.f);
        minPos = glm::vec3(0.f);
    }

    history.emplace_back(Metrics{maxPos, minPos, avgPos, vel, pres});
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

    file << "frame,"
            "max_pos_x,max_pos_y,max_pos_z,"
            "min_pos_x,min_pos_y,min_pos_z,"
            "avg_pos_x,avg_pos_y,avg_pos_z,"
            "vel_x,vel_y,vel_z,"
            "pressure\n";

    for (int i = 0 ; i < history.size(); i++)
        file << i << "," << history[i] << "\n";
}

NodeProperties QueryNode::buildProperties() {
    NodeProperties properties = SimSpaceNode::buildProperties();

    properties.addSection("Query");

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
    return j;
}

std::string QueryNode::getTypeName() const {
    return "QueryNode";
}