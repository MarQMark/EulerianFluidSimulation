#ifndef EULERIANFLUIDSIMULATION_JACOBISOLVER_H
#define EULERIANFLUIDSIMULATION_JACOBISOLVER_H

#include "ISolver.h"
#include "assets/Shader.h"
#include "simulation/data/StorageBuffer.h"

class JacobiSolverCPU2D final : public ISolverCPU2D {
public:
    JacobiSolverCPU2D(unsigned int width, unsigned int height);
    ~JacobiSolverCPU2D() override;

    float solve(IGrid2D *grid) override;

private:
    ScalField2D* _tmp{};

    void impl_mat_mul(ScalField2D& result, const ScalField2D& x, const ScalField2D& rho, const ScalField2D& boundary) const;
};

class JacobiSolverOpenGL2D final : public ISolverOpenGL2D {
public:
    JacobiSolverOpenGL2D(unsigned int width, unsigned int height);
    ~JacobiSolverOpenGL2D() override;

    float solve(IGrid2D *grid) override;

private:
    Shader* _shader_jp{};
    StorageBuffer* _gpu_tmp{};
};

class JacobiSolverCPU3D final : public ISolverCPU3D {
public:
    JacobiSolverCPU3D(unsigned int width, unsigned int height, unsigned int depth);
    ~JacobiSolverCPU3D() override;

    float solve(IGrid3D *grid) override;

private:
    ScalField3D* _tmp{};

    void impl_mat_mul(ScalField3D& result, const ScalField3D& x, const ScalField3D& rho, const ScalField3D& boundary) const;
};

class JacobiSolverOpenGL3D final : public ISolverOpenGL3D {
public:
    JacobiSolverOpenGL3D(unsigned int width, unsigned int height, unsigned int depth);
    ~JacobiSolverOpenGL3D() override;

    float solve(IGrid3D *grid) override;

private:
    Shader* _shader_jp{};
    StorageBuffer* _gpu_tmp{};
};

#endif //EULERIANFLUIDSIMULATION_JACOBISOLVER_H