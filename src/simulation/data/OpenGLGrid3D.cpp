#include "simulation/data/OpenGLGrid3D.h"

#include <algorithm>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "simulation/Properties.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"

#define LOOP_3D(x, y, z, offset)                        \
    for (int z = offset; z < _depth  - (offset); z++)   \
    for (int y = offset; y < _height - (offset); y++)   \
    for (int x = offset; x < _width  - (offset); x++)   \

OpenGLGrid3D::OpenGLGrid3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : IGrid3D(width, height, depth) {
    _gpu_vel        = new StorageBuffer(_size * sizeof(glm::vec4));
    _gpu_tmp_vel     = new StorageBuffer(_size * sizeof(glm::vec4));

    _gpu_rho        = new StorageBuffer(_size * sizeof(float));
    _gpu_tmp_rho    = new StorageBuffer(_size * sizeof(float));
    _gpu_gamma      = new StorageBuffer(_size * sizeof(float));
    _gpu_new_gamma  = new StorageBuffer(_size * sizeof(float));
    _gpu_beta       = new StorageBuffer(_size * sizeof(float));
    _gpu_A_bwd      = new StorageBuffer(_size * sizeof(AdvCell) * 8);
    _gpu_A_fwd      = new StorageBuffer(_size * sizeof(AdvCell) * 8);
    _gpu_counts_bwd = new StorageBuffer(_size * sizeof(unsigned int));
    _gpu_counts_fwd = new StorageBuffer(_size * sizeof(unsigned int));

    _gpu_divergence = new StorageBuffer(_size * sizeof(float));
    _gpu_pressure   = new StorageBuffer(_size * sizeof(float));

    _gpu_forces     = new StorageBuffer(_size * sizeof(glm::vec4));
    _gpu_boundary   = new StorageBuffer(_size * sizeof(float));

    _vel = new VecField3D(width, height, depth);
    _rho = new ScalField3D(width, height, depth);
    _forces = new VecField3D(width, height, depth);
    _boundary = new ScalField3D(width, height, depth);

    _gamma = new ScalField3D(width, height, depth);
    _beta = new ScalField3D(width, height, depth);

    _divergence = new ScalField3D(width, height, depth);
    _pressure = new ScalField3D(width, height, depth);

    *_gamma = 1.f;

    LOOP_3D(x, y, z, 0) {
        if (y == 0) (*_rho)(x, y, z) = 0.f;
        if (x == 0) (*_rho)(x, y, z) = 0.f;
        if (x == _width  - 1) (*_rho)(x, y, z) = 0.f;
        if (y == _height - 1) (*_rho)(x, y, z) = 0.f;

        if (y == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == _width  - 1) (*_boundary)(x, y, z) = 1.f;
        if (y == _height - 1) (*_boundary)(x, y, z) = 1.f;
    }

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    LOOP_3D(x, y, z, 0) {
        ADD_PROPERTY_F(TOTAL_RHO, (*_rho)(x, y, z));
    }

    upload();
}

