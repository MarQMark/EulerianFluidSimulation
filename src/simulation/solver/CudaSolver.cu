#include "simulation/solver/CudaSolver.h"

#include "simulation/Properties.h"
#include "simulation/data/CudaGrid2D.h"
#include "simulation/data/CudaGrid3D.h"
#include "util/CudaUtil.h"

// #################################### Helper Methods ################################################
__device__ void cuda_sample_neighbor_2D(const int nx, const int ny, float& sum, int& nCount, const float* p, const CudaGrid2D::CudaData *grid) {
    //if (nx < 0 || nx >= grid->width || ny < 0 || ny >= grid->height)
    //    return;
    //if (nx == 0 || nx == grid->width-1 || ny == 0 || ny == grid->height-1)
    //    return;

    const unsigned int ni = cuda_idx_2D(nx, ny, grid->width);
    if (grid->boundary[ni] > 0.5f)
        return;
    nCount++;

    if (grid->rho[ni] > 0.5){
        sum += p[ni];
    }
}
__device__ void cuda_sample_neighbor_3D(const int nx, const int ny, const int nz, float& sum, int& nCount, const float* p, const CudaGrid3D::CudaData *grid) {
    if (nx < 0 || nx >= grid->width || ny < 0 || ny >= grid->height || nz < 0 || nz >= grid->depth)
        return;
    if (nx == 0 || nx == grid->width-1 || ny == 0 || ny == grid->height-1 || nz == 0 || nz == grid->depth-1)
        return;

    const unsigned int ni = cuda_idx_3D(nx, ny, nz, grid->width, grid->height);
    if (grid->boundary[ni] > 0.5f)
        return;
    nCount++;

    if (grid->rho[ni] > 0.5){
        sum += p[ni];
    }
}

// #################################### Jacobi Method 2D ################################################
__global__ void cuda_jacobi_pass_2D(const CudaGrid2D::CudaData *grid, float* result, const float* p);

JacobiSolverCuda2D::JacobiSolverCuda2D(const unsigned int width, const unsigned int height)
    : ISolverCuda2D(width, height) {
    cudaMalloc(&_gpu_tmp, _size * sizeof(float));
}

JacobiSolverCuda2D::~JacobiSolverCuda2D() {
    cudaFree(_gpu_tmp);
}

float JacobiSolverCuda2D::solve(IGrid2D *_grid) {
    auto* grid = dynamic_cast<CudaGrid2D*>(_grid);
    if (!grid) return 0;

    const int iter = (PROPERTY_I(MAX_STEPS) / 2);

    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 1 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 1 + blockSize.y - 1) / blockSize.y);

    for (int i = 0; i < iter; i++) {
        cuda_jacobi_pass_2D<<<gridSize, blockSize>>>(grid->cudaData(), _gpu_tmp, grid->cudaDataCPU()->pressure);
        cuda_jacobi_pass_2D<<<gridSize, blockSize>>>(grid->cudaData(), grid->cudaDataCPU()->pressure, _gpu_tmp);
    }

    return 0;
}

__global__ void cuda_jacobi_pass_2D(const CudaGrid2D::CudaData *grid, float* result, const float* p) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    const unsigned int w = grid->width;

    if (x >= grid->width || y >= grid->height)
        return;

    const unsigned int i = cuda_idx_2D(x, y, w);

    if (grid->rho[i] <= 0.5 || grid->boundary[i] > .5) {
        result[i] = p[i];
        return;
    }

    float sum = 0.0;
    int nCount = 0;

    cuda_sample_neighbor_2D(x - 1, y, sum, nCount, p, grid);
    cuda_sample_neighbor_2D(x + 1, y, sum, nCount, p, grid);
    cuda_sample_neighbor_2D(x, y - 1, sum, nCount, p, grid);
    cuda_sample_neighbor_2D(x, y + 1, sum, nCount, p, grid);

    if(nCount == 0)
        result[i] = 0.0;
    else
        result[i] = (sum - grid->divergence[i]) / (float)nCount;
}

// #################################### Conjugate Gradient 2D ################################################

static constexpr int CG_BLOCK   = 256;
static constexpr int CG_NBLOCKS = 128;   // block count for reductions

