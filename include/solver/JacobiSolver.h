#ifndef EULERIANFLUIDSIMULATION_JACOBISOLVER_H
#define EULERIANFLUIDSIMULATION_JACOBISOLVER_H

#include "ISolver.h"
#include "assets/Shader.h"
#include "compute/StorageBuffer.h"
#include "simulation/ScalField.h"

class JacobiSolver : public ISolver {
public:
    JacobiSolver(int width, int height);
    ~JacobiSolver() override;

    float solve(Grid *grid) override;

private:
    ScalField* _tmp{};

#ifdef USE_GPU
    Shader* _shader_jp{};
    StorageBuffer* _gpu_tmp{};
#endif

    void impl_mat_mul(ScalField& result, const ScalField& x, const ScalField& rho, const ScalField& boundary) const;
};

#endif //EULERIANFLUIDSIMULATION_JACOBISOLVER_H