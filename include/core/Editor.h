//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H

#include "simulation/data/StorageBuffer.h"
#include "simulation/data/VecField3D.h"
#include "simulation/scene/nodes/ForceFieldNode.h"
#include "simulation/scene/Scene.h"

class ScalField3D;
class IGrid3D;

class Editor {
public:
    Editor();

    void init(IGrid3D* grid);

    void setScene(Scene* scene);
    Scene* getScene() const;

    void setSelected(SceneNode* node);
    SceneNode* getSelected() const;

    void removeSceneNode(SceneNode* node) const;

    // --- Boundary ---
    void addBoundary(const std::string& path);
    void removeBoundary(const ModelNode* boundaryNode) const;

    // --- Force Fields ---
    void addForceField();
    void removeForceField(const ForceFieldNode* forceFieldNode) const;
    void updateForceFields(VecField3D& forces) const;

    // --- Particle Spawns ---
    void addParticleSpawn(const std::string& path);

    // --- Query Node ---
    void addQuery();

    // --- Deletion Area ---
    void addDeletionArea();

    // --- Window Resizes ---
    void updateRenderingSize(int width, int height) const;

    void initBoundaries(ScalField3D &boundary, ScalField3D &rho) const;


private:
    void initFluid(ScalField3D& rho, StorageBuffer* gpuRho = nullptr) const;

    Scene* _scene;
    SceneNode* _selectedNode = nullptr;

    std::unique_ptr<Shader> _sdfShader;
    std::unique_ptr<Shader> _baseFluidFillShader;
    std::unique_ptr<Shader> _forceFieldCellsShader;
};

#endif //SPHLASH_EDITOR_H
