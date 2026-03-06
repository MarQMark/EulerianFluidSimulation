#include "Simulator.h"

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
}

Simulator::~Simulator() {
    delete _window;
}

void Simulator::update() {
    while (!_window->shouldClose()) {

        _window->update();
    }
}
