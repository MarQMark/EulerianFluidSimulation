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

    void vel_decay() const;
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;
    void project() override;

    void exec_shader(const Shader* shader, unsigned int gx, unsigned int gy) const;
    void exec_shader_adv(const Shader* shader, unsigned int gx, unsigned int gy) const;
    void exec_shader_off(const Shader* shader, unsigned int gx, unsigned int gy, unsigned int offset) const;

    Shader* _vel_decay{};
    Shader* _vel_extrap_init{};
    Shader* _vel_extrapolate{};
    Shader* _rho_sharpen_calc{};
    Shader* _rho_sharpen_apply{};
    Shader* _rho_sharpen_scatter{};
    Shader* _rho_adv_1{};
    Shader* _rho_adv_2_3{};
    Shader* _rho_adv_4{};
    Shader* _rho_adv_5{};
    Shader* _rho_adv_6_10_bwd{};
    Shader* _rho_adv_6_10_fwd{};
    Shader* _rho_adv_7_bwd{};
    Shader* _rho_adv_7_fwd{};
    Shader* _rho_adv_8_bwd{};
    Shader* _rho_adv_8_fwd{};
    Shader* _rho_adv_9_bwd{};
    Shader* _rho_adv_9_fwd{};
    Shader* _rho_adv_11_bwd{};
    Shader* _rho_adv_11_fwd{};
    Shader* _rho_adv_11_cpy{};
    Shader* _rho_adv_12_x{};
    Shader* _rho_adv_12_y{};
    Shader* _add_force{};
    Shader* _vel_adv_pass_1{};
    Shader* _vel_adv_pass_2{};
    Shader* _proj_calc_div{};
    Shader* _proj_sub_pg{};
    Shader* _proj_enf_bound{};

    OpenGLGrid2D* _grid{};
    ISolverOpenGL2D* _solver{};
    SimulationDisplay2D::RenderData2D* _data{};
};


#endif //EULERIANFLUIDSIMULATION_OPENGLSIMULATOR2D_H