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

#endif
#endif //EULERIANFLUIDSIMULATION_CUDASOLVER_H