#include "Simulator.h"

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
    _views = new Views(_window);
}

Simulator::~Simulator() {
    delete _window;
    delete _views;
}

void Simulator::update() {
    while (!_window->shouldClose()) {

        _views->render();
        _window->update();
    }
}
