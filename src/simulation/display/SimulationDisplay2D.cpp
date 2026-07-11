#include "simulation/display/SimulationDisplay2D.h"
#include "assets/Texture2D.h"
#include "views/DistributionView.h"
#include "views/FieldViewerView.h"
#include "views/Simulation2DView.h"
#include "views/StatsView.h"

SimulationDisplay2D::SimulationDisplay2D(const unsigned int width, const unsigned int height)
    : ISimulationDisplay(), _width(width), _height(height) {
    _renderer = new Renderer2D(width, height);
    _viewables["Simulation2D"] = new Simulation2DView(_renderer->getTexture2D()->get(), _width, _height);
    _viewables["Distribution"] = new DistributionView();
    _viewables["Stats"] = new StatsView();

    _viewables["FieldViewerVel"] = new FieldViewerView(Properties::VELOCITY, "FieldViewerVel");
    _viewables["FieldViewerRho"] = new FieldViewerView(Properties::DENSITY, "FieldViewerRho");
    _viewables["FieldViewerPres"] = new FieldViewerView(Properties::PRESSURE, "FieldViewerPres");
    _viewables["FieldViewerDiv"] = new FieldViewerView(Properties::DIVERGENCE, "FieldViewerDiv");
    _viewables["FieldViewerBeta"] = new FieldViewerView(Properties::BETA, "FieldViewerBeta");
    _viewables["FieldViewerGamma"] = new FieldViewerView(Properties::GAMMA, "FieldViewerGamma");
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