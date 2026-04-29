#include "simulation/Grid.h"

Grid::Grid(const unsigned int nWidth, const unsigned int nHeight) : _n_width(nWidth), _n_height(nHeight) {
    const unsigned int nCells = nWidth * nHeight;
    _v0 = new VecField(nWidth, nHeight);
    _v1 = new VecField(nWidth, nHeight);
    _s0 = new ScalField(nWidth, nHeight);
    _s1 = new ScalField(nWidth, nHeight);

    init();
}

Grid::~Grid() {
    delete _v0;
    delete _v1;
    delete _s0;
    delete _s1;
}

VecField& Grid::getVel() const {
    if(_vel == 0)
        return *_v0;

    return *_v1;
}

VecField& Grid::getNewVel() const {
    if(_vel == 1)
        return *_v0;

    return *_v1;
}

ScalField& Grid::getPres() {
    if(_pres == 0)
        return *_s0;

    return *_s1;
}

ScalField& Grid::getNewPres() {
    if(_pres == 1)
        return *_s0;

    return *_s1;
}

void Grid::swapVel() {
    _vel = (_vel + 1) % 2;
}

void Grid::swapPres() {
    _pres = (_pres + 1) % 2;
}


void Grid::init() {
    _v0->clear();
    _v1->clear();
    for(auto & i : *_s0)
        i = 0;
    for(auto & i : *_s1)
        i = 0;
}
