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
    glm::vec2 traceAlongField( glm::vec2 start, const ScalField& rho, const ScalField& walls, float D, int substeps) const;
    void scatterValue(ScalField& rho, const ScalField& walls, glm::vec2 pos, float value);

    void add_force() const;
    void vel_advect();
    void diffuse();
    void project();

    void enforce_wall_boundaries() const;
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H