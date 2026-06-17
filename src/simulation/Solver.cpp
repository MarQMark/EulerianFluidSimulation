#include "simulation/Solver.h"

#include "simulation/Properties.h"

Solver::Solver(const int width, const int height) : _width(width), _height(height) {
    _size = _width * _height;
}

CGSolver::CGSolver(const int width, const int height) : Solver(width, height) {
    _t = new ScalField(width, height);
    _tmp = new ScalField(width, height);
    _r = new ScalField(width, height);
    _d = new ScalField(width, height);
}

CGSolver::~CGSolver() {
    delete _t;
    delete _tmp;
    delete _r;
    delete _d;
}

float CGSolver::solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) {
    const int steps = PROPERTY_I(MAX_STEPS);
    const float errorThreshold = PROPERTY_F(SOLVER_THRESHOLD);

    auto& t = *_t; t.clear();
    auto& tmp = *_tmp; tmp.clear();
    auto& r = *_r; r = divergence;

    pressure = divergence;
    impl_mat_mul(t, pressure, rho, boundary);
    r -= t;
    auto& d = *_d; d = r;

    float sqrtLen = r.sqLen();
    if (sqrtLen < errorThreshold)
        return sqrtLen;

    for (int i = 0; i < steps; i++) {
        impl_mat_mul(t, d, rho, boundary);
        const float alpha = sqrtLen / d.dot(t);
        pressure.addScaled(d, alpha);

        if (i & 0x3F) {
            r.addScaled(t, -alpha);
        }
        else {
            impl_mat_mul(t, pressure, rho, boundary);
            r = divergence;
            r -= t;
        }

        const float oldSqrtLen = sqrtLen;
        sqrtLen = r.sqLen();
        if (sqrtLen < errorThreshold)
            return sqrtLen;

        const float beta = sqrtLen / oldSqrtLen;
        d *= beta;
        d += r;
    }

    return sqrtLen;
}

void CGSolver::impl_mat_mul(ScalField &result, const ScalField &x, const ScalField &rho, const ScalField &boundary) const {
    for (int y = 0; y < _height; y++) {
        for (int x_ = 0; x_ < _width; x_++) {

            // Air cell OR boundary of domain: p = 0, identity row
            if (rho(x_, y) <= 0.5f || boundary(x_, y) == 1) {
                result(x_, y) = x(x_, y);
                continue;
            }

            // Fluid cell (rho > 0.5): build Laplacian row
            float sum    = 0.0f;
            int   nCount = 0;

            auto sampleNeighbor = [&](const int nx, const int ny) {
                // Out of bounds: already Neumann (never reached with border cells in domain)
                if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) return;

                // Border cells are solid walls → Neumann (dp/dn = 0): exclude entirely
                // Previously these were treated as air (Dirichlet p=0) which inflated
                // the diagonal and caused the pressure to be too small to cancel divergence
                if (nx == 0 || nx == _width-1 || ny == 0 || ny == _height-1) return;
                if (boundary(nx, ny) > 0.5f) return;

                nCount++;
                if (rho(nx, ny) > 0.5f)
                    sum += x(nx, ny);
                // Interior air cell: Dirichlet p=0 (correct — counts in nCount, not sum)
            };

            sampleNeighbor(x_ - 1, y);
            sampleNeighbor(x_ + 1, y);
            sampleNeighbor(x_, y - 1);
            sampleNeighbor(x_, y + 1);

            result(x_, y) = sum - (float)nCount * x(x_, y);
        }
    }
}

using SpMat = Eigen::SparseMatrix<float, Eigen::RowMajor>;
using VecXf = Eigen::VectorXf;

EigenSolver::~EigenSolver() {
    delete _A;
    delete _b;
    delete _x0;
}

EigenSolver::EigenSolver(const int width, const int height) : Solver(width, height) {
    _A = new Eigen::SparseMatrix<float, Eigen::RowMajor>(_size, _size);
    _b = new Eigen::VectorXf(_size);
    _x0 = new Eigen::VectorXf(_size);
}

