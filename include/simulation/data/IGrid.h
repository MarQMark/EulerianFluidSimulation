#ifndef EULERIANFLUIDSIMULATION_IGRID_H
#define EULERIANFLUIDSIMULATION_IGRID_H
#include "StorageBuffer.h"

class VecField2D;
class ScalField2D;
class VecField3D;
class ScalField3D;

class IGrid {
public:
    unsigned int size() const { return  _size;}

protected:
    IGrid() = default;

    unsigned int _size;
};

class IGrid2D : public IGrid{
public:
    virtual ~IGrid2D() = default;

    unsigned int width() const {return _width;}
    unsigned int height() const {return _height;}

    virtual VecField2D& vel() = 0;
    virtual ScalField2D& rho() = 0;
    virtual ScalField2D& gamma() = 0;
    virtual ScalField2D& beta() = 0;
    virtual ScalField2D& divergence() = 0;
    virtual ScalField2D& pressure() = 0;
    virtual VecField2D& forces() = 0;
    virtual ScalField2D& boundary() = 0;

protected:
    IGrid2D(const unsigned int width, const unsigned int height)
        : IGrid(), _width(width), _height(height) {_size = width * height;}

    unsigned int _width;
    unsigned int _height;
};

class IGrid3D : public IGrid{
public:
    virtual ~IGrid3D() = default;

    unsigned int width() const {return _width;}
    unsigned int height() const {return _height;}
    unsigned int depth() const {return _depth;}

    virtual VecField3D& vel() = 0;
    virtual ScalField3D& rho() = 0;
    virtual ScalField3D& gamma() = 0;
    virtual ScalField3D& beta() = 0;
    virtual ScalField3D& divergence() = 0;
    virtual ScalField3D& pressure() = 0;
    virtual VecField3D& forces() = 0;
    virtual ScalField3D& boundary() = 0;

protected:
    IGrid3D(const unsigned int width, const unsigned int height, const unsigned int depth)
        : IGrid(), _width(width), _height(height), _depth(depth) {_size = width * height * depth;}

    unsigned int _width;
    unsigned int _height;
    unsigned int _depth;
};

#endif //EULERIANFLUIDSIMULATION_IGRID_H