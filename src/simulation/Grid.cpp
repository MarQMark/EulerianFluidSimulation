#include "simulation/Grid.h"

#include <algorithm>

Grid::Grid(const unsigned int nWidth, const unsigned int nHeight) : _n_width(nWidth), _n_height(nHeight) {
    _vel = new VecField(nWidth, nHeight);
    _v_tmp = new VecField(nWidth, nHeight);
    _rho = new ScalField(nWidth, nHeight);
    _s_tmp = new ScalField(nWidth, nHeight);
    _forces = new VecField(nWidth, nHeight);
    _boundary = new ScalField(nWidth, nHeight);

    _gamma = new ScalField(nWidth, nHeight);
    _beta = new ScalField(nWidth, nHeight);
    // How cares about RAM, just download more if it's not enough
    //_A = new std::vector<DisAdvCell>();//(nWidth * nHeight * nWidth * nHeight);
    _A = new std::map<unsigned int, DisAdvCell>;

    _divergence = new ScalField(nWidth, nHeight);
    _pressure = new ScalField(nWidth, nHeight);

    *_gamma = 1.f;
}

Grid::~Grid() {
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

glm::vec2 Grid::traceParticle(const glm::vec2 start, const float dt, const int substeps) const {
// Core tracer — used by both rho_advect and vel_advect.
// start:    cell-index position (rho lives at cell centers, so pass glm::vec2(x, y))
// dt:       signed timestep (negative = backward trace, positive = forward)
// substeps: 1 = single Euler step (original), 4-8 = accurate with wall stopping
    auto& vel   = this->vel();
    const auto& walls = this->boundary();
    const float W     = (float)this->width();
    const float H     = (float)this->height();

    // Is continuous position p inside a wall or out of domain?
    // Checks the cell that contains p (floor) plus clamps to domain.
    auto inWall = [&](glm::vec2 p) -> bool {
        // Out of domain counts as wall — never leave the valid area
        if (p.x < 1.f || p.x > W - 2.f || p.y < 1.f || p.y > H - 2.f)
            return true;
        int ix = std::clamp((int)p.x, 0, (int)W - 1);
        int iy = std::clamp((int)p.y, 0, (int)H - 1);
        return walls(ix, iy) > 0.5f;
    };

    // Clamp to valid domain range (inside border cells)
    auto clamp = [&](glm::vec2 p) -> glm::vec2 {
        return { std::clamp(p.x, 1.f, W - 2.f),
                 std::clamp(p.y, 1.f, H - 2.f) };
    };

    // Sample velocity at a cell-center position.
    // vel is face-centered so we offset by -0.5 to convert.
    auto sampleVel = [&](glm::vec2 p) -> glm::vec2 {
        return vel.interpolate(p - glm::vec2(0.5f));
    };

    // If start is already in a wall (shouldn't happen, but be safe)
    // return start unchanged.
    if (inWall(start)) return start;

    const float subDt = dt / (float)substeps;
    glm::vec2 pos = start;

    for (int s = 0; s < substeps; s++) {
        // ---- RK2 (midpoint method) ----
        // k1: velocity at current position
        glm::vec2 k1   = sampleVel(pos);
        // Midpoint: half a substep ahead
        glm::vec2 mid  = clamp(pos + k1 * (subDt * 0.5f));
        // k2: velocity at midpoint (better estimate of average velocity)
        glm::vec2 k2   = sampleVel(mid);
        // Full substep using the midpoint velocity
        glm::vec2 next = pos + k2 * subDt;

        // ---- Wall detection ----
        if (inWall(next)) {
            // Binary search: find the last safe position before the wall.
            // After 8 iterations the interval is 1/256 of the substep size.
            glm::vec2 safe   = pos;   // known to be outside wall
            glm::vec2 unsafe = next;  // known to be inside wall

            for (int b = 0; b < 8; b++) {
                glm::vec2 halfway = (safe + unsafe) * 0.5f;
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

ScalField & Grid::beta() const {
    return *_beta;
}

std::map<unsigned int, Grid::DisAdvCell>& Grid::A() const {
    return *_A;
}

void Grid::addA(const unsigned int i, const unsigned int j, const float val) const {
    const unsigned int idx = i * size() + j;
    if (_A->contains(idx))
        (*_A)[idx] += {i, j, val};
    else
        (*_A)[idx] = {i, j, val};
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

unsigned int Grid::size() const {
    return _n_height * _n_width;
}
