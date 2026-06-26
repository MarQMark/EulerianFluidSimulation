#ifndef EULERIANFLUIDSIMULATION_SOLVER_H
#define EULERIANFLUIDSIMULATION_SOLVER_H

#include "simulation/ScalField.h"
#include "simulation/Grid.h"
#include "compute/ComputeShader.h"
#include "compute/StorageBuffer.h"

#include "ISolver.h"

class CGSolver : public ISolver {
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

    ComputeShader* _shader_asg{};
    ComputeShader* _shader_sag{};
    ComputeShader* _shader_dp2{};
    ComputeShader* _shader_r2{};
    ComputeShader* _shader_us{};

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
#endif
};


#endif //EULERIANFLUIDSIMULATION_SOLVER_H