//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_SCENE_H
#define SPHLASH_SCENE_H

#include <vector>
#include <nlohmann/json_fwd.hpp>
#include "simulation/scene/nodes/CameraNode.h"

class IParticleSpawn;
class ForceFieldNode;
class ModelNode;
class ParticlesNode;
class BoundingBoxNode;
class SceneNode;

class Scene {
public:
    void update(float deltaTime) const;

    void addSceneNode(std::unique_ptr<SceneNode> node);
    void removeSceneNode(const SceneNode* node);

    CameraNode* getCamera() const;

    void setSimulationDomain(BoundingBoxNode* domain);
    BoundingBoxNode* getSimulationDomain() const;

    std::vector<ModelNode*> getModels() const;
    std::vector<BoundingBoxNode*> getBoundingBoxes() const;
    std::vector<ForceFieldNode*> getForceFields() const;
    std::vector<IParticleSpawn*> getParticleSpawns() const;
    std::vector<SceneNode*> getSceneNodes() const;

    void saveScene(const std::string& path) const;
    void loadScene(const std::string& path);
    void resetScene();

private:
    // --- Scene Nodes ---
    std::vector<std::unique_ptr<SceneNode>> sceneNodes  = {};

    // --- Node Cache ---
    CameraNode*                     camera              = nullptr;
    BoundingBoxNode*                simulationDomain    = nullptr;
    std::vector<ModelNode*>         models              = {};
    std::vector<BoundingBoxNode*>   boundingBoxes       = {};
    std::vector<ForceFieldNode*>    forceFields         = {};
    std::vector<IParticleSpawn*>    particleSpawns      = {};

    std::unique_ptr<SceneNode> nodeFromJson(const nlohmann::json& j);
};

#endif //SPHLASH_SCENE_H
