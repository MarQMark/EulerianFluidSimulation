//
// Created by ruut on 4/29/26.
//

#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/BoundingBoxParticleSpawnNode.h"
#include "simulation/scene/nodes/ForceFieldNode.h"
#include "simulation/scene/nodes/ModelNode.h"
#include "simulation/scene/nodes/ModelParticleSpawnNode.h"
#include "core/Config.h"
#include <algorithm>

void Scene::update(float deltaTime) const {
    for (auto& node : sceneNodes)
        node->update(deltaTime);
}

CameraNode* Scene::getCamera() const {
    return camera;
}

void Scene::addSceneNode(std::unique_ptr<SceneNode> node) {
    if (auto *camera = dynamic_cast<CameraNode*>(node.get()))
        this->camera = camera;

    if (auto* model = dynamic_cast<ModelNode*>(node.get()))
        models.push_back(model);

    if (auto* boundingBox = dynamic_cast<BoundingBoxNode*>(node.get()))
        boundingBoxes.push_back(boundingBox);

    if (auto* forceField = dynamic_cast<ForceFieldNode*>(node.get()))
        forceFields.push_back(forceField);

    if (auto* particleSpawn = dynamic_cast<IParticleSpawn*>(node.get()))
        particleSpawns.push_back(particleSpawn);

    node->scene = this;
    sceneNodes.push_back(std::move(node));
}

void Scene::removeSceneNode(const SceneNode* node) {
    std::erase_if(models,         [&](const ModelNode* n)       { return n == node; });
    std::erase_if(boundingBoxes,  [&](const BoundingBoxNode* n) { return n == node; });
    std::erase_if(forceFields,    [&](const ForceFieldNode* n)  { return n == node; });

    if (const auto* ps = dynamic_cast<const IParticleSpawn*>(node))
        std::erase_if(particleSpawns, [&](const IParticleSpawn* n) { return n == ps; });

    std::erase_if(sceneNodes, [&](const std::unique_ptr<SceneNode>& n) { return n.get() == node; });
}

void Scene::setSimulationDomain(BoundingBoxNode* domain) {
    simulationDomain = domain;
}

BoundingBoxNode* Scene::getSimulationDomain() const {
    return simulationDomain;
}

std::vector<ModelNode*> Scene::getModels() const {
    return models;
}

std::vector<BoundingBoxNode*> Scene::getBoundingBoxes() const {
    return boundingBoxes;
}

std::vector<ForceFieldNode*> Scene::getForceFields() const {
    return forceFields;
}

std::vector<IParticleSpawn*> Scene::getParticleSpawns() const {
    return particleSpawns;
}

std::vector<SceneNode*> Scene::getSceneNodes() const {
    std::vector<SceneNode*> result;

    for (auto& node : sceneNodes)
        result.push_back(node.get());

    return result;
}

void Scene::saveScene(const std::string& path) const {
    Config config;

    // Simulation domain
    nlohmann::json domain;
    domain["position"] = {
        simulationDomain->getPosition().x,
        simulationDomain->getPosition().y,
        simulationDomain->getPosition().z
    };

    domain["scale"] = {
        simulationDomain->getScale().x,
        simulationDomain->getScale().y,
        simulationDomain->getScale().z
    };

    config.set<nlohmann::json>("simulationDomain", domain);

    // Scene Nodes
    nlohmann::json nodes = nlohmann::json::array();
    for (int i = 0 ; i < sceneNodes.size(); i++) {
        if (!sceneNodes[i]->isSavable())
            continue;

        // Skip simulation domain as saved separately
        // if (sceneNodes[i]->hasTag(FluidSimulation::SimulationDomainTag))
            // continue;

        nodes.push_back(sceneNodes[i]->toJson());
    }

    config.set<nlohmann::json>("nodes", nodes);
    config.save(path);
}

void Scene::loadScene(const std::string& path) {
    Config config;

    if (config.load(path)) {
        // Set simulation domain
        auto domain = config.get<nlohmann::json>("simulationDomain", {});
        simulationDomain->setPosition(glm::vec3(
            domain["position"][0].get<float>(),
            domain["position"][1].get<float>(),
            domain["position"][2].get<float>()
        ));
        simulationDomain->setScale(glm::vec3(
            domain["scale"][0].get<float>(),
            domain["scale"][1].get<float>(),
            domain["scale"][2].get<float>()
        ));

        // Load scene nodes
        for (auto &node : config.get<nlohmann::json>("nodes", {})) {
            auto sceneNode = nodeFromJson(node);
            addSceneNode(std::move(sceneNode));
        }
    }
}

void Scene::resetScene() {
    // Reset simulation domain
    simulationDomain->setPosition({ 0.0, 0.0, 0.0 });
    simulationDomain->setScale({ 2.0, 2.0, 2.0 });

    // Remove all user added SceneNodes
    std::vector<SceneNode*> toRemove;

    for (int i = 3 ; i < sceneNodes.size(); i++) {
        if (!sceneNodes[i]->isDeletable())
            continue;

        toRemove.push_back(sceneNodes[i].get());
    }

    for (auto* node : toRemove)
        removeSceneNode(node);
}

std::unique_ptr<SceneNode> Scene::nodeFromJson(const nlohmann::json& j) {
    const std::string type = j.at("type");

    if (type == "ModelNode")
        return std::make_unique<ModelNode>(j);

    if (type == "BoundingBoxNode")
        return std::make_unique<BoundingBoxNode>(j);

    if (type == "ForceFieldNode")
        return std::make_unique<ForceFieldNode>(j);

    if (type == "BoundingBoxParticleSpawnNode")
        return std::make_unique<BoundingBoxParticleSpawnNode>(j);

    if (type == "ModelParticleSpawnNode")
        return std::make_unique<ModelParticleSpawnNode>(j);

    throw std::runtime_error("Unknown node type: " + type);
}
