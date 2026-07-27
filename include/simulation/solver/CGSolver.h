#ifndef EULERIANFLUIDSIMULATION_SOLVER_H
#define EULERIANFLUIDSIMULATION_SOLVER_H

#include "simulation/data/ScalField2D.h"
#include "simulation/data/Grid2D.h"
#include "simulation/data/StorageBuffer.h"

#include "ISolver.h"
#include "assets/Shader.h"

class CGSolverCPU2D final : public ISolverCPU2D {
public:
    CGSolverCPU2D(unsigned int width, unsigned int height);
    ~CGSolverCPU2D() override;
    float solve(IGrid2D* grid) override;

private:
    void impl_mat_mul(ScalField2D& result, const ScalField2D& x, const ScalField2D& rho, const ScalField2D& boundary) const;

    ScalField2D* _t{};
    ScalField2D* _tmp{};
    ScalField2D* _r{};
    ScalField2D* _d{};
};

class CGSolverOpenGL2D final : public ISolverOpenGL2D {
public:
    CGSolverOpenGL2D(unsigned int width, unsigned int height);
    ~CGSolverOpenGL2D() override;

    float solve(IGrid2D* grid) override;

private:
    Shader* _shader_as{};
    Shader* _shader_mm{};
    Shader* _shader_ss{};
    Shader* _shader_sa{};

    Shader* _shader_asg{};
    Shader* _shader_sag{};
    Shader* _shader_dp2{};
    Shader* _shader_r2{};
    Shader* _shader_us{};

    StorageBuffer* _gpu_t{};
    StorageBuffer* _gpu_tmp{};
    StorageBuffer* _gpu_r{};
    StorageBuffer* _gpu_d{};

    StorageBuffer* _gpu_scalars{};
    StorageBuffer* _gpu_dot2{};
    std::vector<StorageBuffer*> _gpu_reduce_chain;

    std::vector<int> _reduceChain;
    std::vector<float> _scalars_cpu;
    int _numGroups = 0;
};

class CGSolverCPU3D final : public ISolverCPU3D {
public:
    CGSolverCPU3D(unsigned int width, unsigned int height, unsigned int depth);
    ~CGSolverCPU3D() override;
    float solve(IGrid3D* grid) override;

private:
    void impl_mat_mul(ScalField3D& result, const ScalField3D& xField, const ScalField3D& rho, const ScalField3D& boundary) const;

    ScalField3D* _t{};
    ScalField3D* _tmp{};
    ScalField3D* _r{};
    ScalField3D* _d{};
};


#endif //EULERIANFLUIDSIMULATION_SOLVER_H