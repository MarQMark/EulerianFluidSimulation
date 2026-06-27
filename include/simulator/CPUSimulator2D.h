#ifndef EULERIANFLUIDSIMULATION_CPUSIMULATOR2D_H
#define EULERIANFLUIDSIMULATION_CPUSIMULATOR2D_H

#include "ISimulator2D.h"
#include "simulation/Grid.h"
#include "simulation/SimulationDisplay.h"
#include "solver/ISolver.h"

class SimulationDisplay2D;

class CPUSimulator2D final : public ISimulator2D {
public:
    CPUSimulator2D(unsigned width, unsigned height);
    ~CPUSimulator2D() override;

    void* getRenderData() override;

private:
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;
    void project() override;

    Grid* _grid{};
    ISolver* _solver{};
    SimulationDisplay2D::RenderData2D* _data;

    glm::vec2 trace_along_field(glm::vec2 start, float D, int substeps) const;
    void scatter_values(glm::vec2 pos, float value) const;

    void enforce_wall_boundaries() const;
};

#endif //EULERIANFLUIDSIMULATION_CPUSIMULATOR2D_H