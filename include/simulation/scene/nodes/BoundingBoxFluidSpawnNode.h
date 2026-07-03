//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
#define SPHLASH_BOUNDINGBOXPARTICLESPAWN_H

#include "BoundingBoxNode.h"
#include "simulation/scene/IFluidSpawn.h"

class BoundingBoxFluidSpawnNode : public BoundingBoxNode, public IFluidSpawn {
public:
    BoundingBoxFluidSpawnNode(const std::string& name);
    explicit BoundingBoxFluidSpawnNode(const nlohmann::json json);

    FluidSpawn getFluidSpawn() override;

    NodeProperties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    FluidSpawn _fluidSpawn;
    std::unique_ptr<Mesh> _mesh;
};

#endif //SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
