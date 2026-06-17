#ifndef EULERIANFLUIDSIMULATION_SOLVER_H
#define EULERIANFLUIDSIMULATION_SOLVER_H

#include "ScalField.h"
#include <eigen/Eigen/Sparse>
#include <eigen/Eigen/IterativeLinearSolvers>

class Solver {
protected:
    Solver(int width, int height);

public:
    virtual ~Solver() = default;

    virtual float solve(ScalField& pressure, const ScalField& divergence, const ScalField& rho, const ScalField& boundary) = 0;

protected:
    int _width = 0;
    int _height = 0;
    int _size = 0;
};

class CGSolver : public Solver {
public:
    CGSolver(int width, int height);
    ~CGSolver() override;
    float solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) override;

private:
    void impl_mat_mul(ScalField& result, const ScalField& x, const ScalField& rho, const ScalField& boundary) const;

    ScalField* _t{};
    ScalField* _tmp{};
    ScalField* _r{};
    ScalField* _d{};
};

class EigenSolver : public Solver {
public:
    ~EigenSolver() override;

protected:
    EigenSolver(int width, int height);

    Eigen::SparseMatrix<float, Eigen::RowMajor>* _A{};
    Eigen::VectorXf* _b{};
    Eigen::VectorXf* _x0{};

    void determine_equations(const ScalField &divergence, const ScalField &rho, const ScalField &boundary) const;
};

class EigenCGSolver : public EigenSolver {
public:
    EigenCGSolver(int width, int height);

protected:
    float solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) override;
};

class EigenBiCGSTABSolver : public EigenSolver {
public:
    EigenBiCGSTABSolver(int width, int height);

protected:
    float solve(ScalField &pressure, const ScalField &divergence, const ScalField &rho, const ScalField &boundary) override;

};


#endif //EULERIANFLUIDSIMULATION_SOLVER_H