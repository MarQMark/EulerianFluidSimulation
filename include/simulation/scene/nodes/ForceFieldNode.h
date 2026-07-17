//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include "assets/Model.h"
#include "simulation/scene/Transform.h"
#include "SceneNode.h"
#include "simulation/scene/ForceField.h"

class ForceFieldNode : public Transform, public SceneNode {
public:
    ForceFieldNode(const std::string& name);
    explicit ForceFieldNode(const nlohmann::json json);

    ForceField getForceField() const;
    bool updatePending() const;

    void resetCells();
    void setCells(const std::vector<int>& cells);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setTransform(glm::mat4 transform) override;
    void updateTransform() override;

    std::vector<Triangle> getWorldSpaceTriangles() const;

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    std::unique_ptr<Model> _model;
    std::unique_ptr<Model> _wireframeModel;

    ForceField forceField;
    bool pendingUpdate = false;
};

#endif //SPHLASH_FORCEMODEL_H
