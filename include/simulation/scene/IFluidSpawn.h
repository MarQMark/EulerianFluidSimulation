//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_IFLUIDSPAWN_H
#define SPHLASH_IFLUIDSPAWN_H

struct Triangle;

struct FluidSpawn {
    AABB area = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    };

    std::vector<Triangle> shape = {};
};

class IFluidSpawn {
public:
    virtual ~IFluidSpawn() = default;
    virtual FluidSpawn getFluidSpawn() = 0;
};

#endif //SPHLASH_IPARTICLESPAWN_H
