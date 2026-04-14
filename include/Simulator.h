#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

#include <renderer/Renderer.h>

#include "renderer/Window.h"
#include "simulation/Grid.h"
#include "simulation/VecField.h"
#include "views/Views.h"

#define SCAL_FIELD std::vector<float>
#define SCAL float

class Simulator {
public:
    Simulator();
    ~Simulator();

    void update();

private:
    Window* _window{};
    Renderer* _renderer{};

    Views* _views{};

    Grid* _grid;

    bool _paused = false;

    void simulate();

    void add_force(VecField& vel);
    void advect(VecField& velNew, VecField& vel);
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H