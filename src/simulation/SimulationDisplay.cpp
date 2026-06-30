#include "simulation/SimulationDisplay.h"

#include "simulation/scene/nodes/FluidNode.h"
#include "util/Time.h"
#include "views/Simulation2DView.h"
#include "views/Simulation3DView.h"

SimulationDisplay2D::SimulationDisplay2D(const unsigned int width, const unsigned int height)
    : ISimulationDisplay(), _width(width), _height(height) {
    _renderer = new Renderer2D(width, height);
    _viewables["Simualtion2D"] = new Simulation2DView(_renderer->getTexture2D()->get(), _width, _height);
}

SimulationDisplay2D::~SimulationDisplay2D() {
    delete _renderer;
}

void SimulationDisplay2D::update(void* renderData) {
    const auto* data = static_cast<RenderData2D*>(renderData);
    if (renderData) {
        _renderer->render(data->grid);
    }
}

SimulationDisplay3D::SimulationDisplay3D(Window* window) : ISimulationDisplay() {
    _renderer = new Renderer3D();
    _scene = new Scene(window);
    _editor = new Editor;
    _editor->setScene(_scene);

    _viewables["Simulation3D"] = new Simulation3DView(_editor, _renderer);
}

SimulationDisplay3D::~SimulationDisplay3D() {
    delete _editor;
    delete _scene;
    delete _renderer;
}

void SimulationDisplay3D::update(void* renderData) {
    const auto* data = static_cast<RenderData3D *>(renderData);
    _scene->getFluid()->setRenderData(data);

    _scene->update(Time::Get()->dt);
    _renderer->render(_scene);
}