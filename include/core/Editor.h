//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "simulation/scene/nodes/ForceFieldNode.h"
#include "simulation/scene/Scene.h"

class Grid3D;

class Editor {
public:
    void initSimulation(Grid3D* grid);

    void setScene(Scene* scene);
    Scene* getScene() const;

    void setSelected(SceneNode* node);
    SceneNode* getSelected() const;

    void removeSceneNode(SceneNode* node) const;

    // --- Boundary ---
    void addBoundary(const std::string& path);
    void removeBoundary(const ModelNode* boundaryNode) const;

    // BoundaryBuilder* getBoundaryBuilder();

    // --- Force Fields ---
    void addForceField();
    void removeForceField(const ForceFieldNode* forceFieldNode) const;

    // --- Particle Spawns ---
    void addParticleSpawn(const std::string& path);

    // --- Window Resizes ---
    void updateRenderingSize(int width, int height) const;
private:
    Scene* scene;
    SceneNode* selectedNode = nullptr;
};

#endif //SPHLASH_EDITOR_H
