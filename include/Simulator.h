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
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H