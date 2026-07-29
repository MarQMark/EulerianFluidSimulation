//
// Created by ruut on 7/24/26.
//

#ifndef SPHLASH_QUERYNODE_H
#define SPHLASH_QUERYNODE_H

#include "BoundingBoxNode.h"

struct Metrics {
    glm::vec3 position              = { 0.0, 0.0, 0.0 };
    glm::vec3 velocity              = { 0.0, 0.0, 0.0 };
    float pressure                  = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os
        << m.position.x << "," << m.position.y << "," << m.position.z << ","
        << m.velocity.x << "," << m.velocity.y << "," << m.velocity.z << ","
        << m.pressure;

    return os;
}

class QueryNode : public BoundingBoxNode {
public:
    QueryNode();
    explicit QueryNode(const nlohmann::json json);

    void init() override;
    void update(float deltaTime) override;

    void reset();
    void exportData(const std::string& path);

    void draw(const glm::mat4 &view, const glm::mat4 &projection) const override;
    void setTransform(const AABB& boundingBox) override;
    void setTransform(glm::mat4 transform) override;
    glm::mat4 getTransform() const override;
    void updateTransform() override;

    std::vector<Metrics>& getHistory();

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

    void setSimulationData(void* data);

private:
    int filter = 0;

    glm::mat4 getVoxelTransform() const;
    glm::mat4 getInvVoxelTransform() const;
    glm::vec3 getTranslationOffset() const;

    glm::mat4 _model_mat;
    std::vector<Metrics> history;

    void* _sim_data{};
};

#endif //SPHLASH_QUERYNODE_H
