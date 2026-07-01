#ifndef EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H
#define EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H

#include "ISimulator2D.h"
#include "assets/Shader.h"
#include "simulation/Grid2D.h"
#include "simulation/SimulationDisplay2D.h"
#include "solver/ISolver.h"

class SimulationDisplay2D;

class OpenGLSimulator2D final : public ISimulator2D {
public:
    OpenGLSimulator2D(unsigned width, unsigned height);
    ~OpenGLSimulator2D() override;

    void* getRenderData() override;

private:
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;
    void project() override;

    Shader* _vel_pass_1{};
    Shader* _vel_pass_2{};
    Shader* _add_force{};

    Grid2D* _grid{};
    ISolver* _solver{};
    SimulationDisplay2D::RenderData2D* _data;
};


#endif //EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H