#include "simulation/data/Grid2D.h"

#include <algorithm>

#include "simulation/Properties.h"

#define LOOP_2D(x, y, offset)  for (int y = offset; y < _height - offset; y++) \
                               for (int x = offset; x < _width  - offset; x++)


Grid2D::Grid2D(const unsigned int nWidth, const unsigned int nHeight)
    : IGrid2D(nWidth, nHeight) {
    _vel = new VecField2D(nWidth, nHeight);
    _v_tmp = new VecField2D(nWidth, nHeight);
    _rho = new ScalField2D(nWidth, nHeight);
    _s_tmp = new ScalField2D(nWidth, nHeight);
    _forces = new VecField2D(nWidth, nHeight);
    _boundary = new ScalField2D(nWidth, nHeight);

    _gamma = new ScalField2D(nWidth, nHeight);
    _beta = new ScalField2D(nWidth, nHeight);
    _A = new std::map<unsigned int, DisAdvCell>;

    _divergence = new ScalField2D(nWidth, nHeight);
    _pressure = new ScalField2D(nWidth, nHeight);

    *_gamma = 1.f;

    switch (PROPERTY(INIT_SHAPE, Properties::InitShape)) {
        case Properties::FULL:
            (*_rho) = 1.f;
            break;
        case Properties::HALF:
            for (int y = 0; y < _height / 2; y++)
            for (int x = 0; x < _width; x++)
                (*_rho)(x, y) = 1.f;
            break;
        case Properties::SQUARE:
            if (_width < 50 || _height < 50)
                break;
            for (int y = 40; y < 50; y++)
                for (int x = 40; x < 50; x++)
                    (*_rho)(x, y) = 1.f;
            break;
        case Properties::EMPTY:
            (*_rho) = 0.f;
            break;
    }

    LOOP_2D(x, y, 0) {
        if (y == 0) (*_rho)(x, y) = 0.f;
        if (x == 0) (*_rho)(x, y) = 0.f;
        if (x == _width  - 1) (*_rho)(x, y) = 0.f;
        if (y == _height - 1) (*_rho)(x, y) = 0.f;

        if (y == 0) (*_boundary)(x, y) = 1.f;
        if (x == 0) (*_boundary)(x, y) = 1.f;
        if (x == _width  - 1) (*_boundary)(x, y) = 1.f;
        if (y == _height - 1) (*_boundary)(x, y) = 1.f;
    }

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    LOOP_2D(x, y, 0) {
        ADD_PROPERTY_F(TOTAL_RHO, (*_rho)(x, y));
    }
}

Grid2D::~Grid2D() {
    delete _vel;
    delete _v_tmp;
    delete _rho;
    delete _s_tmp;
    delete _forces;
    delete _boundary;

    delete _gamma;
    delete _beta;
    delete _A;

    delete _divergence;
    delete _pressure;
}

glm::vec2 Grid2D::traceParticle(const glm::vec2 start, const float dt, const int substeps) const {
    auto& v   = *_vel;
    const auto& walls = *_boundary;
    const float W     = _width;
    const float H     = _height;

    auto inWall = [&](glm::vec2 p) -> bool {
        if (p.x < 1.f || p.x > W - 2.f || p.y < 1.f || p.y > H - 2.f)
            return true;
        int ix = std::clamp((int)p.x, 0, (int)W - 1);
        int iy = std::clamp((int)p.y, 0, (int)H - 1);
        return walls(ix, iy) > 0.5f;
    };

    auto clamp = [&](glm::vec2 p) -> glm::vec2 {
        return { std::clamp(p.x, 1.f, W - 2.f),
                 std::clamp(p.y, 1.f, H - 2.f) };
    };

    auto sampleVel = [&](glm::vec2 p) -> glm::vec2 {
        return v.interpolate(p - glm::vec2(0.5f));
    };

    if (inWall(start)) return start;

    const float subDt = dt / (float)substeps;
    glm::vec2 pos = start;

    for (int s = 0; s < substeps; s++) {
        glm::vec2 k1   = sampleVel(pos);
        glm::vec2 mid  = clamp(pos + k1 * (subDt * 0.5f));
        glm::vec2 k2   = sampleVel(mid);
        glm::vec2 next = pos + k2 * subDt;

        if (inWall(next)) {
            glm::vec2 safe   = pos;
            glm::vec2 unsafe = next;

            for (int b = 0; b < 8; b++) {
                glm::vec2 halfway = (safe + unsafe) * 0.5f;
                if (inWall(halfway))
                    unsafe = halfway;
                else
                    safe = halfway;
            }

            return safe;
        }

        pos = next;
    }

    return pos;
}

VecField2D & Grid2D::vel() {
    return *_vel;
}

VecField2D & Grid2D::tmpVec() const {
    return *_v_tmp;
}

void Grid2D::swapVec() {
    VecField2D* tmp = _vel;
    _vel = _v_tmp;
    _v_tmp = tmp;

    _v_tmp->clear();
}

ScalField2D & Grid2D::rho() {
    return *_rho;
}

ScalField2D & Grid2D::tmpScal() const {
    return *_s_tmp;
}

void Grid2D::swapRho() {
    ScalField2D* tmp = _rho;
    _rho = _s_tmp;
    _s_tmp = tmp;

    _s_tmp->clear();
}

ScalField2D & Grid2D::gamma() {
    return *_gamma;
}

ScalField2D & Grid2D::beta() {
    return *_beta;
}

std::map<unsigned int, Grid2D::DisAdvCell>& Grid2D::A() const {
    return *_A;
}

void Grid2D::addA(const unsigned int i, const unsigned int j, const float val) const {
    const unsigned int idx = i * size() + j;
    if (_A->contains(idx))
        (*_A)[idx] += {i, j, val};
    else
        (*_A)[idx] = {i, j, val};
}

ScalField2D & Grid2D::divergence() {
    return *_divergence;
}

ScalField2D & Grid2D::pressure() {
    return *_pressure;
}

VecField2D & Grid2D::forces() {
    return *_forces;
}

ScalField2D & Grid2D::boundary() {
    return *_boundary;
}

#undef LOOP_2D