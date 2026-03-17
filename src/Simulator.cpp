#include "Simulator.h"

#include <views/SimulationView.h>

#define RES_X 128
#define RES_Y 72

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
    _renderer = new Renderer(RES_X, RES_Y);
    _views = new Views(_window);

    _views->addView("Simulation", new SimulationView(_renderer->getTexture2D()->get(), RES_X, RES_Y));

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
    }
}

void Simulator::simulate() {
    v_step();
    s_step();
    //_grid->swap();

    auto& vel = _grid->getVel();
    for(int i = 0; i < vel.size(); i++)
        vel[i] = dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getForces()[i];
}

void Simulator::v_step() {
}

void Simulator::s_step() {
}
