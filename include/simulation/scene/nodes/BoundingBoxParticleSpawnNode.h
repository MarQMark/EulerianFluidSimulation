//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
#define SPHLASH_BOUNDINGBOXPARTICLESPAWN_H

#include "BoundingBoxNode.h"
#include "simulation/scene/IParticleSpawn.h"

class BoundingBoxParticleSpawnNode : public BoundingBoxNode, public IParticleSpawn {
public:
    BoundingBoxParticleSpawnNode(const std::string& name);
    explicit BoundingBoxParticleSpawnNode(const nlohmann::json json);

    ParticleSpawn getParticleSpawn() override;

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    ParticleSpawn particleSpawn;
};

#endif //SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
