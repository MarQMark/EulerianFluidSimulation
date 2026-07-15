#include "simulation/data/CudaGrid2D.h"

#include <algorithm>

#include "simulation/Properties.h"
#include "simulation/data/ScalField2D.h"
#include "simulation/data/VecField2D.h"

#include <cuda_runtime.h>

#define LOOP_2D(x, y, offset)  for (int y = offset; y < _height - offset; y++) \
                               for (int x = offset; x < _width  - offset; x++)

CudaGrid2D::CudaGrid2D(const unsigned width, const unsigned height) : IGrid2D(width, height) {
    gpu_alloc(&_cuda_data.vel,          _size * sizeof(glm::vec2));
    gpu_alloc(&_cuda_data.tmp_vel,      _size * sizeof(glm::vec2));
    gpu_alloc(&_cuda_data.rho,          _size * sizeof(float));
    gpu_alloc(&_cuda_data.tmp_rho,      _size * sizeof(float));
    gpu_alloc(&_cuda_data.gamma,        _size * sizeof(float));
    gpu_alloc(&_cuda_data.new_gamma,    _size * sizeof(float));
    gpu_alloc(&_cuda_data.beta,         _size * sizeof(float));
    gpu_alloc(&_cuda_data.A_bwd,       _size * sizeof(AdvCell) * 4);
    gpu_alloc(&_cuda_data.A_fwd,       _size * sizeof(AdvCell) * 4);
    gpu_alloc(&_cuda_data.counts_bwd,   _size * sizeof(unsigned int));
    gpu_alloc(&_cuda_data.counts_fwd,   _size * sizeof(unsigned int));
    gpu_alloc(&_cuda_data.divergence,   _size * sizeof(float));
    gpu_alloc(&_cuda_data.pressure,     _size * sizeof(float));
    gpu_alloc(&_cuda_data.forces,       _size * sizeof(glm::vec2));
    gpu_alloc(&_cuda_data.boundary,     _size * sizeof(float));

    _cuda_data.width = width;
    _cuda_data.height = height;

    gpu_alloc(&_gpu_cuda_data, sizeof(CudaData));
    gpu_copy_H2D(_gpu_cuda_data, &_cuda_data, sizeof(CudaData));

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

CudaGrid2D::~CudaGrid2D() {
    cudaFree(_cuda_data.vel);
    cudaFree(_cuda_data.tmp_vel);
    cudaFree(_cuda_data.rho);
    cudaFree(_cuda_data.tmp_rho);
    cudaFree(_cuda_data.gamma);
    cudaFree(_cuda_data.new_gamma);
    cudaFree(_cuda_data.beta);
    cudaFree(_cuda_data.A_bwd);
    cudaFree(_cuda_data.A_fwd);
    cudaFree(_cuda_data.counts_bwd);
    cudaFree(_cuda_data.counts_fwd);
    cudaFree(_cuda_data.divergence);
    cudaFree(_cuda_data.pressure);
    cudaFree(_cuda_data.forces);
    cudaFree(_cuda_data.boundary);

    delete _vel;
    delete _rho;
    delete _forces;
    delete _boundary;
    delete _gamma;
    delete _beta;
    delete _divergence;
    delete _pressure;
}

void CudaGrid2D::setDirty() {
    _dirty = true;
}

VecField2D& CudaGrid2D::vel() {
    if (_dirty) download();
    return *_vel;
}
ScalField2D& CudaGrid2D::rho() {
    if (_dirty) download();
    return *_rho;
}
ScalField2D & CudaGrid2D::gamma() {
    if (_dirty) download();
    return *_gamma;
}
ScalField2D & CudaGrid2D::beta() {
    if (_dirty) download();
    return *_beta;
}
ScalField2D & CudaGrid2D::divergence() {
    if (_dirty) download();
    return *_divergence;
}
ScalField2D & CudaGrid2D::pressure() {
    if (_dirty) download();
    return *_pressure;
}
VecField2D & CudaGrid2D::forces() {
    if (_dirty) download();
    return *_forces;
}
ScalField2D & CudaGrid2D::boundary() {
    if (_dirty) download();
    return *_boundary;
}

struct CudaGrid2D::CudaData* CudaGrid2D::cudaData() const {
    return _gpu_cuda_data;
}

struct CudaGrid2D::CudaData * CudaGrid2D::cudaDataCPU(){
    return &_cuda_data;
}

void CudaGrid2D::gpu_alloc(void **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_alloc(float **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_alloc(unsigned int **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
       "cudaMalloc failed: %s (%s)\n",
       cudaGetErrorName(cudaStatus),
       cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_alloc(glm::vec2 **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_alloc(AdvCell **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_alloc(CudaData **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_copy_H2D(void *dest, const void *src, const size_t count) {
    const cudaError_t cudaStatus = cudaMemcpy(dest, src, count, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMemcpy(HostToDevice) failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid2D::gpu_copy_D2H(void *dest, const void *src, const size_t count) {
    const cudaError_t cudaStatus = cudaMemcpy(dest, src, count, cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMemcpy(DeviceToHost) failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}

void CudaGrid2D::upload() const {
    gpu_copy_H2D(_cuda_data.vel,        _vel->data(),       _size * sizeof(glm::vec2));
    gpu_copy_H2D(_cuda_data.rho,        _rho->data(),       _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.gamma,      _gamma->data(),     _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.beta,       _beta->data(),      _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.divergence, _divergence->data(),_size * sizeof(float));
    gpu_copy_H2D(_cuda_data.pressure,   _pressure->data(),  _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.forces,     _forces->data(),    _size * sizeof(glm::vec2));
    gpu_copy_H2D(_cuda_data.boundary,   _boundary->data(),  _size * sizeof(float));
}

void CudaGrid2D::download() {
    gpu_copy_D2H(_vel->data(),       _cuda_data.vel,        _size * sizeof(glm::vec2));
    gpu_copy_D2H(_rho->data(),       _cuda_data.rho,        _size * sizeof(float));
    gpu_copy_D2H(_gamma->data(),     _cuda_data.gamma,      _size * sizeof(float));
    gpu_copy_D2H(_beta->data(),      _cuda_data.beta,       _size * sizeof(float));
    gpu_copy_D2H(_divergence->data(),_cuda_data.divergence, _size * sizeof(float));
    gpu_copy_D2H(_pressure->data(),  _cuda_data.pressure,   _size * sizeof(float));
    gpu_copy_D2H(_forces->data(),    _cuda_data.forces,     _size * sizeof(glm::vec2));
    gpu_copy_D2H(_boundary->data(),  _cuda_data.boundary,   _size * sizeof(float));

    _dirty = false;
}

void CudaGrid2D::partialClear() const {
    cudaMemset(_cuda_data.beta,	  	  0,_size * sizeof(float));
    cudaMemset(_cuda_data.A_bwd,      0,_size * sizeof(AdvCell) * 4);
    cudaMemset(_cuda_data.A_fwd,      0,_size * sizeof(AdvCell) * 4);
    cudaMemset(_cuda_data.counts_bwd, 0,_size * sizeof(unsigned int));
    cudaMemset(_cuda_data.counts_fwd, 0,_size * sizeof(unsigned int));
}

#undef LOOP_2D
