#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

#include <renderer/Renderer.h>

#include "renderer/Window.h"
#include "simulation/Grid.h"
#include "views/Views.h"

#define SCAL_FIELD std::vector<float>
#define SCAL float
#define VEC_FIELD std::vector<glm::vec2>
#define VEC glm::vec2

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

    void add_force(VEC_FIELD& vel);
    void advect(VEC_FIELD& velNew, VEC_FIELD& vel);
    void diffuse(VEC_FIELD& vel);
    void project(VEC_FIELD& velNew, VEC_FIELD& vel, SCAL_FIELD& presNew, SCAL_FIELD& pres);
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H