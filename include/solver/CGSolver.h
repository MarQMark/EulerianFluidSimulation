#ifndef EULERIANFLUIDSIMULATION_SOLVER_H
#define EULERIANFLUIDSIMULATION_SOLVER_H

#include "simulation/ScalField.h"
#include "simulation/Grid.h"
#include "compute/StorageBuffer.h"

#include "ISolver.h"
#include "assets/Shader.h"

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
#endif
};


#endif //EULERIANFLUIDSIMULATION_SOLVER_H