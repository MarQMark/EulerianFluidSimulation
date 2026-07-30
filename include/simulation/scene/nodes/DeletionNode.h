#ifndef EULERIANFLUIDSIMULATION_DELETIONNODE_H
#define EULERIANFLUIDSIMULATION_DELETIONNODE_H

#include "SimSpaceNode.h"

class DeletionNode : public SimSpaceNode {
public:
    DeletionNode();
    explicit DeletionNode(nlohmann::json json);

    void update(float deltaTime) override;

    std::string getTypeName() const override;
};

#endif //EULERIANFLUIDSIMULATION_DELETIONNODE_H