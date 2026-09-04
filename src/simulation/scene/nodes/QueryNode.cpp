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
    _enabled = json.value("enabled", _enabled);
    _mode = json.value("mode", _mode);
    _avg_frames = json.value("avgFrames", _avg_frames);
    _delay_frames = json.value("delayFrames", _delay_frames);
    _change_value = json.value("changeValue", _change_value);
    _iterations = json.value("iterations",_iterations);
    _only_fluid = json.value("onlyFluid",_only_fluid);
}

void QueryNode::update(float deltaTime) {
    SimSpaceNode::update(deltaTime);

    if (!_enabled)
        return;

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
        if (_only_fluid && grid->rho()(x, y, z) < 0.5f)
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

    if (_mode == SINGLE_SIMULATION) {
        _history.emplace_back(Metrics{maxPos, minPos, avgPos, vel, pres, PROPERTY_F(GRAVITY_FORCE_PHYSICAL)});
        return;
    }

    if (PROPERTY_I(SIMULATION_FRAMES) < _delay_frames)
        return;

    if (PROPERTY_I(SIMULATION_FRAMES) >= _delay_frames + _avg_frames) {
        Metrics metrics;
        for (auto& m : _avg_history) {
            metrics.maxPosition += m.maxPosition;
            metrics.minPosition += m.minPosition;
            metrics.avgPosition += m.avgPosition;
            metrics.velocity += m.velocity;
            metrics.pressure += m.pressure;
            metrics.gravity += m.gravity;
        }
        metrics.maxPosition /= _avg_history.size();
        metrics.minPosition /= _avg_history.size();
        metrics.avgPosition /= _avg_history.size();
        metrics.velocity /= _avg_history.size();
        metrics.pressure /= _avg_history.size();
        metrics.gravity /= _avg_history.size();
        _history.emplace_back(metrics);
        _avg_history.clear();
        SET_PROPERTY_B(SIMULATION_RESET, true);

        if (_mode == CHANGE_GRAVITY)
            ADD_PROPERTY_F(GRAVITY_FORCE_PHYSICAL, _change_value);
        else if (_mode == CHANGE_Y_POS)
            setPosition(position + glm::vec3(0, _change_value, 0));

        _iterations--;
    }
    else {
        _avg_history.emplace_back(Metrics{maxPos, minPos, avgPos, vel, pres, PROPERTY_F(GRAVITY_FORCE_PHYSICAL)});
    }

    if (_iterations == 0)
        SET_PROPERTY_B(PAUSED, true);
}

void QueryNode::reset() {
    _history.clear();
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
            "pressure,gravity,";
    file << "dt,cell_size,"
            "grid_width,grid_height,grid_depth";

    for (const auto* p : Properties::Get()->getProperties()) {
        if (p && !p->name.empty())
            file << "," << p->name;
    }
    file << "\n";

    for (int i = 0 ; i < _history.size(); i++) {
        file << i << "," << _history[i];
        if (i == 0) {
            file << ","
                 << PROPERTY_F(DT) << ","
                 << PROPERTY_F(CELL_SIZE) << ","
                 << PROPERTY_I(WIDTH_3D) << "," << PROPERTY_I(HEIGHT_3D) << "," << PROPERTY_I(DEPTH_3D);

            for (auto* baseP : Properties::Get()->getProperties()) {
                if (!baseP || baseP->name.empty())
                    continue;

                if (auto* p = dynamic_cast<Properties::Property<int>*>(baseP))
                    file << "," << std::to_string(p->value);
                else if (auto* p = dynamic_cast<Properties::Property<unsigned int>*>(baseP))
                    file << "," <<  std::to_string(p->value);
                else if (auto* p = dynamic_cast<Properties::Property<float>*>(baseP))
                    file << "," <<  std::to_string(p->value);
                else if (auto* p = dynamic_cast<Properties::Property<bool>*>(baseP))
                    file << "," <<  std::to_string(p->value);
                else
                    file << ",nan";
            }
        }
        file << "\n";
    }
}

NodeProperties QueryNode::buildProperties() {
    NodeProperties properties = SimSpaceNode::buildProperties();

    properties.addSection("Query");
    properties.addProperty(std::make_unique<BoolProperty>(
        "Enabled",
        [this] () { return _enabled; },
        [this] (const bool on) { _enabled = on;}
    ));
    properties.addProperty(std::make_unique<ButtonProperty>(
        "Clear Data",
        [this] {
            _history.clear();
        }
    ));

    properties.addProperty(std::make_unique<DialogProperty>(
        "Export Data",
        [this](const std::string& path) {
            exportData(path);
        },
        ".csv"
    ));

    properties.addSection("Multiple Simulations");
    properties.addProperty(std::make_unique<EnumProperty<QueryMode>>(
        "Query Mode",
        [this] () { return _mode; },
        [this] (const QueryMode c) { _mode = c; },
        std::vector<std::pair<QueryMode, std::string>>{
            { SINGLE_SIMULATION, "Single Simulation" },
            { CHANGE_GRAVITY, "Change Gravity" },
            { CHANGE_Y_POS, "Change Y-Position" }
        }
    ));
    properties.addProperty(std::make_unique<IntProperty>(
        "Average Frames",
        [this] () { return _avg_frames; },
        [this] (int s) { _avg_frames = s; },
        1, INT_MAX, 0.01f
    ));
    properties.addProperty(std::make_unique<IntProperty>(
        "Delay Frames",
        [this] () { return _delay_frames; },
        [this] (int s) { _delay_frames = s; },
        0, INT_MAX, 0.01f
    ));
    properties.addProperty(std::make_unique<FloatProperty>(
        "Change Amount",
        [this] () { return _change_value; },
        [this] (float s) { _change_value = s;},
        -INFINITY, INFINITY, 0.01f
    ));
    properties.addProperty(std::make_unique<IntProperty>(
        "Iterations",
        [this] () { return _iterations; },
        [this] (int s) { _iterations = s; },
        1, INT_MAX, 0.01f
    ));
    properties.addProperty(std::make_unique<BoolProperty>(
        "Only query Fluid cells",
        [this] () { return _only_fluid; },
        [this] (const bool on) { _only_fluid = on;}
    ));

    return properties;
}

std::vector<Metrics>& QueryNode::getHistory() {
    return _history;
}

nlohmann::json QueryNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();
    j["enabled"] = _enabled;
    j["mode"] = _mode;
    j["avgFrames"] = _avg_frames;
    j["delayFrames"] = _delay_frames;
    j["changeValue"] = _change_value;
    j["iterations"] = _iterations;
    j["onlyFluid"] = _only_fluid;
    return j;
}

std::string QueryNode::getTypeName() const {
    return "QueryNode";
}