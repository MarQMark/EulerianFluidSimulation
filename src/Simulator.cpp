#include "Simulator.h"

#include <algorithm>
#include <iostream>
#include <views/SimulationView.h>

#include "simulation/Properties.h"
#include "views/PropertiesView.h"

#define RES_X 128
#define RES_Y 72

#define DT 0.0167f

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
    _renderer = new Renderer(RES_X, RES_Y);
    _views = new Views(_window);

    _views->addView("Simulation", new SimulationView(_renderer->getTexture2D()->get(), RES_X, RES_Y));
    _views->addView("Properties", new PropertiesView());

    _grid = new Grid(RES_X, RES_Y);
}

Simulator::~Simulator() {
    delete _grid;
    delete _views;
    delete _renderer;
    delete _window;
}

void Simulator::update() {
    while (!_window->shouldClose()) {
        simulate();
        _views->render();
        _renderer->render(_grid);
        _window->update();

        if(ImGui::IsKeyPressed(ImGuiKey_R)) {
            delete _grid;
            _grid = new Grid(RES_X, RES_Y);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_P))
            _paused = !_paused;
    }
}

void Simulator::simulate() {
    if (_paused)
        return;

    advect(_grid->getNewVel(), _grid->getVel());
    _grid->swapVel();
    add_force(_grid->getVel());


    //_grid->swapPres();
    //_grid->swapVel();
}

glm::vec2 bilinearInterpol(glm::vec2 pos, const std::vector<glm::vec2> &field) {
    pos += glm::vec2(.5);
    const int x = (int)pos.x;
    const int y = (int)pos.y;

    int ix1 = x;
    int ix2 = x + 1;
    if(pos.x - (float)x < 0.5f) {
        ix1 = x - 1;
        ix2 = x;
    }
    int iy1 = y;
    int iy2 = y + 1;
    if(pos.y - (float)y < 0.5f) {
        iy1 = y - 1;
        iy2 = y;
    }

    ix1 = std::clamp(ix1, 0, RES_X - 1);
    ix2 = std::clamp(ix2, 0, RES_X - 1);
    iy1 = std::clamp(iy1, 0, RES_Y - 1);
    iy2 = std::clamp(iy2, 0, RES_Y - 1);

    glm::vec2 v[4] = {
        field[iy1 * RES_X + ix1],
        field[iy2 * RES_X + ix1],
        field[iy1 * RES_X + ix2],
        field[iy2 * RES_X + ix2],
    };

    float x1 = (float)ix1 + 0.5f;
    float x2 = (float)ix2 + 0.5f;
    float y1 = (float)iy1 + 0.5f;
    float y2 = (float)iy2 + 0.5f;

    const float dx2x1 = x2 - x1;
    glm::vec2 fxy1 = ((x2 - pos.x)/dx2x1) * v[0] + ((pos.x - x1)/dx2x1) * v[2];
    glm::vec2 fxy2 = ((x2 - pos.x)/dx2x1) * v[1] + ((pos.x - x1)/dx2x1) * v[3];
    if(dx2x1 < 1e-9) {
        fxy1 = v[1];
        fxy2 = v[3];
    }

    const float dy2y1 = y2 - y1;
    glm::vec2 fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
    if(dy2y1 < 1e-9)
        fxy = fxy1;

    return fxy;
}
glm::vec2 bilinearInterpol2(const glm::vec2 pos, const std::vector<glm::vec2> &field)
{
    int x0 = (int)std::floor(pos.x);
    int y0 = (int)std::floor(pos.y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    x0 = std::clamp(x0, 0, RES_X - 1);
    y0 = std::clamp(y0, 0, RES_Y - 1);
    x1 = std::clamp(x1, 0, RES_X - 1);
    y1 = std::clamp(y1, 0, RES_Y - 1);

    float sx = pos.x - x0;
    float sy = pos.y - y0;

    auto at = [&](int x, int y) -> const glm::vec2& {
        return field[y * RES_X + x];
    };

    const glm::vec2 &v00 = at(x0, y0);
    const glm::vec2 &v10 = at(x1, y0);
    const glm::vec2 &v01 = at(x0, y1);
    const glm::vec2 &v11 = at(x1, y1);

    glm::vec2 v0 = (1.0f - sx) * v00 + sx * v10;
    glm::vec2 v1 = (1.0f - sx) * v01 + sx * v11;

    glm::vec2 ret = (1.0f - sy) * v0 + sy * v1;

    return ret;
}

void Simulator::add_force(VecField &vel) {
    for (int y = 1; y < RES_Y - 1; y++)
        for (int x = 1; x < RES_X - 1; x++)
            vel(x, y) += dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getForces()(x, y) * DT;
}

void Simulator::advect(VecField& velNew, VecField& vel) {
    glm::vec2 rdx(RES_X, RES_Y);
    for (int y = 1; y < RES_Y - 1; y++) {
        for (int x = 1; x < RES_X - 1; x++)
        {
            const glm::vec2 pos = glm::vec2(x, y) - DT * rdx * vel(x, y);
            velNew(x, y) = vel.interpolate(pos);
        }
    }
}