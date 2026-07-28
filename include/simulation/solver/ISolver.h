#ifndef EULERIANFLUIDSIMULATION_ISOLVER_H
#define EULERIANFLUIDSIMULATION_ISOLVER_H

class IGrid3D;
class IGrid2D;

class ISolver2D {
protected:
    ISolver2D(const unsigned int width, const unsigned int height)
        : _width(width), _height(height), _size(width * height) {};

public:
    virtual ~ISolver2D() = default;

    virtual float solve(IGrid2D* grid) = 0;

protected:
    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _size = 0;
};

class ISolverCPU2D : public ISolver2D {
protected:
    ISolverCPU2D(const unsigned int width, const unsigned int height) : ISolver2D(width, height) {}
};
class ISolverOpenGL2D : public ISolver2D {
protected:
    ISolverOpenGL2D(const unsigned int width, const unsigned int height) : ISolver2D(width, height) {}
};

class ISolver3D {
protected:
    ISolver3D(const unsigned int width, const unsigned int height, const unsigned int depth)
        : _width(width), _height(height), _depth(depth), _size(width * height * depth) {};

public:
    virtual ~ISolver3D() = default;

    virtual float solve(IGrid3D* grid) = 0;

protected:
    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _depth = 0;
    unsigned int _size = 0;
};

class ISolverCPU3D : public ISolver3D {
public:
    ISolverCPU3D(const unsigned int width, const unsigned int height, const unsigned int depth)
        : ISolver3D(width, height, depth) {}
};
class ISolverOpenGL3D : public ISolver3D {
protected:
    ISolverOpenGL3D(const unsigned int width, const unsigned int height, const unsigned int depth)
        : ISolver3D(width, height, depth) {}
};

#endif //EULERIANFLUIDSIMULATION_ISOLVER_H