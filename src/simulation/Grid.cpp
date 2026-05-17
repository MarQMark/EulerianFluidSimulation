#include "simulation/Grid.h"

Grid::Grid(const unsigned int nWidth, const unsigned int nHeight) : _n_width(nWidth), _n_height(nHeight) {
    _vel = new VecField(nWidth, nHeight);
    _v_tmp = new VecField(nWidth, nHeight);
    _rho = new ScalField(nWidth, nHeight);
    _s_tmp = new ScalField(nWidth, nHeight);
    _forces = new VecField(nWidth, nHeight);
    _boundary = new ScalField(nWidth, nHeight);
    _gamma = new ScalField(nWidth, nHeight);
    _divergence = new ScalField(nWidth, nHeight);
    _pressure = new ScalField(nWidth, nHeight);
}

Grid::~Grid() {
    delete _vel;
    delete _v_tmp;
    delete _rho;
    delete _s_tmp;
    delete _forces;
    delete _boundary;
    delete _gamma;
    delete _divergence;
    delete _pressure;
}

VecField & Grid::vel() const {
    return *_vel;
}

VecField & Grid::tmpVec() const {
    return *_v_tmp;
}

void Grid::swapVec() {
    VecField* tmp = _vel;
    _vel = _v_tmp;
    _v_tmp = tmp;

    _v_tmp->clear();
}

ScalField & Grid::rho() const {
    return *_rho;
}

ScalField & Grid::tmpScal() const {
    return *_s_tmp;
}

void Grid::swapRho() {
    ScalField* tmp = _rho;
    _rho = _s_tmp;
    _s_tmp = tmp;

    _s_tmp->clear();
}

ScalField & Grid::gamma() const {
    return *_gamma;
}

ScalField & Grid::divergence() const {
    return *_divergence;
}

ScalField & Grid::pressure() const {
    return *_pressure;
}

VecField & Grid::forces() const {
    return *_forces;
}

ScalField & Grid::boundary() const {
    return *_boundary;
}

unsigned int Grid::width() const {
    return _n_width;
}

unsigned int Grid::height() const {
    return _n_height;
}
