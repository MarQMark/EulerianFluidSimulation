#ifndef EULERIANFLUIDSIMULATION_ISIMULATOR3D_H
#define EULERIANFLUIDSIMULATION_ISIMULATOR3D_H

#include "ISimulator.h"

class ISimulator3D : public ISimulator {
protected:
    ISimulator3D(const unsigned int width, const unsigned int height, const unsigned int depth)
        : _width(width), _height(height), _depth(depth) {}

    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _depth = 0;
};

#endif //EULERIANFLUIDSIMULATION_ISIMULATOR3D_H