void EigenSolver::determine_equations(const ScalField &divergence, const ScalField &rho, const ScalField &boundary) const {
    auto idx = [&](const int x, const int y){ return y * _width + x; };

    // ── Build sparse matrix ──────────────────────────────────────────────────
    // Reserve 5 slots per row (diagonal + 4 neighbours).
    std::vector<Eigen::Triplet<float>> triplets;
    triplets.reserve(_size * 5);
    auto& b = *_b;
    b.setZero();

    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            const int i = idx(x, y);

            // Air, wall, or domain border → identity row, p = 0
            const bool liquid = rho(x, y)      >  0.5f
                             && boundary(x, y)  <  0.5f
                             && x > 0 && x < _width-1
                             && y > 0 && y < _height-1;

            if (!liquid) {
                triplets.emplace_back(i, i, 1.f);
                // b[i] already 0 — Dirichlet p = 0 for air/wall
                continue;
            }

            // Liquid cell — positive-definite form: +nCount on diagonal, -1 off-diagonal
            int nCount = 0;

            auto addNeighbour = [&](const int nx, const int ny) {
                if (nx < 0 || nx >= _width || ny < 0 || ny >= _height) return;
                if (nx == 0 || nx == _width-1 || ny == 0 || ny == _height-1) return; // Neumann border
                if (boundary(nx, ny) > 0.5f) return;                             // Neumann wall

                nCount++;
                if (rho(nx, ny) > 0.5f)
                    triplets.emplace_back(i, idx(nx, ny), -1.f); // liquid neighbour
                // Air neighbour: Dirichlet p=0, counted in nCount but no off-diagonal
            };

            addNeighbour(x-1, y); addNeighbour(x+1, y);
            addNeighbour(x, y-1); addNeighbour(x, y+1);

            triplets.emplace_back(i, i, (float)nCount); // +nCount diagonal
            b[i] = -divergence(x, y);                     // negated RHS
        }
    }

    auto& A = *_A;
    A.setFromTriplets(triplets.begin(), triplets.end());
}

EigenCGSolver::EigenCGSolver(const int width, const int height) : EigenSolver(width, height){
}

float EigenCGSolver::solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) {
    const int steps = PROPERTY_I(MAX_STEPS);
    const float errorThreshold = PROPERTY_F(SOLVER_THRESHOLD);

    determine_equations(divergence, rho, boundary);

    Eigen::ConjugateGradient<SpMat, Eigen::Lower | Eigen::Upper, Eigen::DiagonalPreconditioner<float>> solver;

    const auto& A = *_A;
    const auto& b = *_b;

    solver.setMaxIterations(steps);
    solver.setTolerance(errorThreshold);
    solver.compute(A);

    // Warm start: previous frame's pressure is a good initial guess
    auto& x0 = *_x0;
    for (int i = 0; i < _size; i++)
        x0[i] = pressure[i];

    VecXf sol = solver.solveWithGuess(b, x0);

    for (int i = 0; i < _size; i++)
        pressure[i] = sol[i];

    return (float)solver.error();
}

EigenBiCGSTABSolver::EigenBiCGSTABSolver(const int width, const int height) : EigenSolver(width, height) {
}

float EigenBiCGSTABSolver::solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) {
    const int steps = PROPERTY_I(MAX_STEPS);
    const float errorThreshold = PROPERTY_F(SOLVER_THRESHOLD);

    determine_equations(divergence, rho, boundary);

    Eigen::BiCGSTAB<SpMat, Eigen::DiagonalPreconditioner<float>> solver;

    const auto& A = *_A;
    const auto& b = *_b;

    solver.setMaxIterations(steps);
    solver.setTolerance(errorThreshold);
    solver.compute(A);

    // Warm start: previous frame's pressure is a good initial guess
    auto& x0 = *_x0;
    for (int i = 0; i < _size; i++)
        x0[i] = pressure[i];

    VecXf sol = solver.solveWithGuess(b, x0);

    for (int i = 0; i < _size; i++)
        pressure[i] = sol[i];

    return (float)solver.error();
}