__global__ static void cuda_cg_mat_mul_2D(
    const CudaGrid2D::CudaData* __restrict__ grid,
    float* __restrict__ result, const float* __restrict__ x)
{
    const unsigned int gx = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gy = blockIdx.y * blockDim.y + threadIdx.y;
    const unsigned int w = grid->width;
    const unsigned int h = grid->height;
    if (gx >= w || gy >= h) return;

    const unsigned int i = cuda_idx_2D(gx, gy, w);
    if (grid->rho[i] <= 0.5f || grid->boundary[i] > 0.5f) {
        result[i] = x[i];
        return;
    }

    float sum  = 0.f;
    int nCount = 0;
    cuda_sample_neighbor_2D(gx - 1, gy, sum, nCount, x, grid);
    cuda_sample_neighbor_2D(gx + 1, gy, sum, nCount, x, grid);
    cuda_sample_neighbor_2D(gx, gy - 1, sum, nCount, x, grid);
    cuda_sample_neighbor_2D(gx, gy + 1, sum, nCount, x, grid);

    if (nCount == 0) { result[i] = x[i]; return; }   // pin this cell: L behaves like identity here
    result[i] = sum - ((float)nCount + 1e-2f) * x[i];
}

// a += alpha * b
__global__ static void cuda_cg_axpy(float* a, const float* b, const float alpha, const unsigned int n) {
    const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) a[i] += alpha * b[i];
}

// a = alpha * a + b
__global__ static void cuda_cg_xpay(float* a, const float* b, const float alpha, const unsigned int n) {
    const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) a[i] = alpha * a[i] + b[i];
}

// dst = src
__global__ static void cuda_cg_copy(float* dst, const float* src, const unsigned int n) {
    const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) dst[i] = src[i];
}

// a -= b
__global__ static void cuda_cg_sub(float* a, const float* b, const unsigned int n) {
    const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) a[i] -= b[i];
}

__global__ static void cuda_cg_dot_pass1(
    const float* __restrict__ a, const float* __restrict__ b,
    float* __restrict__ partial, const unsigned int n)
{
    __shared__ float sdata[CG_BLOCK];
    const int tid = threadIdx.x;

    float acc = 0.f;
    for (int i = blockIdx.x * CG_BLOCK + tid; i < n; i += CG_NBLOCKS * CG_BLOCK)
        acc += a[i] * b[i];
    sdata[tid] = acc;
    __syncthreads();

    if (CG_BLOCK >= 512) { if (tid < 256) sdata[tid] += sdata[tid+256]; __syncthreads(); }
    if (CG_BLOCK >= 256) { if (tid < 128) sdata[tid] += sdata[tid+128]; __syncthreads(); }
    if (CG_BLOCK >= 128) { if (tid < 64)  sdata[tid] += sdata[tid+64];  __syncthreads(); }
    // sdata[0..63] are now valid. Finish within one warp — no __syncthreads needed.
    if (tid < 32) {
        float v = sdata[tid] + sdata[tid + 32];   // 64→32, safe: __syncthreads above
        v += __shfl_down_sync(0xffffffff, v, 16);
        v += __shfl_down_sync(0xffffffff, v,  8);
        v += __shfl_down_sync(0xffffffff, v,  4);
        v += __shfl_down_sync(0xffffffff, v,  2);
        v += __shfl_down_sync(0xffffffff, v,  1);
        if (tid == 0) partial[blockIdx.x] = v;
    }
}

__global__ static void cuda_cg_dot_pass2(const float* __restrict__ partial, float* __restrict__ result) {
    __shared__ float sdata[CG_NBLOCKS];    // 128 elements
    const int tid = threadIdx.x;
    sdata[tid] = partial[tid];
    __syncthreads();

    if (tid < 64) { sdata[tid] += sdata[tid + 64]; __syncthreads(); }

    if (tid < 32) {
        float v = sdata[tid] + sdata[tid + 32];
        v += __shfl_down_sync(0xffffffff, v, 16);
        v += __shfl_down_sync(0xffffffff, v,  8);
        v += __shfl_down_sync(0xffffffff, v,  4);
        v += __shfl_down_sync(0xffffffff, v,  2);
        v += __shfl_down_sync(0xffffffff, v,  1);
        if (tid == 0) *result = v;
    }
}

CGSolverCuda2D::CGSolverCuda2D(const unsigned int width, const unsigned int height)
    : ISolverCuda2D(width, height) {
    cudaMalloc(&_d_r,       _size * sizeof(float));
    cudaMalloc(&_d_d,       _size * sizeof(float));
    cudaMalloc(&_d_t,       _size * sizeof(float));
    cudaMalloc(&_d_partial, CG_NBLOCKS * sizeof(float));
    cudaMalloc(&_d_dot1,    sizeof(float));
    cudaMalloc(&_d_dot2,    sizeof(float));
}

