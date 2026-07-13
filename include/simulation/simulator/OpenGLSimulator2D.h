#ifndef EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H
#define EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H

#include "ISimulator2D.h"
#include "assets/Shader.h"
#include "simulation/data/OpenGLGrid2D.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/solver/ISolver.h"

class SimulationDisplay2D;

class OpenGLSimulator2D final : public ISimulator2D {
public:
    OpenGLSimulator2D(unsigned width, unsigned height);
    ~OpenGLSimulator2D() override;

    void* getRenderData() override;


private:
    void update() override;

    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;
    void project() override;

    Shader* _rv_1_adv_gamma{};
    Shader* _rv_2_comp_a{};
    Shader* _rv_4_comp_beta{};
    Shader* _rv_5_fwd_trace{};
    Shader* _rv_6_comp_gamma{};
    Shader* _rv_7_scale_a{};
    Shader* _rv_9_clamp_beta{};
    Shader* _rv_11_Ap{};

    Shader* _add_force{};
    Shader* _vel_pass_1{};
    Shader* _vel_pass_2{};
    Shader* _proj_calc_div{};
    Shader* _proj_sub_pg{};

    OpenGLGrid2D* _grid{};
    ISolverOpenGL2D* _solver{};
    SimulationDisplay2D::RenderData2D* _data{};
};


#endif //EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H