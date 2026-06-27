#include "simulation/SimulationDisplay.h"

#include "util/Time.h"
#include "views/Simulation2DView.h"
#include "views/Simulation3DView.h"

SimulationDisplay2D::SimulationDisplay2D(const unsigned int width, const unsigned int height) {
    _renderer = new Renderer2D(width, height);
}

SimulationDisplay2D::~SimulationDisplay2D() {
    delete _renderer;
}

void SimulationDisplay2D::update(void* renderData) {
    const auto* data = static_cast<RenderData2D*>(renderData);
    if (renderData) {
        _renderer->render(data->grid);
        _simulation_view->updateGrid(data->grid);
    }
}

Viewable* SimulationDisplay2D::createViewable() {
    _simulation_view = new Simulation2DView(_renderer->getTexture2D()->get());
    return _simulation_view;
}

std::string_view SimulationDisplay2D::viewableName() {
    return "Simualtion2D";
}


SimulationDisplay3D::SimulationDisplay3D(Window* window) {
    _renderer = new Renderer3D();
    _scene = new Scene(window);
    _editor = new Editor;
    _editor->setScene(_scene);
}

SimulationDisplay3D::~SimulationDisplay3D() {
    delete _editor;
    delete _scene;
    delete _renderer;
}

void SimulationDisplay3D::update(void* renderData) {
    const auto* data = static_cast<RenderData3D *>(renderData);

    _scene->update(Time::Get()->dt);
    _renderer->render(_scene);
}

Viewable* SimulationDisplay3D::createViewable() {
    return new Simulation3DView(_editor, _renderer);
}

std::string_view SimulationDisplay3D::viewableName() {
    return "Simulation3D";
}

