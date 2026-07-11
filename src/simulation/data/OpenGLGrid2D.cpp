#include "simulation/data/OpenGLGrid2D.h"

#include "glm/vec2.hpp"
#include "simulation/Properties.h"
#include "simulation/data/ScalField2D.h"
#include "simulation/data/VecField2D.h"

#define LOOP_2D(x, y, offset)  for (int y = offset; y < _height - offset; y++) \
                               for (int x = offset; x < _width  - offset; x++)

OpenGLGrid2D::OpenGLGrid2D(const unsigned width, const unsigned height) : IGrid2D(width, height) {
    _gpu_vel        = new StorageBuffer(_size * sizeof(glm::vec2));
    _gpu_tmp_vel     = new StorageBuffer(_size * sizeof(glm::vec2));

    _gpu_rho        = new StorageBuffer(_size * sizeof(float));
    _gpu_tmp_rho    = new StorageBuffer(_size * sizeof(float));
    _gpu_gamma      = new StorageBuffer(_size * sizeof(float));
    _gpu_new_gamma  = new StorageBuffer(_size * sizeof(float));
    _gpu_beta       = new StorageBuffer(_size * sizeof(float));
    _gpu_A          = new StorageBuffer(_size * sizeof(AdvCell) * 8); // 4 backward trace + 4 forward trace
    _gpu_counts     = new StorageBuffer(_size * sizeof(unsigned int));

    _gpu_divergence = new StorageBuffer(_size * sizeof(float));
    _gpu_pressure   = new StorageBuffer(_size * sizeof(float));

    _gpu_forces     = new StorageBuffer(_size * sizeof(glm::vec2));
    _gpu_boundary   = new StorageBuffer(_size * sizeof(float));

    _vel = new VecField2D(width, height);
    _rho = new ScalField2D(width, height);
    _forces = new VecField2D(width, height);
    _boundary = new ScalField2D(width, height);

    _gamma = new ScalField2D(width, height);
    _beta = new ScalField2D(width, height);

    _divergence = new ScalField2D(width, height);
    _pressure = new ScalField2D(width, height);

    *_gamma = 1.f;

    switch (PROPERTY(INIT_SHAPE, Properties::InitShape)) {
        case Properties::FULL:
            (*_rho) = 1.f;
            break;
        case Properties::HALF:
            for (int y = 0; y < _height / 2; y++)
                for (int x = 0; x < _width; x++)
                    (*_rho)(x, y) = 1.f;
            break;
        case Properties::SQUARE:
            if (_width < 50 || _height < 50)
                break;
            for (int y = 40; y < 50; y++)
                for (int x = 40; x < 50; x++)
                    (*_rho)(x, y) = 1.f;
            break;
        case Properties::EMPTY:
            (*_rho) = 0.f;
            break;
    }

    LOOP_2D(x, y, 0) {
        if (y == 0) (*_rho)(x, y) = 0.f;
        if (x == 0) (*_rho)(x, y) = 0.f;
        if (x == _width  - 1) (*_rho)(x, y) = 0.f;
        if (y == _height - 1) (*_rho)(x, y) = 0.f;

        if (y == 0) (*_boundary)(x, y) = 1.f;
        if (x == 0) (*_boundary)(x, y) = 1.f;
        if (x == _width  - 1) (*_boundary)(x, y) = 1.f;
        if (y == _height - 1) (*_boundary)(x, y) = 1.f;
    }

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    LOOP_2D(x, y, 0) {
        ADD_PROPERTY_F(TOTAL_RHO, (*_rho)(x, y));
    }

    upload();
}

OpenGLGrid2D::~OpenGLGrid2D() {
    delete _gpu_vel;
    delete _gpu_tmp_vel;
    delete _gpu_rho;
    delete _gpu_tmp_rho;
    delete _gpu_gamma;
    delete _gpu_new_gamma;
    delete _gpu_beta;
    delete _gpu_A;
    delete _gpu_counts;
    delete _gpu_divergence;
    delete _gpu_pressure;
    delete _gpu_forces;
    delete _gpu_boundary;

    delete _vel;
    delete _rho;
    delete _forces;
    delete _boundary;
    delete _gamma;
    delete _beta;
    delete _divergence;
    delete _pressure;
}

void OpenGLGrid2D::setDirty() {
    _dirty = true;
}

VecField2D& OpenGLGrid2D::vel() {
    if (_dirty) download();
    return *_vel;
}
ScalField2D& OpenGLGrid2D::rho() {
    if (_dirty) download();
    return *_rho;
}
ScalField2D & OpenGLGrid2D::gamma() {
    if (_dirty) download();
    return *_gamma;
}
ScalField2D & OpenGLGrid2D::beta() {
    if (_dirty) download();
    return *_beta;
}
ScalField2D & OpenGLGrid2D::divergence() {
    if (_dirty) download();
    return *_divergence;
}
ScalField2D & OpenGLGrid2D::pressure() {
    if (_dirty) download();
    return *_pressure;
}
VecField2D & OpenGLGrid2D::forces() {
    if (_dirty) download();
    return *_forces;
}
ScalField2D & OpenGLGrid2D::boundary() {
    if (_dirty) download();
    return *_boundary;
}

StorageBuffer & OpenGLGrid2D::gpuVel() const { return *_gpu_vel; }
StorageBuffer & OpenGLGrid2D::gpuTmpVel() const { return *_gpu_tmp_vel; }
StorageBuffer & OpenGLGrid2D::gpuRho() const { return *_gpu_rho; }
StorageBuffer & OpenGLGrid2D::gpuTmpRho() const { return *_gpu_tmp_rho; }
StorageBuffer & OpenGLGrid2D::gpuGamma() const { return *_gpu_gamma; }
StorageBuffer & OpenGLGrid2D::gpuNewGamma() const { return *_gpu_new_gamma; }
StorageBuffer & OpenGLGrid2D::gpuBeta() const { return *_gpu_beta; }
StorageBuffer & OpenGLGrid2D::gpuA() const { return *_gpu_A; }
StorageBuffer & OpenGLGrid2D::gpuCounts() const { return *_gpu_counts; }
StorageBuffer & OpenGLGrid2D::gpuDivergence() const { return *_gpu_divergence; }
StorageBuffer & OpenGLGrid2D::gpuPressure() const { return *_gpu_pressure; }
StorageBuffer & OpenGLGrid2D::gpuForces() const { return *_gpu_forces; }
StorageBuffer & OpenGLGrid2D::gpuBoundary() const { return *_gpu_boundary;}

void OpenGLGrid2D::upload() const {
    _gpu_vel->upload(_vel->data());
    _gpu_rho->upload(_rho->data());
    _gpu_boundary->upload(_boundary->data());
    _gpu_forces->upload(_forces->data());

    _gpu_divergence->upload(_divergence->data());
    _gpu_pressure->upload(_pressure->data());

    //_gpu_beta->upload(_beta->data());
    //_gpu_gamma->upload(_gamma->data());
}

void OpenGLGrid2D::download() {
    _gpu_vel->download(_vel->data());

    _gpu_rho->download(_rho->data());
    _gpu_boundary->download(_boundary->data());
    _gpu_forces->download(_forces->data());
    _gpu_divergence->download(_divergence->data());
    _gpu_pressure->download(_pressure->data());

    _gpu_beta->download(_beta->data());
    _gpu_gamma->download(_gamma->data());

    _dirty = false;
}

#undef LOOP_2D