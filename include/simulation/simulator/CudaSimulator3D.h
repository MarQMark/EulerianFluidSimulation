#ifndef EULERIANFLUIDSIMULATION_CUDASIMULATOR3D_H
#define EULERIANFLUIDSIMULATION_CUDASIMULATOR3D_H
#ifdef HAS_CUDA

#include "ISimulator3D.h"
#include "simulation/data/CudaGrid3D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "simulation/solver/CudaSolver.h"

class SimulationDisplay3D;

class CudaSimulator3D final : public ISimulator3D {
public:
    CudaSimulator3D(unsigned int width, unsigned int height, unsigned int depth);
    ~CudaSimulator3D() override;

    void* getRenderData() override;

private:

    void update() override;

    void vel_decay() const;
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void diffuse() override;
    void vel_advect() override;

    void project() override;

    CudaGrid3D* _grid{};
    ISolverCuda3D* _solver{};
    SimulationDisplay3D::RenderData3D* _data{};
};


#endif

#endif //EULERIANFLUIDSIMULATION_CUDASIMULATOR3D_H