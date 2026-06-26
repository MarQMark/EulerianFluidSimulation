//
// Created by ruut on 6/11/26.
//

#ifndef SPHLASH_MODELPARTICLESPAWNNODE_H
#define SPHLASH_MODELPARTICLESPAWNNODE_H

#include "assets/Model.h"
#include "ModelNode.h"
#include "simulation/scene/IParticleSpawn.h"
#include "SceneNode.h"

class ModelParticleSpawnNode : public ModelNode, public IParticleSpawn {
public:
    ModelParticleSpawnNode(const std::string& name, const std::shared_ptr<Model>& model);
    ModelParticleSpawnNode(const nlohmann::json json);

    ParticleSpawn getParticleSpawn() override;

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    void init();

    ParticleSpawn particleSpawn;
};

#endif //SPHLASH_MODELPARTICLESPAWNNODE_H