OpenGLGrid3D::~OpenGLGrid3D() {
    delete _gpu_vel;
    delete _gpu_tmp_vel;
    delete _gpu_rho;
    delete _gpu_tmp_rho;
    delete _gpu_gamma;
    delete _gpu_new_gamma;
    delete _gpu_beta;
    delete _gpu_A_bwd;
    delete _gpu_A_fwd;
    delete _gpu_counts_bwd;
    delete _gpu_counts_fwd;
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

void OpenGLGrid3D::setDirty() {
    _dirty = true;
}

VecField3D& OpenGLGrid3D::vel() {
    if (_dirty) download();
    return *_vel;
}
ScalField3D& OpenGLGrid3D::rho() {
    if (_dirty) download();
    return *_rho;
}
ScalField3D & OpenGLGrid3D::gamma() {
    if (_dirty) download();
    return *_gamma;
}
ScalField3D & OpenGLGrid3D::beta() {
    if (_dirty) download();
    return *_beta;
}
ScalField3D & OpenGLGrid3D::divergence() {
    if (_dirty) download();
    return *_divergence;
}
ScalField3D & OpenGLGrid3D::pressure() {
    if (_dirty) download();
    return *_pressure;
}
VecField3D & OpenGLGrid3D::forces() {
    if (_dirty) download();
    return *_forces;
}
ScalField3D & OpenGLGrid3D::boundary() {
    if (_dirty) download();
    return *_boundary;
}

StorageBuffer & OpenGLGrid3D::gpuVel() const { return *_gpu_vel; }
StorageBuffer & OpenGLGrid3D::gpuTmpVel() const { return *_gpu_tmp_vel; }
StorageBuffer & OpenGLGrid3D::gpuRho() const { return *_gpu_rho; }
StorageBuffer & OpenGLGrid3D::gpuTmpRho() const { return *_gpu_tmp_rho; }
StorageBuffer & OpenGLGrid3D::gpuGamma() const { return *_gpu_gamma; }
StorageBuffer & OpenGLGrid3D::gpuNewGamma() const { return *_gpu_new_gamma; }
StorageBuffer & OpenGLGrid3D::gpuBeta() const { return *_gpu_beta; }
StorageBuffer & OpenGLGrid3D::gpuDivergence() const { return *_gpu_divergence; }
StorageBuffer & OpenGLGrid3D::gpuPressure() const { return *_gpu_pressure; }
StorageBuffer & OpenGLGrid3D::gpuForces() const { return *_gpu_forces; }
StorageBuffer & OpenGLGrid3D::gpuBoundary() const { return *_gpu_boundary;}

void OpenGLGrid3D::bindAll() const {
    _gpu_vel       ->bind(0);
    _gpu_tmp_vel   ->bind(1);
    _gpu_rho       ->bind(2);
    _gpu_tmp_rho   ->bind(3);
    _gpu_gamma     ->bind(4);
    _gpu_new_gamma ->bind(5);
    _gpu_beta      ->bind(6);
    _gpu_A_bwd     ->bind(7);
    _gpu_A_fwd     ->bind(8);
    _gpu_counts_bwd->bind(9);
    _gpu_counts_fwd->bind(10);
    _gpu_divergence->bind(11);
    _gpu_pressure  ->bind(12);
    _gpu_forces    ->bind(13);
    _gpu_boundary  ->bind(14);
}

void OpenGLGrid3D::clearA() const {
    _gpu_beta->clear();
    _gpu_A_bwd->clear();
    _gpu_A_fwd->clear();
    _gpu_counts_bwd->clear();
    _gpu_counts_fwd->clear();
}

void OpenGLGrid3D::upload() const {
    auto* data = (glm::vec4*)malloc(_size * sizeof(glm::vec4));
    for (int i = 0; i < _size; i++) {
        data[i].x = _vel->data()[i].x;
        data[i].y = _vel->data()[i].y;
        data[i].z = _vel->data()[i].z;
    }
    _gpu_vel->upload(data);
    for (int i = 0; i < _size; i++) {
        data[i].x = _forces->data()[i].x;
        data[i].y = _forces->data()[i].y;
        data[i].z = _forces->data()[i].z;
    }
    _gpu_forces->upload(data);
    free(data);

    //_gpu_vel->upload(_vel->data());
    _gpu_rho->upload(_rho->data());
    _gpu_boundary->upload(_boundary->data());
    //_gpu_forces->upload(_forces->data());
    _gpu_divergence->upload(_divergence->data());
    _gpu_pressure->upload(_pressure->data());
}

void OpenGLGrid3D::download() {
    auto* data = (glm::vec4*)malloc(_size * sizeof(glm::vec4));
    _gpu_vel->download(data);
    for (int i = 0; i < _size; i++) {
        _vel->data()[i].x = data[i].x;
        _vel->data()[i].y = data[i].y;
        _vel->data()[i].z = data[i].z;
    }
    _gpu_forces->download(data);
    for (int i = 0; i < _size; i++) {
        _forces->data()[i].x = data[i].x;
        _forces->data()[i].y = data[i].y;
        _forces->data()[i].z = data[i].z;
    }
    free(data);

    //_gpu_vel->download(_vel->data());
    _gpu_rho->download(_rho->data());
    _gpu_boundary->download(_boundary->data());
    //_gpu_forces->download(_forces->data());
    _gpu_divergence->download(_divergence->data());
    _gpu_pressure->download(_pressure->data());

    _gpu_beta->download(_beta->data());
    _gpu_gamma->download(_gamma->data());

    _dirty = false;
}

#undef LOOP_3D