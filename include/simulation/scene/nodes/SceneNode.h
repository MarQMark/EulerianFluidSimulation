//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_SCENENODE_H
#define SPHLASH_SCENENODE_H

#include "simulation/scene/ISelectable.h"
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "renderer/IRenderable.h"

class Scene;

class SceneNode : public ISelectable, public IRenderable {
public:
    SceneNode() = default;
    SceneNode(const nlohmann::json j);
    ~SceneNode() override = default;

    virtual void update(float deltaTime) = 0;

    // --- Name ---
    const std::string& getName() const;
    void setName(const std::string& name);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isDeletable() const;
    void setDeletable(bool deletable);

    // --- Editor Flags ---
    bool isSceneGraphHideable() const;
    void setSceneGraphHideable(bool hideable);

    bool isSceneGraphVisible() const;
    void setSceneGraphVisible(bool visible);

    bool showGizmo() const;
    void setShowGizmo(bool showGizmo);

    // --- Tags ---
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::unordered_set<std::string>& getTags() const;

    // --- Save / Load ---
    bool isSavable() const;
    void setSavable(bool savable);

    virtual nlohmann::json toJson() const;
    virtual std::string getTypeName() const;

    std::vector<SceneNode*> getChildren() const;

protected:
    std::string name;
    Scene* scene;

    bool visible = true;
    bool deletable = true;
    bool savable = true;

    bool sceneGraphHideable = true;
    bool sceneGraphVisible = true;
    bool gizmoEnabled = true;

    std::unordered_set<std::string> tags = {};

    std::vector<std::unique_ptr<SceneNode>> children;

    friend class Scene;
};

#endif //SPHLASH_SCENENODE_H
