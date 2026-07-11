#include "simulation/solver/EigenSolver.h"

#include "simulation/data/Grid2D.h"
#include "simulation/Properties.h"

using SpMat = Eigen::SparseMatrix<float, Eigen::RowMajor>;
using VecXf = Eigen::VectorXf;

EigenSolver2D::EigenSolver2D(const int width, const int height) : ISolverCPU2D(width, height) {
    _A  = new Eigen::SparseMatrix<float, Eigen::RowMajor>(_size, _size);
    _b  = new Eigen::VectorXf(_size);
    _x0 = new Eigen::VectorXf(_size);
}

EigenSolver2D::~EigenSolver2D() { delete _A; delete _b; delete _x0; }

void EigenSolver2D::determine_equations(const ScalField2D &divergence, const ScalField2D &rho, const ScalField2D &boundary) const
{
    auto idx = [&](int x, int y){ return y * _width + x; };
    std::vector<Eigen::Triplet<float>> triplets;
    triplets.reserve(_size * 5);
    _b->setZero();

    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            const int i = idx(x, y);
            const bool liquid = rho(x,y) > 0.5f && boundary(x,y) < 0.5f
                             && x > 0 && x < _width-1 && y > 0 && y < _height-1;
            if (!liquid) { triplets.emplace_back(i, i, 1.f); continue; }

            int nCount = 0;
            auto addN = [&](int nx, int ny) {
                if (nx<0||nx>=_width||ny<0||ny>=_height) return;
                if (nx==0||nx==_width-1||ny==0||ny==_height-1) return;
                if (boundary(nx,ny) > 0.5f) return;
                nCount++;
                if (rho(nx,ny) > 0.5f) triplets.emplace_back(i, idx(nx,ny), -1.f);
            };
            addN(x-1,y); addN(x+1,y); addN(x,y-1); addN(x,y+1);
            triplets.emplace_back(i, i, (float)nCount);
            (*_b)[i] = -divergence(x, y);
        }
    }
    _A->setFromTriplets(triplets.begin(), triplets.end());
}

EigenCGSolver2D::EigenCGSolver2D(const int w, const int h) : EigenSolver2D(w, h) {}
float EigenCGSolver2D::solve(IGrid2D* _grid) {
    auto* grid = dynamic_cast<Grid2D*>(_grid);
    if (!grid) return 0;

    determine_equations(grid->divergence(), grid->rho(), grid->boundary());
    Eigen::ConjugateGradient<SpMat,Eigen::Lower|Eigen::Upper,
                             Eigen::DiagonalPreconditioner<float>> solver;
    solver.setMaxIterations(PROPERTY_I(MAX_STEPS));
    solver.setTolerance(PROPERTY_F(SOLVER_THRESHOLD));
    solver.compute(*_A);
    auto& x0 = *_x0; auto& p = grid->pressure();
    for (int i = 0; i < _size; i++) x0[i] = p[i];
    VecXf sol = solver.solveWithGuess(*_b, x0);
    for (int i = 0; i < _size; i++) p[i] = sol[i];
    return (float)solver.error();
}

EigenBiCGSTABSolver2D::EigenBiCGSTABSolver2D(const int w, const int h) : EigenSolver2D(w, h) {}
float EigenBiCGSTABSolver2D::solve(IGrid2D* _grid) {
    auto* grid = dynamic_cast<Grid2D*>(_grid);
    if (!grid) return 0;

    determine_equations(grid->divergence(), grid->rho(), grid->boundary());
    Eigen::BiCGSTAB<SpMat, Eigen::DiagonalPreconditioner<float>> solver;
    solver.setMaxIterations(PROPERTY_I(MAX_STEPS));
    solver.setTolerance(PROPERTY_F(SOLVER_THRESHOLD));
    solver.compute(*_A);
    auto& x0 = *_x0; auto& p = grid->pressure();
    for (int i = 0; i < _size; i++) x0[i] = p[i];
    VecXf sol = solver.solveWithGuess(*_b, x0);
    for (int i = 0; i < _size; i++) p[i] = sol[i];
    return (float)solver.error();
}