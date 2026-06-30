#ifndef EULERIANFLUIDSIMULATION_JACOBISOLVER_H
#define EULERIANFLUIDSIMULATION_JACOBISOLVER_H

#include "ISolver.h"
#include "assets/Shader.h"
#include "compute/StorageBuffer.h"
#include "simulation/ScalField2D.h"

class JacobiSolver : public ISolver {
public:
    JacobiSolver(int width, int height);
    ~JacobiSolver() override;

    float solve(Grid *grid) override;

private:
    ScalField2D* _tmp{};

#ifdef USE_GPU
    Shader* _shader_jp{};
    StorageBuffer* _gpu_tmp{};
#endif

    void impl_mat_mul(ScalField2D& result, const ScalField2D& x, const ScalField2D& rho, const ScalField2D& boundary) const;
};

#endif //EULERIANFLUIDSIMULATION_JACOBISOLVER_H