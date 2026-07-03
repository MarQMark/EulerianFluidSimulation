#ifndef EULERIANFLUIDSIMULATION_EIGENSOLVER_H
#define EULERIANFLUIDSIMULATION_EIGENSOLVER_H

#include "ISolver.h"

#include <eigen/Eigen/Sparse>
#include <eigen/Eigen/IterativeLinearSolvers>

#include "simulation/ScalField2D.h"

class EigenSolver2D : public ISolverCPU2D {
public:
    ~EigenSolver2D() override;

protected:
    EigenSolver2D(int width, int height);

    Eigen::SparseMatrix<float, Eigen::RowMajor>* _A{};
    Eigen::VectorXf* _b{};
    Eigen::VectorXf* _x0{};

    void determine_equations(const ScalField2D &divergence, const ScalField2D &rho, const ScalField2D &boundary) const;
};

class EigenCGSolver2D : public EigenSolver2D {
public:
    EigenCGSolver2D(int width, int height);

protected:
    float solve(Grid2D* grid) override;
};

class EigenBiCGSTABSolver2D : public EigenSolver2D {
public:
    EigenBiCGSTABSolver2D(int width, int height);

protected:
    float solve(Grid2D* grid) override;

};

#endif //EULERIANFLUIDSIMULATION_EIGENSOLVER_H