#ifndef EULERIANFLUIDSIMULATION_ISIMULATOR2D_H
#define EULERIANFLUIDSIMULATION_ISIMULATOR2D_H

#include "ISimulator.h"

class ISimulator2D : public ISimulator {
protected:
    ISimulator2D(const unsigned int width, const unsigned int height)
        : _width(width), _height(height){}

    unsigned int _width = 0;
    unsigned int _height = 0;
};

#endif //EULERIANFLUIDSIMULATION_ISIMULATOR2D_H