CGSolverCuda2D::~CGSolverCuda2D() {
    cudaFree(_d_r);
    cudaFree(_d_d);
    cudaFree(_d_t);
    cudaFree(_d_partial);
    cudaFree(_d_dot1);
    cudaFree(_d_dot2);
}

float CGSolverCuda2D::solve(IGrid2D *_grid) {
    auto* grid = dynamic_cast<CudaGrid2D*>(_grid);
    if (!grid) return 0.f;

    const auto* d = grid->cudaDataCPU();
    const int   steps        = PROPERTY_I(MAX_STEPS);
    const float errThreshold = PROPERTY_F(SOLVER_THRESHOLD);
    const auto solveErrors         = PROPERTY(SOLVE_ERRORS, std::vector<float>*);
    solveErrors->clear();

    constexpr dim3 block2D(16, 16);
    const dim3 grid2D((_width  + 15) / 16, (_height + 15) / 16);
    const unsigned int blocks1D = (_size + CG_BLOCK - 1) / CG_BLOCK;

    cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(d->pressure, d->divergence, _size);  // p = div
    cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(_d_r,        d->divergence, _size);  // r = div

    cuda_cg_mat_mul_2D<<<grid2D,   block2D  >>>(grid->cudaData(), _d_t, d->pressure);  // t = L * p
    cuda_cg_sub    <<<blocks1D, CG_BLOCK >>>(_d_r, _d_t, _size);        // r = div - L*p

    cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(_d_d, _d_r, _size);            // d = r

    dot(_d_r, _d_r, _d_dot1);
    float sqLen = read_scalar(_d_dot1);
    if (sqLen < errThreshold) return sqLen;

    int iter;
    for (iter = 0; iter < steps; iter++) {
        cuda_cg_mat_mul_2D<<<grid2D, block2D>>>(grid->cudaData(), _d_t, _d_d);    // t = L * d

        dot(_d_d, _d_t, _d_dot2);
        const float dDotT = read_scalar(_d_dot2);
        if (fabsf(dDotT) < 1e-10f)
            break;
        const float alpha = sqLen / dDotT;                       // α = (r·r) / (d·t)

        cuda_cg_axpy<<<blocks1D, CG_BLOCK>>>(d->pressure, _d_d,  alpha, _size); // p += α*d

        if (iter & 0x3F) {
            cuda_cg_axpy<<<blocks1D, CG_BLOCK>>>(_d_r, _d_t, -alpha, _size);    // r -= α*t
        } else {
            // Periodic exact recompute to prevent floating-point drift
            cuda_cg_mat_mul_2D<<<grid2D,   block2D  >>>(grid->cudaData(), _d_t, d->pressure);
            cuda_cg_copy   <<<blocks1D, CG_BLOCK >>>(_d_r, d->divergence, _size);
            cuda_cg_sub    <<<blocks1D, CG_BLOCK >>>(_d_r, _d_t, _size);
        }

        const float oldSqLen = sqLen;
        dot(_d_r, _d_r, _d_dot1);
        sqLen = read_scalar(_d_dot1);
        solveErrors->push_back(sqLen);
        if (sqLen < errThreshold) {
            break;
        }

        const float beta = sqLen / oldSqLen;
        cuda_cg_xpay<<<blocks1D, CG_BLOCK>>>(_d_d, _d_r, beta, _size);  // d = β*d + r
    }

    SET_PROPERTY_I(SOLVE_STEPS, iter + 1);
    return sqLen;
}

void CGSolverCuda2D::dot(const float *a, const float *b, float *d_result) const {
    cuda_cg_dot_pass1<<<CG_NBLOCKS, CG_BLOCK>>>(a, b, _d_partial, _size);
    cuda_cg_dot_pass2<<<1,          CG_NBLOCKS>>>(_d_partial, d_result);
}

float CGSolverCuda2D::read_scalar(const float *d_ptr) {
    float h;
    cudaMemcpy(&h, d_ptr, sizeof(float), cudaMemcpyDeviceToHost);
    return h;
}


// #################################### Jacobi Method 3D ################################################
__global__ void cuda_jacobi_pass_3D(const CudaGrid3D::CudaData *grid, float* result, const float* p);

