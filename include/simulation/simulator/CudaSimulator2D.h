#ifndef EULERIANFLUIDSIMULATION_CUDASIMULATOR2D_H
#define EULERIANFLUIDSIMULATION_CUDASIMULATOR2D_H
#ifdef HAS_CUDA

#include "ISimulator2D.h"
#include "simulation/data/CudaGrid2D.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/solver/CudaSolver.h"

class SimulationDisplay2D;

class CudaSimulator2D final : public ISimulator2D {
public:
    CudaSimulator2D(unsigned width, unsigned height);
    ~CudaSimulator2D() override;

    void* getRenderData() override;


private:
    void update() override;

    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void diffuse() override;
    void vel_advect() override;
    void project() override;

    CudaGrid2D* _grid{};
    ISolverCuda2D* _solver{};
    SimulationDisplay2D::RenderData2D* _data{};
};


#endif
#endif //EULERIANFLUIDSIMULATION_CUDASIMULATOR2D_H