//
// Created by ruut on 6/11/26.
//

#ifndef SPHLASH_MODELPARTICLESPAWNNODE_H
#define SPHLASH_MODELPARTICLESPAWNNODE_H

#include "assets/Model.h"
#include "ModelNode.h"
#include "simulation/scene/IFluidSpawn.h"
#include "SceneNode.h"

class ModelFluidSpawnNode : public ModelNode, public IFluidSpawn {
public:
    ModelFluidSpawnNode(const std::string& name, const std::shared_ptr<Model>& model);
    ModelFluidSpawnNode(const nlohmann::json json);

    FluidSpawn getFluidSpawn() override;

    NodeProperties buildProperties() override;

    std::string getTypeName() const override;

private:
    void init();

    FluidSpawn particleSpawn;
};

#endif //SPHLASH_MODELPARTICLESPAWNNODE_H