JacobiSolverCuda3D::JacobiSolverCuda3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : ISolverCuda3D(width, height, depth) {
    cudaMalloc(&_gpu_tmp, _size * sizeof(float));
}

JacobiSolverCuda3D::~JacobiSolverCuda3D() {
    cudaFree(_gpu_tmp);
}

float JacobiSolverCuda3D::solve(IGrid3D *_grid) {
    auto* grid = dynamic_cast<CudaGrid3D*>(_grid);
    if (!grid) return 0;

    const int iter = (PROPERTY_I(MAX_STEPS) / 2);

    constexpr dim3 blockSize(8, 8, 8);
    const dim3 gridSize(
        (_grid->width()  - 1 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 1 + blockSize.y - 1) / blockSize.y,
        (_grid->depth()  - 1 + blockSize.z - 1) / blockSize.z);

    for (int i = 0; i < iter; i++) {
        cuda_jacobi_pass_3D<<<gridSize, blockSize>>>(grid->cudaData(), _gpu_tmp, grid->cudaDataCPU()->pressure);
        cuda_jacobi_pass_3D<<<gridSize, blockSize>>>(grid->cudaData(), grid->cudaDataCPU()->pressure, _gpu_tmp);
    }

    return 0;
}

__global__ void cuda_jacobi_pass_3D(const CudaGrid3D::CudaData *grid, float* result, const float* p) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
    const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
    if (x >= grid->width || y >= grid->height || z >= grid->depth)
        return;

    const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);

    if (grid->rho[i] <= 0.5 || grid->boundary[i] > .5) {
        result[i] = p[i];
        return;
    }

    float sum = 0.0;
    int nCount = 0;

    cuda_sample_neighbor_3D(x - 1, y, z, sum, nCount, p, grid);
    cuda_sample_neighbor_3D(x + 1, y, z, sum, nCount, p, grid);
    cuda_sample_neighbor_3D(x, y - 1, z, sum, nCount, p, grid);
    cuda_sample_neighbor_3D(x, y + 1, z, sum, nCount, p, grid);
    cuda_sample_neighbor_3D(x, y, z - 1, sum, nCount, p, grid);
    cuda_sample_neighbor_3D(x, y, z + 1, sum, nCount, p, grid);

    if(nCount == 0)
        result[i] = 0.0;
    else
        result[i] = (sum - grid->divergence[i]) / (float)nCount;
}

// #################################### Conjugate Gradient 3D ################################################

__global__ static void cuda_cg_mat_mul_3D(
    const CudaGrid3D::CudaData* __restrict__ grid,
    float* __restrict__ result, const float* __restrict__ x)
{
    const unsigned int gx = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gy = blockIdx.y * blockDim.y + threadIdx.y;
    const unsigned int gz = blockIdx.z * blockDim.z + threadIdx.z;
    const unsigned int w = grid->width;
    const unsigned int h = grid->height;
    const unsigned int d = grid->depth;
    if (gx >= w || gy >= h || gz >= d) return;

    const unsigned int i = cuda_idx_3D(gx, gy, gz, w, h);
    if (grid->rho[i] <= 0.5f || grid->boundary[i] > 0.5f) {
        result[i] = x[i];
        return;
    }

    float sum  = 0.f;
    int nCount = 0;
    cuda_sample_neighbor_3D(gx - 1, gy, gz, sum, nCount, x, grid);
    cuda_sample_neighbor_3D(gx + 1, gy, gz, sum, nCount, x, grid);
    cuda_sample_neighbor_3D(gx, gy - 1, gz, sum, nCount, x, grid);
    cuda_sample_neighbor_3D(gx, gy + 1, gz, sum, nCount, x, grid);
    cuda_sample_neighbor_3D(gx, gy, gz - 1, sum, nCount, x, grid);
    cuda_sample_neighbor_3D(gx, gy, gz + 1, sum, nCount, x, grid);

    result[i] = sum - (float)nCount * x[i];
}

CGSolverCuda3D::CGSolverCuda3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : ISolverCuda3D(width, height, depth) {
    cudaMalloc(&_d_r,       _size * sizeof(float));
    cudaMalloc(&_d_d,       _size * sizeof(float));
    cudaMalloc(&_d_t,       _size * sizeof(float));
    cudaMalloc(&_d_partial, CG_NBLOCKS * sizeof(float));
    cudaMalloc(&_d_dot1,    sizeof(float));
    cudaMalloc(&_d_dot2,    sizeof(float));
}

