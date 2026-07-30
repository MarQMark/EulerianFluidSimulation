#include "simulation/display/SimulationDisplay2D.h"
#include "assets/Texture2D.h"
#include "views/DistributionView.h"
#include "views/FieldViewerView2D.h"
#include "views/Simulation2DView.h"
#include "views/StatsView.h"

SimulationDisplay2D::SimulationDisplay2D(const unsigned int width, const unsigned int height)
    : ISimulationDisplay(), _width(width), _height(height) {
    _renderer = new Renderer2D(width, height);
    _viewables["Simulation2D"] = new Simulation2DView(_width, _height);
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