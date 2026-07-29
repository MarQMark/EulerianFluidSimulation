//
// Created by ruut on 7/24/26.
//

#include "simulation/scene/nodes/QueryNode.h"
#include <fstream>

#include "simulation/Properties.h"
#include "simulation/scene/Scene.h"
#include "util/Logger.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "simulation/scene/Transform.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "simulation/data/IGrid.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"
#include "simulation/display/SimulationDisplay3D.h"

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
}

void QueryNode::update(float deltaTime) {
    const int w = PROPERTY_I(WIDTH_3D);
    const int h = PROPERTY_I(HEIGHT_3D);
    const int d = PROPERTY_I(DEPTH_3D);
    const auto dim = glm::vec3(w, h, d);

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);

    if (!_sim_data)
        return;

    const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(_sim_data);
    auto* grid = data->grid;
    if (!grid)
        return;

    if (PROPERTY_B(PAUSED))
        return;

    glm::vec3 pos(0.f);
    glm::vec3 vel(0.f);
    float pres = 0;
    int count = 0;
    for (int z = std::max(0, (int)position.z); z < std::min(d - 1, (int)(position.z + scale.z)); z++)
    for (int y = std::max(0, (int)position.y); y < std::min(h - 1, (int)(position.y + scale.y)); y++)
    for (int x = std::max(0, (int)position.x); x < std::min(w - 1, (int)(position.x + scale.x)); x++) {
        if (grid->rho()(x, y, z) < 0.5f)
            continue;

        count++;
        pos += glm::vec3(x, y, z);
        vel += grid->vel()(x, y, z);
        pres += grid->pressure()(x, y, z);
    }
    if (count != 0) {
        pos /= count;
        vel /= count;
        pres /= count;
    }

    history.emplace_back(Metrics{pos, vel, pres});
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
            "pos_x,pos_y,pos_z,"
            "vel_x,vel_y,vel_z,"
            "pressure\n";

    for (int i = 0 ; i < history.size(); i++)
        file << i << "," << history[i] << "\n";
}

void QueryNode::draw(const glm::mat4 &view, const glm::mat4 &projection) const {

    material->bind();
    material->getShader()->setMat4("view", view);
    material->getShader()->setMat4("projection", projection);

    const auto mat = scene->getSimulationDomain()->getTransform() * getVoxelTransform();
    material->getShader()->setMat4("model", mat);

    mesh->draw();
}

glm::mat4 QueryNode::getVoxelTransform() const {
    const glm::vec3 dim(
        PROPERTY_I(WIDTH_3D),
        PROPERTY_I(HEIGHT_3D),
        PROPERTY_I(DEPTH_3D));

    glm::mat4 M(1.0f);

    // Normalize the voxel grid to [-0.5,0.5]
    M = glm::scale(M, 1.f / dim);
    M = glm::translate(M, -dim * .5f + (glm::vec3(.5) * scale));
    M = glm::translate(M, position);
    M = glm::scale(M, scale);

    return M;
}

glm::mat4 QueryNode::getInvVoxelTransform() const {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    glm::mat4 M(1.0f);
    M = glm::scale(M, dim);
    M = glm::inverse(scene->getSimulationDomain()->getTransform()) * M;
    return M;
}

glm::vec3 QueryNode::getTranslationOffset() const {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    return (scene->getSimulationDomain()->getScale() * (1.f - dim)) / (2.f * dim) + scene->getSimulationDomain()->getPosition();
}


void QueryNode::setTransform(const AABB &boundingBox) {}

void QueryNode::setTransform(glm::mat4 transform) {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    transform = glm::translate(glm::mat4(1.f), -getTranslationOffset()) * transform;
    Transform::transform = transform;

    glm::vec3 skew;
    glm::vec4 perspective;
    glm::vec3 s;
    glm::vec3 p;

    glm::decompose(
        transform,
        scale,
        rotation,
        p,
        skew,
        perspective);

    auto inv = getInvVoxelTransform();
    position = inv * glm::vec4(p, 1.f);
    //scale = inv * glm::vec4(s, 1.f);

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);
}

glm::mat4 QueryNode::getTransform() const {
    return glm::translate(glm::mat4(1.f), getTranslationOffset()) * transform;
}

void QueryNode::updateTransform() {
    const glm::vec3 dim(
    PROPERTY_I(WIDTH_3D),
    PROPERTY_I(HEIGHT_3D),
    PROPERTY_I(DEPTH_3D));

    position = glm::clamp(glm::round(position),glm::vec3(0.0f),dim);
    scale = glm::clamp(glm::round(scale),glm::vec3(1.0f),dim - position);

    glm::mat4 M(1.0f);

    M = glm::scale(M, 2.f / dim);
    M = glm::translate(M, position);
    M[0][0] = scale.x;
    M[1][1] = scale.y;
    M[2][2] = scale.z;
    transform = M;
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

void QueryNode::setSimulationData(void *data) {
    _sim_data = data;
}
