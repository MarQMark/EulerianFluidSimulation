#include "simulation/solver/CudaSolver.h"

#include "simulation/Properties.h"
#include "simulation/data/CudaGrid2D.h"

__global__ void cuda_jacobi_pass(const CudaGrid2D::CudaData *grid, float* result, const float* p);

JacobiSolverCuda2D::JacobiSolverCuda2D(const int width, const int height) : ISolverCuda2D(width, height) {
    cudaMalloc(&_gpu_tmp, width * height * sizeof(float));
}

JacobiSolverCuda2D::~JacobiSolverCuda2D() {
    cudaFree(_gpu_tmp);
}

float JacobiSolverCuda2D::solve(IGrid2D *_grid) {
    auto* grid = dynamic_cast<CudaGrid2D*>(_grid);
    if (!grid) return 0;

    const int iter = PROPERTY_I(MAX_STEPS);

    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 1 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 1 + blockSize.y - 1) / blockSize.y);

    for (int i = 0; i < iter; i++) {
        cuda_jacobi_pass<<<gridSize, blockSize>>>(grid->cudaData(), _gpu_tmp, grid->cudaData()->pressure);
        cuda_jacobi_pass<<<gridSize, blockSize>>>(grid->cudaData(), grid->cudaData()->pressure, _gpu_tmp);
    }

    return 0;

}

__device__ unsigned int cuda_idx2(const unsigned int x, const unsigned int y, const unsigned int w) {
    return y * w + x;
}

__device__ void sampleNeighbor(const int nx, const int ny, float& sum, int& nCount, const float* p, const CudaGrid2D::CudaData *grid) {
    if (nx < 0 || nx >= grid->width || ny < 0 || ny >= grid->height) return;
    if (nx == 0 || nx == grid->width-1 || ny == 0 || ny == grid->height-1) return;
    const unsigned int ni = cuda_idx2(nx, ny, grid->width);
    if (grid->boundary[ni] > 0.5) return;

    nCount++;
    if (grid->rho[ni] > 0.5){
        sum += p[ni];
    }
}
__global__ void cuda_jacobi_pass(const CudaGrid2D::CudaData *grid, float* result, const float* p) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width || y >= grid->height)
        return;

    const unsigned int i = cuda_idx2(x, y, w);

    if (grid->rho[i] <= 0.5 || grid->boundary[i] > .5) {
        result[i] = p[i];
        return;
    }

    float sum = 0.0;
    int nCount = 0;

    sampleNeighbor(x - 1, y, sum, nCount, p, grid);
    sampleNeighbor(x + 1, y, sum, nCount, p, grid);
    sampleNeighbor(x, y - 1, sum, nCount, p, grid);
    sampleNeighbor(x, y + 1, sum, nCount, p, grid);

    if(nCount == 0)
        result[i] = 0.0;
    else
        result[i] = (sum - grid->divergence[i]) / (float)nCount;
}