#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

#include <renderer/Renderer2D.h>

#include "assets/Shader.h"
#include "renderer/Window.h"
#include "simulation/Grid.h"
#include "simulation/Solver.h"
#include "simulation/VecField.h"
#include "views/Views.h"

class Scene;
class Editor;
class Renderer3D;

class Simulator {
public:
    Simulator();
    ~Simulator();

    void update();

private:
    Scene* _scene{};
    Window* _window{};
    Editor* _editor{};
    Renderer2D* _renderer2D{};
    Renderer3D* _renderer3D{};

    Views* _views{};

    Grid* _grid{};

    Solver* _solver{};

#ifdef USE_GPU
    Shader* _vel_pass_1{};
    Shader* _vel_pass_2{};
    Shader* _add_force{};
#endif

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