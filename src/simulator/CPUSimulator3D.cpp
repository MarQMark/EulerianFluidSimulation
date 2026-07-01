#include "simulator/CPUSimulator3D.h"
#include "simulation/Grid3D.h"
#include "simulation/Properties.h"
#include "util/Profiler.h"

CPUSimulator3D::CPUSimulator3D(const unsigned int width, const unsigned int height, const  unsigned int depth) : ISimulator3D(width, height, depth) {
    _grid = new Grid3D(width, height, depth);
    _data = new SimulationDisplay3D::RenderData3D;
    _data->grid = _grid;
}

CPUSimulator3D::~CPUSimulator3D() {
    delete _grid;
}

void CPUSimulator3D::update() {
    if (PROPERTY_B(SIMULATION_GRID_RESIZED)) {
        delete _grid;
        _grid = new Grid3D(PROPERTY_I(WIDTH_3D), PROPERTY_I(HEIGHT_3D), PROPERTY_I(DEPTH_3D));
        _data->grid = _grid;

        SET_PROPERTY_B(SIMULATION_GRID_RESIZED, false);
    }
}

void* CPUSimulator3D::getRenderData() {
    return _data;
}

void CPUSimulator3D::vel_extrapolate() {
}

void CPUSimulator3D::rho_advect() {
}

void CPUSimulator3D::rho_sharpen() {
}

void CPUSimulator3D::add_force() {
}

void CPUSimulator3D::vel_advect() {
}

void CPUSimulator3D::diffuse() {
}

void CPUSimulator3D::project() {
}
