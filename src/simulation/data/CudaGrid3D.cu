#include "simulation/data/CudaGrid3D.h"

#include <algorithm>

#include "simulation/Properties.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"

#include <cuda_runtime.h>

#define LOOP_3D(x, y, z, offset)                        \
      for (int z = offset; z < _depth  - (offset); z++)   \
      for (int y = offset; y < _height - (offset); y++)   \
      for (int x = offset; x < _width  - (offset); x++)   \

CudaGrid3D::CudaGrid3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : IGrid3D(width, height, depth) {
    gpu_alloc(&_cuda_data.vel,          _size * sizeof(glm::vec3));
    gpu_alloc(&_cuda_data.tmp_vel,      _size * sizeof(glm::vec3));
    gpu_alloc(&_cuda_data.rho,          _size * sizeof(float));
    gpu_alloc(&_cuda_data.tmp_rho,      _size * sizeof(float));
    gpu_alloc(&_cuda_data.gamma,        _size * sizeof(float));
    gpu_alloc(&_cuda_data.new_gamma,    _size * sizeof(float));
    gpu_alloc(&_cuda_data.beta,         _size * sizeof(float));
    gpu_alloc(&_cuda_data.A_bwd,       _size * sizeof(AdvCell) * 8);
    gpu_alloc(&_cuda_data.A_fwd,       _size * sizeof(AdvCell) * 8);
    gpu_alloc(&_cuda_data.counts_bwd,   _size * sizeof(unsigned int));
    gpu_alloc(&_cuda_data.counts_fwd,   _size * sizeof(unsigned int));
    gpu_alloc(&_cuda_data.divergence,   _size * sizeof(float));
    gpu_alloc(&_cuda_data.pressure,     _size * sizeof(float));
    gpu_alloc(&_cuda_data.forces,       _size * sizeof(glm::vec3));
    gpu_alloc(&_cuda_data.boundary,     _size * sizeof(float));

    cudaMemset(_cuda_data.vel,          0, _size * sizeof(glm::vec3));
    cudaMemset(_cuda_data.tmp_vel,      0, _size * sizeof(glm::vec3));
    cudaMemset(_cuda_data.rho,          0, _size * sizeof(float));
    cudaMemset(_cuda_data.tmp_rho,      0, _size * sizeof(float));
    cudaMemset(_cuda_data.gamma,        0, _size * sizeof(float));
    cudaMemset(_cuda_data.new_gamma,    0, _size * sizeof(float));
    cudaMemset(_cuda_data.beta,         0, _size * sizeof(float));
    cudaMemset(_cuda_data.A_bwd,        0, _size * sizeof(AdvCell) * 8);
    cudaMemset(_cuda_data.A_fwd,        0, _size * sizeof(AdvCell) * 8);
    cudaMemset(_cuda_data.counts_bwd,   0, _size * sizeof(unsigned int));
    cudaMemset(_cuda_data.counts_fwd,   0, _size * sizeof(unsigned int));
    cudaMemset(_cuda_data.divergence,   0, _size * sizeof(float));
    cudaMemset(_cuda_data.pressure,     0, _size * sizeof(float));
    cudaMemset(_cuda_data.forces,       0, _size * sizeof(glm::vec3));
    cudaMemset(_cuda_data.boundary,     0, _size * sizeof(float));

    _cuda_data.width = width;
    _cuda_data.height = height;
    _cuda_data.depth = depth;

    gpu_alloc(&_gpu_cuda_data, sizeof(CudaData));
    gpu_copy_H2D(_gpu_cuda_data, &_cuda_data, sizeof(CudaData));

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
        if (z == 0) (*_rho)(x, y, z) = 0.f;
        if (y == 0) (*_rho)(x, y, z) = 0.f;
        if (x == 0) (*_rho)(x, y, z) = 0.f;
        if (x == _width  - 1) (*_rho)(x, y, z) = 0.f;
        if (y == _height - 1) (*_rho)(x, y, z) = 0.f;
        if (z == _depth  - 1) (*_rho)(x, y, z) = 0.f;

        if (z == 0) (*_boundary)(x, y, z) = 1.f;
        if (y == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == 0) (*_boundary)(x, y, z) = 1.f;
        if (x == _width  - 1) (*_boundary)(x, y, z) = 1.f;
        if (y == _height - 1) (*_boundary)(x, y, z) = 1.f;
        if (z == _depth  - 1) (*_boundary)(x, y, z) = 1.f;
    }

    SET_PROPERTY_F(TOTAL_RHO, 0.f);
    LOOP_3D(x, y, z, 0) {
        ADD_PROPERTY_F(TOTAL_RHO, (*_rho)(x, y, z));
    }

    upload();
}

