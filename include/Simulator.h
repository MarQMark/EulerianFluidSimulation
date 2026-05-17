#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

 #include <renderer/Renderer.h>

#include "renderer/Window.h"
#include "simulation/Grid.h"
#include "simulation/VecField.h"
#include "views/Views.h"

class Simulator {
public:
    Simulator();
    ~Simulator();

    void update();

private:
    Window* _window{};
    Renderer* _renderer{};

    Views* _views{};

    Grid* _grid{};

    bool _reset = false;

    void recreate();

    void simulate();
    void vel_extrapolate();
    void rho_advect();
    void rho_sharpen();
    void add_force() const;
    void vel_advect();
    void diffuse();
    void project();

#if 0
    void simulate();

    void add_force(VecField& vel, ScalField &rho);
    void advect(VecField& velNew, VecField& vel);
    //void projection(VecField& field);
    void VorticityConfinement();

    void extrapolateVelocity(VecField& vel, const ScalField& rho);
    void advectScalar(ScalField& rhoNew, const ScalField& rho, VecField& vel);
    float interpolateScalar(const ScalField& f, glm::vec2 pos);
    void sharpenDensity(ScalField& rho);
    void applyGravity(VecField& field, ScalField& vof);
    void projection(VecField& field, ScalField& vof);

    void advect2(VecField& velNew, VecField& vel);
#endif
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H