CGSolverCuda3D::~CGSolverCuda3D() {
    cudaFree(_d_r);
    cudaFree(_d_d);
    cudaFree(_d_t);
    cudaFree(_d_partial);
    cudaFree(_d_dot1);
    cudaFree(_d_dot2);
}

float CGSolverCuda3D::solve(IGrid3D *_grid) {
    auto* grid = dynamic_cast<CudaGrid3D*>(_grid);
    if (!grid) return 0.f;

    const auto* d = grid->cudaDataCPU();
    const int   steps        = PROPERTY_I(MAX_STEPS);
    const float errThreshold = PROPERTY_F(SOLVER_THRESHOLD);
    const auto solveErrors         = PROPERTY(SOLVE_ERRORS, std::vector<float>*);
    solveErrors->clear();

    constexpr dim3 block3D(8, 8, 8);
    const dim3 grid3D(
        (_width  + block3D.x - 1) / block3D.x,
        (_height + block3D.y - 1) / block3D.y,
        (_depth  + block3D.z - 1) / block3D.z);
    const unsigned int blocks1D = (_size + CG_BLOCK - 1) / CG_BLOCK;

    //cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(d->pressure, d->divergence, _size);  // p = div
    cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(_d_r,        d->divergence, _size);  // r = div

    cuda_cg_mat_mul_3D<<<grid3D, block3D>>>(grid->cudaData(), _d_t, d->pressure);  // t = L * p
    cuda_cg_sub<<<blocks1D, CG_BLOCK>>>(_d_r, _d_t, _size);        // r = div - L*p

    cuda_cg_copy<<<blocks1D, CG_BLOCK>>>(_d_d, _d_r, _size);            // d = r

    dot(_d_r, _d_r, _d_dot1);
    float sqLen = read_scalar(_d_dot1);
    if (sqLen < errThreshold) {
        SET_PROPERTY_I(SOLVE_STEPS, 0);
        return sqLen;
    }

    int iter;
    for (iter = 0; iter < steps; iter++) {
        cuda_cg_mat_mul_3D<<<grid3D, block3D>>>(grid->cudaData(), _d_t, _d_d);    // t = L * d

        dot(_d_d, _d_t, _d_dot2);
        const float dDotT = read_scalar(_d_dot2);
        if (fabsf(dDotT) < 1e-10f)
            break;
        const float alpha = sqLen / dDotT;                       // α = (r·r) / (d·t)

        cuda_cg_axpy<<<blocks1D, CG_BLOCK>>>(d->pressure, _d_d,  alpha, _size); // p += α*d

        if (iter & 0x3F) {
            cuda_cg_axpy<<<blocks1D, CG_BLOCK>>>(_d_r, _d_t, -alpha, _size);    // r -= α*t
        } else {
            // Periodic exact recompute to prevent floating-point drift
            cuda_cg_mat_mul_3D<<<grid3D, block3D>>>(grid->cudaData(), _d_t, d->pressure);
            cuda_cg_copy   <<<blocks1D, CG_BLOCK>>>(_d_r, d->divergence, _size);
            cuda_cg_sub    <<<blocks1D, CG_BLOCK>>>(_d_r, _d_t, _size);
        }

        const float oldSqLen = sqLen;
        dot(_d_r, _d_r, _d_dot1);
        sqLen = read_scalar(_d_dot1);
        solveErrors->push_back(sqLen);
        if (sqLen < errThreshold) {
            break;
        }

        const float beta = sqLen / oldSqLen;
        cuda_cg_xpay<<<blocks1D, CG_BLOCK>>>(_d_d, _d_r, beta, _size);  // d = β*d + r
    }

    SET_PROPERTY_I(SOLVE_STEPS, iter + 1);
    return sqLen;
}

void CGSolverCuda3D::dot(const float *a, const float *b, float *d_result) const {
    cuda_cg_dot_pass1<<<CG_NBLOCKS, CG_BLOCK>>>(a, b, _d_partial, _size);
    cuda_cg_dot_pass2<<<1,          CG_NBLOCKS>>>(_d_partial, d_result);
}

float CGSolverCuda3D::read_scalar(const float *d_ptr) {
    float h;
    cudaMemcpy(&h, d_ptr, sizeof(float), cudaMemcpyDeviceToHost);
    return h;
}
