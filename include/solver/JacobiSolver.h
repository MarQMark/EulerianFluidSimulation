#ifndef EULERIANFLUIDSIMULATION_JACOBISOLVER_H
#define EULERIANFLUIDSIMULATION_JACOBISOLVER_H

#include "ISolver.h"
#include "assets/Shader.h"
#include "compute/StorageBuffer.h"

class JacobiSolverCPU2D final : public ISolverCPU2D {
public:
    JacobiSolverCPU2D(int width, int height);
    ~JacobiSolverCPU2D() override;

    float solve(Grid2D *grid) override;

private:
    ScalField2D* _tmp{};

    void impl_mat_mul(ScalField2D& result, const ScalField2D& x, const ScalField2D& rho, const ScalField2D& boundary) const;
};

class JacobiSolverOpenGL2D final : public ISolverOpenGL2D {
public:
    JacobiSolverOpenGL2D(int width, int height);
    ~JacobiSolverOpenGL2D() override;

    float solve(Grid2D *grid) override;

private:
    Shader* _shader_jp{};
    StorageBuffer* _gpu_tmp{};
};

#endif //EULERIANFLUIDSIMULATION_JACOBISOLVER_H