//
// Created by ruut on 5/31/26.
//

#include "../../../../include/simulation/scene/nodes/SceneNode.h"

SceneNode::SceneNode(const nlohmann::json j) {
    this->name = j["name"];
    this->visible = j["visible"];
    this->tags = j["tags"];
}

const std::string& SceneNode::getName() const {
    return name;
}

void SceneNode::setName(const std::string& name) {
    this->name = name;
}

bool SceneNode::isVisible() const {
    return visible;
}

void SceneNode::setVisible(bool visible) {
    this->visible = visible;
}

bool SceneNode::isSceneGraphVisible() const {
    return sceneGraphVisible;
}

void SceneNode::setSceneGraphVisible(bool visible) {
    this->sceneGraphVisible = visible;
}

bool SceneNode::showGizmo() const {
    return gizmoEnabled;
}

void SceneNode::setShowGizmo(bool showGizmo) {
    gizmoEnabled = showGizmo;
}

void SceneNode::addTag(const std::string& tag) {
    tags.insert(tag);
}

void SceneNode::removeTag(const std::string& tag) {
    tags.erase(tag);
}

bool SceneNode::hasTag(const std::string& tag) const {
    return tags.contains(tag);
}

const std::unordered_set<std::string>& SceneNode::getTags() const {
    return tags;
}

bool SceneNode::isSavable() const {
    return savable;
}

void SceneNode::setSavable(bool savable) {
    this->savable = savable;
}

nlohmann::json SceneNode::toJson() const {
    nlohmann::json j;

    j["type"] = getTypeName();
    j["name"] = name;
    j["visible"] = visible;
    j["tags"] = tags;

    return j;
}

std::string SceneNode::getTypeName() const {
    return "SceneNode";
}

std::vector<SceneNode*> SceneNode::getChildren() const {
    std::vector<SceneNode*> result;

    for (auto &c : children)
        result.push_back(c.get());

    return result;
}

bool SceneNode::isDeletable() const {
    return deletable;
}

void SceneNode::setDeletable(bool deletable) {
    this->deletable = deletable;
}

bool SceneNode::isSceneGraphHideable() const {
    return sceneGraphHideable;
}

void SceneNode::setSceneGraphHideable(bool hideable) {
    sceneGraphHideable = hideable;
}
