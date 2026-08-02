//
// Created by ruut on 7/24/26.
//

#ifndef SPHLASH_QUERYNODE_H
#define SPHLASH_QUERYNODE_H

#include "SimSpaceNode.h"

struct Metrics {
    glm::vec3 maxPosition = { 0.0, 0.0, 0.0 };
    glm::vec3 minPosition = { 0.0, 0.0, 0.0 };
    glm::vec3 avgPosition = { 0.0, 0.0, 0.0 };
    glm::vec3 velocity    = { 0.0, 0.0, 0.0 };
    float pressure        = 0;
    float gravity         = 0;
};

inline std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os
        << m.maxPosition.x << "," << m.maxPosition.y << "," << m.maxPosition.z << ","
        << m.minPosition.x << "," << m.minPosition.y << "," << m.minPosition.z << ","
        << m.avgPosition.x << "," << m.avgPosition.y << "," << m.avgPosition.z << ","
        << m.velocity.x << "," << m.velocity.y << "," << m.velocity.z << ","
        << m.pressure <<  "," << m.gravity;

    return os;
}

class QueryNode : public SimSpaceNode {
public:
    QueryNode();
    explicit QueryNode(const nlohmann::json json);

    void update(float deltaTime) override;

    void reset();
    void exportData(const std::string& path);

    std::vector<Metrics>& getHistory();

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    bool _enabled = true;
    std::vector<Metrics> _history{};
    std::vector<Metrics> _avg_history{};

    enum QueryMode {
        SINGLE_SIMULATION,
        CHANGE_GRAVITY,
        CHANGE_Y_POS
    };
    QueryMode _mode = SINGLE_SIMULATION;
    int _avg_frames = 5;
    int _delay_frames = 20;
    float _change_value = 0.05f;
    int _iterations = 200;
};

#endif //SPHLASH_QUERYNODE_H
