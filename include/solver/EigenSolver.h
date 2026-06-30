#ifndef EULERIANFLUIDSIMULATION_EIGENSOLVER_H
#define EULERIANFLUIDSIMULATION_EIGENSOLVER_H

#include "ISolver.h"

#include <eigen/Eigen/Sparse>
#include <eigen/Eigen/IterativeLinearSolvers>

#include "simulation/ScalField2D.h"

class EigenSolver : public ISolver {
public:
    ~EigenSolver() override;

protected:
    EigenSolver(int width, int height);

    Eigen::SparseMatrix<float, Eigen::RowMajor>* _A{};
    Eigen::VectorXf* _b{};
    Eigen::VectorXf* _x0{};

    void determine_equations(const ScalField2D &divergence, const ScalField2D &rho, const ScalField2D &boundary) const;
};

class EigenCGSolver : public EigenSolver {
public:
    EigenCGSolver(int width, int height);

protected:
    float solve(Grid* grid) override;
};

class EigenBiCGSTABSolver : public EigenSolver {
public:
    EigenBiCGSTABSolver(int width, int height);

protected:
    float solve(Grid* grid) override;

};

#endif //EULERIANFLUIDSIMULATION_EIGENSOLVER_H