#include "simulation/display/SimulationDisplay2D.h"
#include "assets/Texture2D.h"
#include "simulation/data/Scene2D.h"
#include "views/DistributionView.h"
#include "views/FieldViewerView2D.h"
#include "views/SceneView2D.h"
#include "views/Simulation2DView.h"

SimulationDisplay2D::SimulationDisplay2D(const unsigned int width, const unsigned int height)
    : ISimulationDisplay(), _width(width), _height(height) {
    _renderer = new Renderer2D(width, height);
    _scene = new Scene2D();

    _viewables["Simulation2D"] = new Simulation2DView(_width, _height);
    _viewables["Scene2D"] = new SceneView2D(_scene);
    _viewables["Distribution"] = new DistributionView();

    _viewables["FieldViewerVel"] = new FieldViewerView2D(Properties::VELOCITY, "FieldViewerVel");
    _viewables["FieldViewerRho"] = new FieldViewerView2D(Properties::DENSITY, "FieldViewerRho");
    _viewables["FieldViewerPres"] = new FieldViewerView2D(Properties::PRESSURE, "FieldViewerPres");
    _viewables["FieldViewerDiv"] = new FieldViewerView2D(Properties::DIVERGENCE, "FieldViewerDiv");
    _viewables["FieldViewerBeta"] = new FieldViewerView2D(Properties::BETA, "FieldViewerBeta");
    _viewables["FieldViewerGamma"] = new FieldViewerView2D(Properties::GAMMA, "FieldViewerGamma");
}

SimulationDisplay2D::~SimulationDisplay2D() {
    delete _renderer;
}

void SimulationDisplay2D::render(void* renderData) {
    _renderer->render(renderData);
}

void SimulationDisplay2D::reset(void *simulationData) {
    const auto* data = static_cast<RenderData2D*>(simulationData);
    _scene->resetScene(data->grid);
}
