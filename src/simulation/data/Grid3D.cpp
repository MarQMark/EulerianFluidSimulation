#include "simulation/data/Grid3D.h"
#include <algorithm>
#include "simulation/Properties.h"

#define LOOP_3D(x, y, z, offset)                        \
    for (int z = offset; z < _depth  - offset; z++)   \
    for (int y = offset; y < _height - offset; y++)   \
    for (int x = offset; x < _width  - offset; x++)   \

Grid3D::Grid3D(const unsigned int nWidth, const unsigned int nHeight, const unsigned int nDepth)
    : IGrid3D(nWidth, nHeight, nDepth) {
    _vel = new VecField3D(nWidth, nHeight, nDepth);
    _v_tmp = new VecField3D(nWidth, nHeight, nDepth);
    _rho = new ScalField3D(nWidth, nHeight, nDepth);
    _s_tmp = new ScalField3D(nWidth, nHeight, nDepth);
    _forces = new VecField3D(nWidth, nHeight, nDepth);
    _boundary = new ScalField3D(nWidth, nHeight, nDepth);

    _gamma = new ScalField3D(nWidth, nHeight, nDepth);
    _beta = new ScalField3D(nWidth, nHeight, nDepth);
    _A = new std::unordered_map<unsigned int, DisAdvCell>;

    _divergence = new ScalField3D(nWidth, nHeight, nDepth);
    _pressure = new ScalField3D(nWidth, nHeight, nDepth);

    *_gamma = 1.f;

    LOOP_3D(x, y, z, 0) {
        if (z == 0) (*_rho)(x, y, z) = 0.f;
        if (y == 0) (*_rho)(x, y, z) = 0.f;
        if (x == 0) (*_rho)(x, y, z) = 0.f;
        if (x == _width  - 1) (*_rho)(x, y, z) = 0.f;
        if (y == _height - 1) (*_rho)(x, y, z) = 0.f;
        if (z == _depth  - 1) (*_rho)(x, y, z) = 0.f;

        if (z == 0) (*_boundary)(x, y, z) = 1.f;
        if (y == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == _width  - 1) (*_boundary)(x, y, z) = 1.f;
        if (y == _height - 1) (*_boundary)(x, y, z) = 1.f;
        if (z == _depth  - 1) (*_boundary)(x, y, z) = 1.f;
    }

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    LOOP_3D(x, y, z, 0) {
        ADD_PROPERTY_F(TOTAL_RHO, (*_rho)(x, y, z));
    }
}

Grid3D::~Grid3D() {
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

glm::vec3 Grid3D::traceParticle(const glm::vec3 start, const float dt, const int substeps) const {
    auto& vel   = *_vel;
    const auto& walls = *_boundary;
    const float W     = _width;
    const float H     = _height;
    const float D     = _depth;

    auto inWall = [&](glm::vec3 p) -> bool {
        if (p.x < 1.f || p.x > W - 2.f || p.y < 1.f || p.y > H - 2.f || p.z < 1.f || p.z > D - 2.f)
            return true;
        int ix = std::clamp((int)p.x, 0, (int)W - 1);
        int iy = std::clamp((int)p.y, 0, (int)H - 1);
        int iz = std::clamp((int)p.z, 0, (int)D - 1);
        return walls(ix, iy, iz) > 0.5f;
    };

    auto clamp = [&](glm::vec3 p) -> glm::vec3 {
        return { std::clamp(p.x, 1.f, W - 2.f),
                 std::clamp(p.y, 1.f, H - 2.f),
                 std::clamp(p.z, 1.f, D - 2.f) };
    };

    auto sampleVel = [&](glm::vec3 p) -> glm::vec3 {
        return vel.interpolate(p - glm::vec3(0.5f));
    };

    if (inWall(start)) return start;

    const float subDt = dt / (float)substeps;
    glm::vec3 pos = start;

    for (int s = 0; s < substeps; s++) {
        glm::vec3 k1   = sampleVel(pos);
        glm::vec3 mid  = clamp(pos + k1 * (subDt * 0.5f));
        glm::vec3 k2   = sampleVel(mid);
        glm::vec3 next = pos + k2 * subDt;

        if (inWall(next)) {
            glm::vec3 safe   = pos;
            glm::vec3 unsafe = next;

            for (int b = 0; b < 8; b++) {
                glm::vec3 halfway = (safe + unsafe) * 0.5f;
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

VecField3D & Grid3D::vel() {
    return *_vel;
}

VecField3D & Grid3D::tmpVec() const {
    return *_v_tmp;
}

void Grid3D::swapVec() {
    VecField3D* tmp = _vel;
    _vel = _v_tmp;
    _v_tmp = tmp;

    _v_tmp->clear();
}

ScalField3D & Grid3D::rho() {
    return *_rho;
}

ScalField3D & Grid3D::tmpScal() const {
    return *_s_tmp;
}

void Grid3D::swapRho() {
    ScalField3D* tmp = _rho;
    _rho = _s_tmp;
    _s_tmp = tmp;

    _s_tmp->clear();
}

ScalField3D & Grid3D::gamma() {
    return *_gamma;
}

ScalField3D & Grid3D::beta() {
    return *_beta;
}

std::unordered_map<unsigned int, Grid3D::DisAdvCell>& Grid3D::A() const {
    return *_A;
}

void Grid3D::addA(const unsigned int i, const unsigned int j, const float val) const {
    const unsigned int idx = i * size() + j;
    if (_A->contains(idx))
        (*_A)[idx] += {i, j, val};
    else
        (*_A)[idx] = {i, j, val};
}

ScalField3D & Grid3D::divergence() {
    return *_divergence;
}

ScalField3D & Grid3D::pressure() {
    return *_pressure;
}

VecField3D & Grid3D::forces() {
    return *_forces;
}

ScalField3D & Grid3D::boundary() {
    return *_boundary;
}

#undef LOOP_3D