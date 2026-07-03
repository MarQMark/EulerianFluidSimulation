//
// Created by ruut on 7/1/26.
//

#include "simulation/SimulationDisplay3D.h"
#include "core/Editor.h"
#include "renderer/Renderer3D.h"
#include "simulation/Properties.h"
#include "simulation/scene/nodes/FluidNode.h"
#include "util/Time.h"
#include "views/NodePropertiesView.h"
#include "views/SceneView.h"
#include "views/Simulation3DView.h"

SimulationDisplay3D::SimulationDisplay3D(Window* window) : ISimulationDisplay() {
    _renderer = new Renderer3D();
    _scene = new Scene(window);
    _editor = new Editor;
    _editor->setScene(_scene);

    _viewables["Simulation3D"] = new Simulation3DView(_editor, _renderer);
    _viewables["Scene"] = new SceneView(_editor, _renderer);
    _viewables["NodeProperties"] = new NodePropertiesView(_editor);

    SET_PROPERTY_B(PAUSED, true);
}

SimulationDisplay3D::~SimulationDisplay3D() {
    delete _editor;
    delete _scene;
    delete _renderer;
}

void SimulationDisplay3D::update(void* renderData) {
    const auto* data = static_cast<RenderData3D *>(renderData);

    // Initialize simulation if simulation is not paused but uninitialized (just started)
    if (!PROPERTY_B(PAUSED) && !PROPERTY_B(SIMULATION_INITIALIZED)) {
        _editor->initSimulation(data->grid);
    }

    _scene->getFluid()->setRenderData(data);

    _scene->update(Time::Get()->dt / 1000.f);
    _renderer->render(_scene);
}