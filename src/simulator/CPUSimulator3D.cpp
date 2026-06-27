#include "simulator/CPUSimulator3D.h"

CPUSimulator3D::CPUSimulator3D(const unsigned int width, const unsigned int height, const  unsigned int depth)
    : ISimulator3D(width, height, depth) {
}

CPUSimulator3D::~CPUSimulator3D() {
}

void CPUSimulator3D::update() {
    //ISimulator3D::update();
}

void * CPUSimulator3D::getRenderData() {
    return nullptr;
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
