#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

#include <renderer/Renderer.h>

#include "renderer/Window.h"
#include "simulation/Grid.h"
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

    Grid* _grid;
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H