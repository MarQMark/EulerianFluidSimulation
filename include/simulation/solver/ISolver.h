#ifndef EULERIANFLUIDSIMULATION_ISOLVER_H
#define EULERIANFLUIDSIMULATION_ISOLVER_H

class Grid3D;
class IGrid2D;

class ISolver2D {
protected:
    ISolver2D(const int width, const int height)
        : _width(width), _height(height), _size(width * height) {};

public:
    virtual ~ISolver2D() = default;

    virtual float solve(IGrid2D* grid) = 0;

protected:
    int _width = 0;
    int _height = 0;
    int _size = 0;
};

class ISolverCPU2D : public ISolver2D {
public:
    ISolverCPU2D(const int width, const int height) : ISolver2D(width, height) {}
};
class ISolverOpenGL2D : public ISolver2D {
public:
    ISolverOpenGL2D(const int width, const int height) : ISolver2D(width, height) {}
};

class ISolver3D {
protected:
    ISolver3D(const int width, const int height, const int depth)
        : _width(width), _height(height), _depth(depth), _size(width * height * depth) {};

public:
    virtual ~ISolver3D() = default;

    virtual float solve(Grid3D* grid) = 0;

protected:
    int _width = 0;
    int _height = 0;
    int _depth = 0;
    int _size = 0;
};

class ISolverCPU3D : public ISolver3D {
public:
    ISolverCPU3D(const int width, const int height, const int depth)
        : ISolver3D(width, height, depth) {}
};

#endif //EULERIANFLUIDSIMULATION_ISOLVER_H