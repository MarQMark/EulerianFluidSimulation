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

    void simulate();
    void v_step();
    void s_step();

    static void advect(std::vector<glm::vec2>& field, std::vector<glm::vec2>& vel);
    void diffuse(std::vector<glm::vec2>& field, std::vector<glm::vec2>& vel);
    void add_force(std::vector<glm::vec2>& field, std::vector<glm::vec2>& vel);
    void compute_pres(std::vector<float>& pres, std::vector<glm::vec2>& vel);
    void sub_pres(std::vector<glm::vec2>& field, std::vector<float>& pres, std::vector<glm::vec2>& vel);
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H