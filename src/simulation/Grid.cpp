#include "simulation/Grid.h"

Grid::Grid(const unsigned int nWidth, const unsigned int nHeight) : _n_width(nWidth), _n_height(nHeight) {
    const unsigned int nCells = nWidth * nHeight;
    _v0.resize(nCells);
    _v1.resize(nCells);
    _s0.resize(nCells);
    _s1.resize(nCells);

    init();
}

std::vector<glm::vec2> & Grid::getVel() {
    if(_field == 0)
        return _v0;

    return _v1;
}

std::vector<glm::vec2> & Grid::getNewVel() {
    if(_field == 1)
        return _v0;

    return _v1;
}

std::vector<float> & Grid::getPres() {
    return _s0;
}

void Grid::swap() {
    _field = (_field + 1) % 2;
}

void Grid::init() {
    for(auto & i : _v0)
        i = glm::vec2(0);
    for(auto & i : _v1)
        i = glm::vec2(0);
    for(auto & i : _s0)
        i = 0;
    for(auto & i : _s1)
        i = 0;
}
