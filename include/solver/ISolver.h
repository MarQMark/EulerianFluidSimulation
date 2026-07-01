#ifndef EULERIANFLUIDSIMULATION_ISOLVER_H
#define EULERIANFLUIDSIMULATION_ISOLVER_H

class Grid2D;

class ISolver {
protected:
    ISolver(const int width, const int height)
        : _width(width), _height(height), _size(width * height) {};

public:
    virtual ~ISolver() = default;

    virtual float solve(Grid2D* grid) = 0;

protected:
    int _width = 0;
    int _height = 0;
    int _size = 0;
};

#endif //EULERIANFLUIDSIMULATION_ISOLVER_H