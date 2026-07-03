#include "simulation/Grid3D.h"
#include <algorithm>
#include "simulation/Properties.h"

#define LOOP_3D(x, y, z, offset)                        \
    for (int z = offset; z < _n_depth  - offset; z++)   \
    for (int y = offset; y < _n_height - offset; y++)   \
    for (int x = offset; x < _n_width  - offset; x++)   \

#ifdef USE_GPU
void Grid3D::gpuUpload() const {
    gpuVel->upload(_vel->data());
    gpuRho->upload(_rho->data());
    gpuBoundary->upload(_boundary->data());
    gpuForces->upload(_forces->data());
}

void Grid3D::gpuDownload() const {
    gpuVel->download(_vel->data());
}
#endif

Grid3D::Grid3D(const unsigned int nWidth, const unsigned int nHeight, const unsigned int nDepth)
    : _n_width(nWidth), _n_height(nHeight), _n_depth(nDepth), _size(nWidth * nHeight * nDepth) {
    _vel = new VecField3D(nWidth, nHeight, nDepth);
    _v_tmp = new VecField3D(nWidth, nHeight, nDepth);
    _rho = new ScalField3D(nWidth, nHeight, nDepth);
    _s_tmp = new ScalField3D(nWidth, nHeight, nDepth);
    _forces = new VecField3D(nWidth, nHeight, nDepth);
    _boundary = new ScalField3D(nWidth, nHeight, nDepth);

    _gamma = new ScalField3D(nWidth, nHeight, nDepth);
    _beta = new ScalField3D(nWidth, nHeight, nDepth);
    // How cares about RAM, just download more if it's not enough
    //_A = new std::vector<DisAdvCell>();//(nWidth * nHeight * nWidth * nHeight);
    _A = new std::unordered_map<unsigned int, DisAdvCell>;

    _divergence = new ScalField3D(nWidth, nHeight, nDepth);
    _pressure = new ScalField3D(nWidth, nHeight, nDepth);

    *_gamma = 1.f;

#ifdef USE_GPU
    gpuVel        = new StorageBuffer(_size * sizeof(glm::vec3));
    gpuTmpVel     = new StorageBuffer(_size * sizeof(glm::vec3));

    gpuRho        = new StorageBuffer(_size * sizeof(float));
    gpuTmpRho     = new StorageBuffer(_size * sizeof(float));
    gpuGamma      = new StorageBuffer(_size * sizeof(float));
    gpuBeta       = new StorageBuffer(_size * sizeof(float));

    gpuDivergence = new StorageBuffer(_size * sizeof(float));
    gpuPressure   = new StorageBuffer(_size * sizeof(float));

    gpuForces     = new StorageBuffer(_size * sizeof(glm::vec3));
    gpuBoundary   = new StorageBuffer(_size * sizeof(float));
#endif

    // switch (PROPERTY(INIT_SHAPE, Properties::InitShape)) {
    //     case Properties::FULL:
    //         (*_rho) = 1.f;
    //         break;
    //     case Properties::HALF:
    //         for (int y = 0; y < _n_height / 2; y++)
    //         for (int x = 0; x < _n_width; x++)
    //             (*_rho)(x, y) = 1.f;
    //         break;
    //     case Properties::SQUARE:
    //         if (_n_width < 50 || _n_height < 50)
    //             break;
    //         for (int y = 40; y < 50; y++)
    //             for (int x = 40; x < 50; x++)
    //                 (*_rho)(x, y) = 1.f;
    //         break;
    //     case Properties::EMPTY:
    //         (*_rho) = 0.f;
    //         break;
    // }

    LOOP_3D(x, y, z, 0) {
        if (z == 0) (*_rho)(x, y, z) = 0.f;
        if (y == 0) (*_rho)(x, y, z) = 0.f;
        if (x == 0) (*_rho)(x, y, z) = 0.f;
        if (x == _n_width  - 1) (*_rho)(x, y, z) = 0.f;
        if (y == _n_height - 1) (*_rho)(x, y, z) = 0.f;
        if (z == _n_depth  - 1) (*_rho)(x, y, z) = 0.f;

        if (z == 0) (*_boundary)(x, y, z) = 1.f;
        if (y == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == _n_width  - 1) (*_boundary)(x, y, z) = 1.f;
        if (y == _n_height - 1) (*_boundary)(x, y, z) = 1.f;
        if (z == _n_depth  - 1) (*_boundary)(x, y, z) = 1.f;
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

#ifdef USE_GPU
    delete gpuVel;
    delete gpuTmpVel;
    delete gpuRho;
    delete gpuTmpRho;
    delete gpuGamma;
    delete gpuBeta;
    delete gpuDivergence;
    delete gpuPressure;
    delete gpuForces;
    delete gpuBoundary;
#endif
}

glm::vec3 Grid3D::traceParticle(const glm::vec3 start, const float dt, const int substeps) const {
// Core tracer — used by both rho_advect and vel_advect.
// start:    cell-index position (rho lives at cell centers, so pass glm::vec2(x, y))
// dt:       signed timestep (negative = backward trace, positive = forward)
// substeps: 1 = single Euler step (original), 4-8 = accurate with wall stopping
    auto& vel   = this->vel();
    const auto& walls = this->boundary();
    const float W     = (float)this->width();
    const float H     = (float)this->height();
    const float D     = (float)this->depth();

    // Is continuous position p inside a wall or out of domain?
    // Checks the cell that contains p (floor) plus clamps to domain.
    auto inWall = [&](glm::vec3 p) -> bool {
        // Out of domain counts as wall — never leave the valid area
        if (p.x < 1.f || p.x > W - 2.f || p.y < 1.f || p.y > H - 2.f || p.z < 1.f || p.z > D - 2.f)
            return true;
        int ix = std::clamp((int)p.x, 0, (int)W - 1);
        int iy = std::clamp((int)p.y, 0, (int)H - 1);
        int iz = std::clamp((int)p.z, 0, (int)D - 1);
        return walls(ix, iy, iz) > 0.5f;
    };

    // Clamp to valid domain range (inside border cells)
    auto clamp = [&](glm::vec3 p) -> glm::vec3 {
        return { std::clamp(p.x, 1.f, W - 2.f),
                 std::clamp(p.y, 1.f, H - 2.f),
                 std::clamp(p.z, 1.f, D - 2.f) };
    };

    // Sample velocity at a cell-center position.
    // vel is face-centered so we offset by -0.5 to convert.
    auto sampleVel = [&](glm::vec3 p) -> glm::vec3 {
        return vel.interpolate(p - glm::vec3(0.5f));
    };

    // If start is already in a wall (shouldn't happen, but be safe)
    // return start unchanged.
    if (inWall(start)) return start;

    const float subDt = dt / (float)substeps;
    glm::vec3 pos = start;

    for (int s = 0; s < substeps; s++) {
        // ---- RK2 (midpoint method) ----
        // k1: velocity at current position
        glm::vec3 k1   = sampleVel(pos);
        // Midpoint: half a substep ahead
        glm::vec3 mid  = clamp(pos + k1 * (subDt * 0.5f));
        // k2: velocity at midpoint (better estimate of average velocity)
        glm::vec3 k2   = sampleVel(mid);
        // Full substep using the midpoint velocity
        glm::vec3 next = pos + k2 * subDt;

        // ---- Wall detection ----
        if (inWall(next)) {
            // Binary search: find the last safe position before the wall.
            // After 8 iterations the interval is 1/256 of the substep size.
            glm::vec3 safe   = pos;   // known to be outside wall
            glm::vec3 unsafe = next;  // known to be inside wall

            for (int b = 0; b < 8; b++) {
                glm::vec3 halfway = (safe + unsafe) * 0.5f;
                if (inWall(halfway))
                    unsafe = halfway;  // still inside, move unsafe closer
                else
                    safe = halfway;    // now outside, move safe closer
            }

            // Stop here — do not continue substeps past a wall
            return safe;
        }

        // No wall hit — accept the substep
        pos = next;
    }

    return pos;
}

VecField3D & Grid3D::vel() const {
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

ScalField3D & Grid3D::rho() const {
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

ScalField3D & Grid3D::gamma() const {
    return *_gamma;
}

ScalField3D & Grid3D::beta() const {
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

ScalField3D & Grid3D::divergence() const {
    return *_divergence;
}

ScalField3D & Grid3D::pressure() const {
    return *_pressure;
}

VecField3D & Grid3D::forces() const {
    return *_forces;
}

ScalField3D & Grid3D::boundary() const {
    return *_boundary;
}

unsigned int Grid3D::width() const {
    return _n_width;
}

unsigned int Grid3D::height() const {
    return _n_height;
}

unsigned int Grid3D::depth() const {
    return _n_depth;
}

unsigned int Grid3D::size() const {
    return _size;
}

#undef LOOP_2D