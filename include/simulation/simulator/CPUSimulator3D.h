#ifndef EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H
#define EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H

#include "ISimulator3D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "simulation/solver/ISolver.h"

class CPUSimulator3D final : public ISimulator3D {
public:
    CPUSimulator3D(unsigned int width, unsigned int height, unsigned int depth);
    ~CPUSimulator3D() override;

    void update() override;

    void* getRenderData() override;

private:
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;

    void enforce_wall_boundaries() const;

    void project() override;

    Grid3D* _grid{};
    ISolverCPU3D* _solver{};
    SimulationDisplay3D::RenderData3D* _data;
};


#endif //EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H