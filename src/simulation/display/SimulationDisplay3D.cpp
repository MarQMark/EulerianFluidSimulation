//
// Created by ruut on 7/1/26.
//

#include "simulation/display/SimulationDisplay3D.h"
#include "core/Editor.h"
#include "renderer/Renderer3D.h"
#include "simulation/Properties.h"
#include "simulation/data/IGrid.h"
#include "simulation/scene/nodes/FluidNode.h"
#include "util/Time.h"
#include "views/FieldViewerView3D.h"
#include "views/NodePropertiesView.h"
#include "views/QueryDataView.h"
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
    _viewables["QueryData"] = new QueryDataView(_editor);

    _viewables["FieldViewerVel"] = new FieldViewerView3D(Properties::VELOCITY, "FieldViewerVel");
    _viewables["FieldViewerRho"] = new FieldViewerView3D(Properties::DENSITY, "FieldViewerRho");
    _viewables["FieldViewerPres"] = new FieldViewerView3D(Properties::PRESSURE, "FieldViewerPres");
    _viewables["FieldViewerDiv"] = new FieldViewerView3D(Properties::DIVERGENCE, "FieldViewerDiv");
    _viewables["FieldViewerBeta"] = new FieldViewerView3D(Properties::BETA, "FieldViewerBeta");
    _viewables["FieldViewerGamma"] = new FieldViewerView3D(Properties::GAMMA, "FieldViewerGamma");

    SET_PROPERTY_B(PAUSED, true);
}

SimulationDisplay3D::~SimulationDisplay3D() {
    delete _editor;
    delete _scene;
    delete _renderer;
}

void SimulationDisplay3D::update(void* simulationData) {
    const auto* data = static_cast<RenderData3D *>(simulationData);
    _editor->updateForceFields(data->grid->forces());
    _scene->update(Time::Get()->dt / 1000.f);
}

void SimulationDisplay3D::render(void* renderData) {
    const auto* data = static_cast<RenderData3D *>(renderData);
    _scene->getFluid()->setRenderData(data);
    _renderer->render(_scene);
}

void SimulationDisplay3D::reset(void* simulationData) {
    const auto* data = static_cast<RenderData3D *>(simulationData);
    _editor->init(data->grid);
    _scene->getFluid()->setRenderData(nullptr);
}