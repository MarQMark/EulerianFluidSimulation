#include "simulation/Grid.h"

Grid::Grid(const unsigned int nWidth, const unsigned int nHeight) : _n_width(nWidth), _n_height(nHeight) {
    const unsigned int nCells = nWidth * nHeight;
    _v0.resize(nCells);
    _v1.resize(nCells);
    _s0.resize(nCells);
    _s1.resize(nCells);
}