CudaGrid3D::~CudaGrid3D() {
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

    cudaFree(_gpu_cuda_data);

    delete _vel;
    delete _rho;
    delete _forces;
    delete _boundary;
    delete _gamma;
    delete _beta;
    delete _divergence;
    delete _pressure;
}

void CudaGrid3D::setDirty() {
    _dirty = true;
}

VecField3D& CudaGrid3D::vel() {
    if (_dirty) download();
    return *_vel;
}
ScalField3D& CudaGrid3D::rho() {
    if (_dirty) download();
    return *_rho;
}
ScalField3D& CudaGrid3D::gamma() {
    if (_dirty) download();
    return *_gamma;
}
ScalField3D& CudaGrid3D::beta() {
    if (_dirty) download();
    return *_beta;
}
ScalField3D& CudaGrid3D::divergence() {
    if (_dirty) download();
    return *_divergence;
}
ScalField3D& CudaGrid3D::pressure() {
    if (_dirty) download();
    return *_pressure;
}
VecField3D& CudaGrid3D::forces() {
    if (_dirty) download();
    return *_forces;
}
ScalField3D& CudaGrid3D::boundary() {
    if (_dirty) download();
    return *_boundary;
}

struct CudaGrid3D::CudaData* CudaGrid3D::cudaData() const {
    return _gpu_cuda_data;
}

struct CudaGrid3D::CudaData * CudaGrid3D::cudaDataCPU(){
    return &_cuda_data;
}

void CudaGrid3D::gpu_alloc(void **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_alloc(float **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_alloc(unsigned int **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
       "cudaMalloc failed: %s (%s)\n",
       cudaGetErrorName(cudaStatus),
       cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_alloc(glm::vec3 **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_alloc(AdvCell **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_alloc(CudaData **ptr, const size_t size) {
    const cudaError_t cudaStatus = cudaMalloc(ptr, size);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMalloc failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_copy_H2D(void *dest, const void *src, const size_t count) {
    const cudaError_t cudaStatus = cudaMemcpy(dest, src, count, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMemcpy(HostToDevice) failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}
void CudaGrid3D::gpu_copy_D2H(void *dest, const void *src, const size_t count) {
    const cudaError_t cudaStatus = cudaMemcpy(dest, src, count, cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess)
        fprintf(stderr,
               "cudaMemcpy(DeviceToHost) failed: %s (%s)\n",
               cudaGetErrorName(cudaStatus),
               cudaGetErrorString(cudaStatus));
}

void CudaGrid3D::upload() const {
    gpu_copy_H2D(_cuda_data.vel,        _vel->data(),       _size * sizeof(glm::vec3));
    gpu_copy_H2D(_cuda_data.rho,        _rho->data(),       _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.gamma,      _gamma->data(),     _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.beta,       _beta->data(),      _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.divergence, _divergence->data(),_size * sizeof(float));
    gpu_copy_H2D(_cuda_data.pressure,   _pressure->data(),  _size * sizeof(float));
    gpu_copy_H2D(_cuda_data.forces,     _forces->data(),    _size * sizeof(glm::vec3));
    gpu_copy_H2D(_cuda_data.boundary,   _boundary->data(),  _size * sizeof(float));
}

void CudaGrid3D::download() {
    gpu_copy_D2H(_vel->data(),       _cuda_data.vel,        _size * sizeof(glm::vec3));
    gpu_copy_D2H(_rho->data(),       _cuda_data.rho,        _size * sizeof(float));
    gpu_copy_D2H(_gamma->data(),     _cuda_data.gamma,      _size * sizeof(float));
    gpu_copy_D2H(_beta->data(),      _cuda_data.beta,       _size * sizeof(float));
    gpu_copy_D2H(_divergence->data(),_cuda_data.divergence, _size * sizeof(float));
    gpu_copy_D2H(_pressure->data(),  _cuda_data.pressure,   _size * sizeof(float));
    gpu_copy_D2H(_forces->data(),    _cuda_data.forces,     _size * sizeof(glm::vec3));
    gpu_copy_D2H(_boundary->data(),  _cuda_data.boundary,   _size * sizeof(float));

    _dirty = false;
}

void CudaGrid3D::clearA() const {
    cudaMemset(_cuda_data.beta,	  	  0,_size * sizeof(float));
    cudaMemset(_cuda_data.A_bwd,      0,_size * sizeof(AdvCell) * 8);
    cudaMemset(_cuda_data.A_fwd,      0,_size * sizeof(AdvCell) * 8);
    cudaMemset(_cuda_data.counts_bwd, 0,_size * sizeof(unsigned int));
    cudaMemset(_cuda_data.counts_fwd, 0,_size * sizeof(unsigned int));
}

void CudaGrid3D::clearProj() const {
    cudaMemset(_cuda_data.pressure,	  0,_size * sizeof(float));
    cudaMemset(_cuda_data.divergence, 0,_size * sizeof(float));
}

#undef LOOP_2D
