#include "Simulator.h"

#include <views/SimulationView.h>

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
    _renderer = new Renderer();
    _views = new Views(_window);

    _views->addView("Simulation", new SimulationView(0));

    _grid = new Grid(128, 72);
}

Simulator::~Simulator() {
    delete _grid;
    delete _views;
    delete _renderer;
    delete _window;
}

void Simulator::update() {
    while (!_window->shouldClose()) {
        _views->render();
        _renderer->render(_grid);
        _window->update();
    }
}
