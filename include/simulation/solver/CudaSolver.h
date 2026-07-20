#ifndef EULERIANFLUIDSIMULATION_CUDASOLVER_H
#define EULERIANFLUIDSIMULATION_CUDASOLVER_H
#ifdef HAS_CUDA

#include "ISolver.h"

class ISolverCuda2D : public ISolver2D {
protected:
    ISolverCuda2D(const int width, const int height) : ISolver2D(width, height) {}
};
class JacobiSolverCuda2D final : public ISolverCuda2D {
public:
    JacobiSolverCuda2D(int width, int height);
    ~JacobiSolverCuda2D() override;

    float solve(IGrid2D *grid) override;

private:
    float* _gpu_tmp{};
};
class CGSolverCuda2D final : public ISolverCuda2D {
public:
    CGSolverCuda2D(int width, int height);
    ~CGSolverCuda2D() override;

    float solve(IGrid2D *grid) override;

private:
    float* _d_r       = nullptr;   // residual
    float* _d_d       = nullptr;   // search direction
    float* _d_t       = nullptr;   // t = L * d  (and L * p for recompute)
    float* _d_partial = nullptr;   // CG_NBLOCKS partial dot-product sums
    float* _d_dot1    = nullptr;   // scalar result (r·r)
    float* _d_dot2    = nullptr;   // scalar result (d·t)

    void  dot(const float* a, const float* b, float* d_result) const;
    static float read_scalar(const float* d_ptr);
};

#endif
#endif //EULERIANFLUIDSIMULATION_CUDASOLVER_H