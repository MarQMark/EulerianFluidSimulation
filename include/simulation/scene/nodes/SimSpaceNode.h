#ifndef EULERIANFLUIDSIMULATION_SIMSPACENODE_H
#define EULERIANFLUIDSIMULATION_SIMSPACENODE_H

#include "BoundingBoxNode.h"

class SimSpaceNode : public BoundingBoxNode {
protected:
    SimSpaceNode(const std::string& name, glm::vec3 color);
    explicit SimSpaceNode(const nlohmann::json json);

public:
    void update(float deltaTime) override;

    void draw(const glm::mat4 &view, const glm::mat4 &projection) const override;
    void setTransform(const AABB& boundingBox) override;
    void setTransform(glm::mat4 transform) override;
    glm::mat4 getTransform() const override;
    void updateTransform() override;

    void setSimulationData(void* data);

    NodeProperties buildProperties() override;

private:

    glm::mat4 voxel_transform() const;
    glm::mat4 inv_voxel_transform() const;
    glm::vec3 translation_offset() const;

protected:
    void* _sim_data{};
};

#endif //EULERIANFLUIDSIMULATION_SIMSPACENODE_H