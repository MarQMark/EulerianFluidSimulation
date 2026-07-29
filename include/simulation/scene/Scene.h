//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <vector>
#include <nlohmann/json_fwd.hpp>

#include "nodes/SimulationDomainNode.h"
#include "simulation/scene/nodes/CameraNode.h"

class QueryNode;
class FluidNode;
class IFluidSpawn;
class ForceFieldNode;
class ModelNode;
class ParticlesNode;
class BoundingBoxNode;
class SceneNode;

class Scene {
public:
    Scene(Window* window);

    void update(float deltaTime, void* simulationData) const;

    void addSceneNode(std::unique_ptr<SceneNode> node);
    void removeSceneNode(const SceneNode* node);

    CameraNode* getCamera() const;
    FluidNode* getFluid() const;

    SimulationDomainNode* getSimulationDomain() const;

    std::vector<ModelNode*> getModels() const;
    std::vector<BoundingBoxNode*> getBoundingBoxes() const;
    std::vector<ForceFieldNode*> getForceFields() const;
    std::vector<IFluidSpawn*> getFluidSpawns() const;
    std::vector<SceneNode*> getSceneNodes() const;
    std::vector<QueryNode*> getQueryNodes() const;

    void saveScene(const std::string& path) const;
    void loadScene(const std::string& path);
    void resetScene();

private:
    // --- Scene Nodes ---
    std::vector<std::unique_ptr<SceneNode>> sceneNodes  = {};

    // --- Node Cache ---
    CameraNode*                     camera              = nullptr;
    FluidNode*                      fluid               = nullptr;
    SimulationDomainNode*           simulationDomain    = nullptr;

    std::vector<ModelNode*>         models              = {};
    std::vector<BoundingBoxNode*>   boundingBoxes       = {};
    std::vector<ForceFieldNode*>    forceFields         = {};
    std::vector<IFluidSpawn*>    particleSpawns         = {};
    std::vector<QueryNode*>         queryNodes          = {};

    std::unique_ptr<SceneNode> nodeFromJson(const nlohmann::json& j);
};

#endif //SPHLASH_SCENE_H
