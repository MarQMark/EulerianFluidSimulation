#ifndef EULERIANFLUIDSIMULATION_SOLVER_H
#define EULERIANFLUIDSIMULATION_SOLVER_H

#include "ScalField.h"
#include <eigen/Eigen/Sparse>
#include <eigen/Eigen/IterativeLinearSolvers>

#include "Grid.h"
#include "compute/ComputeShader.h"
#include "compute/StorageBuffer.h"

class Solver {
protected:
    Solver(int width, int height);

public:
    virtual ~Solver() = default;

    virtual float solve(Grid* grid) = 0;

protected:
    int _width = 0;
    int _height = 0;
    int _size = 0;
};

class CGSolver : public Solver {
public:
    CGSolver(int width, int height);
    ~CGSolver() override;
    float solve(Grid* grid) override;

private:
    void impl_mat_mul(ScalField& result, const ScalField& x, const ScalField& rho, const ScalField& boundary) const;

    ScalField* _t{};
    ScalField* _tmp{};
    ScalField* _r{};
    ScalField* _d{};

#ifdef USE_GPU
    ComputeShader* _shader_as{};
    ComputeShader* _shader_mm{};
    ComputeShader* _shader_ss{};
    ComputeShader* _shader_sa{};
    ComputeShader* _shader_dp{};

    StorageBuffer* _gpu_t{};
    StorageBuffer* _gpu_tmp{};
    StorageBuffer* _gpu_r{};
    StorageBuffer* _gpu_d{};

    std::vector<float> _dot;
    StorageBuffer* _gpu_dot{};
    StorageBuffer* _gpu_sqrt_len{};
#endif
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
    float solve(Grid* grid) override;
};

class EigenBiCGSTABSolver : public EigenSolver {
public:
    EigenBiCGSTABSolver(int width, int height);

protected:
    float solve(Grid* grid) override;

};


#endif //EULERIANFLUIDSIMULATION_SOLVER_H