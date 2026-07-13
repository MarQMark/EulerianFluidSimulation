#include "simulation/simulator/CudaSimulator2D.h"

#include <algorithm>

#include "simulation/solver/CudaSolver.h"
#include "util/Profiler.h"

__device__ unsigned int cuda_idx(unsigned int x, unsigned int y, unsigned int width);
__device__ bool cuda_in_wall(glm::vec2 p, const CudaGrid2D::CudaData *grid);
__device__ glm::vec2 cuda_domain_clamp(glm::vec2 p, const CudaGrid2D::CudaData *grid);
__device__ glm::vec2 cuda_staggered_offset(int comp);
__device__ float cuda_bilinear_interpolation(const glm::vec2 &pos, int low_x, int low_y, int comp, glm::vec2* vel, unsigned int w);
__device__ glm::vec2 cuda_sample_vel(glm::vec2 pos, glm::vec2* vel, unsigned int w);
__device__ glm::vec2 cuda_trace_particle(glm::vec2 start, const CudaGrid2D::CudaData *grid, float dt, int subSteps);

__global__ void cuda_add_force(const CudaGrid2D::CudaData *grid, float dt, float gravity);
__global__ void cuda_vel_adv_pass_1(const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_vel_adv_pass_2(const CudaGrid2D::CudaData *grid);
__global__ void cuda_proj_calc_div(const CudaGrid2D::CudaData *grid, float lambdaP, float lambdaM, float eta);
__global__ void cuda_proj_sub_pg(const CudaGrid2D::CudaData *grid);

CudaSimulator2D::CudaSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
    _grid = new CudaGrid2D(width, height);

    const bool validSolver = availableSolvers(std::vector({
    Properties::JACOBI,
    //Properties::SELF_CG
    }));

    if (validSolver) {
        switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
            case Properties::JACOBI:
                _solver = new JacobiSolverCuda2D(width, height);
                break;
            case Properties::SELF_CG:
                //_solver = new CGSolverCuda2D(width, height);
            default:
                break;
        }
    }

    _data = new SimulationDisplay2D::RenderData2D();
    _data->grid = _grid;
}

CudaSimulator2D::~CudaSimulator2D() {
    delete _grid;
    //delete _solver;
}

void* CudaSimulator2D::getRenderData() {
    return _data;
}

void CudaSimulator2D::update() {
    Profiler* profiler = Profiler::Get();
    profiler->start("1 Vel Extrapolate");
    if (!PROPERTY_B(DISABLE_VEL_EXTRAPOLATE))	vel_extrapolate();
    profiler->stop();
    profiler->start("2 Rho Sharpen");
    if (!PROPERTY_B(DISABLE_RHO_SHARPEN))		rho_sharpen();
    profiler->stop();
    profiler->start("3 Rho Advect");
    if (!PROPERTY_B(DISABLE_RHO_ADVECT))		rho_advect();
    profiler->stop();
    profiler->start("4 Vel Advect");
    if (!PROPERTY_B(DISABLE_VEL_ADVECT))		vel_advect();
    profiler->stop();
    profiler->start("5 Add Force");
    if (!PROPERTY_B(DISABLE_ADD_FORCE))			add_force();
    profiler->stop();
    profiler->start("6 Project");
    if (!PROPERTY_B(DISABLE_PROJECT))			project();
    profiler->stop();
}

void CudaSimulator2D::vel_extrapolate() {
}

void CudaSimulator2D::rho_advect() {
}

void CudaSimulator2D::rho_sharpen() {
}

void CudaSimulator2D::add_force() {
    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

    cuda_add_force<<<gridSize, blockSize>>>(
        _grid->cudaData(),
        PROPERTY_F(DT),
        PROPERTY_F(GRAVITY_FORCE));
}

void CudaSimulator2D::diffuse() {}

void CudaSimulator2D::vel_advect() {
    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

    cuda_vel_adv_pass_1<<<gridSize, blockSize>>>(
        _grid->cudaData(),
        PROPERTY_F(DT),
        4);

    cuda_vel_adv_pass_2<<<gridSize, blockSize>>>(_grid->cudaData());
}

void CudaSimulator2D::project() {
    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 1 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 1 + blockSize.y - 1) / blockSize.y);

    cuda_proj_calc_div<<<gridSize, blockSize>>>(
        _grid->cudaData(),
        PROPERTY_F(LAMBDA_PLUS),
        PROPERTY_F(LAMBDA_MINUS),
        PROPERTY_F(ETA));

    const float err = _solver->solve(_grid);
    if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
        SET_PROPERTY_B(PAUSED, true);
        return;
    }

    cuda_proj_sub_pg<<<gridSize, blockSize>>>(_grid->cudaData());
}

__device__ unsigned int cuda_idx(const unsigned int x, const unsigned int y, const unsigned int width) {
    return y * width + x;
}
__device__ float cuda_clamp(const float x, const float min, const float max) {
    return fmaxf(min, fminf(max, x));
}
__device__ bool cuda_in_wall(glm::vec2 p, const CudaGrid2D::CudaData *grid) {
    if (p.x < 1.0 || p.x > grid->width  - 2.0 ||
        p.y < 1.0 || p.y > grid->height - 2.0)
        return true;
    return grid->boundary[cuda_idx(
        cuda_clamp((unsigned int)p.x, 0u, grid->width-1u),
        cuda_clamp((unsigned int)p.y, 0u, grid->height-1u),
        grid->width)] > 0.5;
}
__device__ glm::vec2 cuda_domain_clamp(const glm::vec2 p, const CudaGrid2D::CudaData *grid) {
    return glm::clamp(p, glm::vec2(1.f), glm::vec2(grid->width - 2.0, grid->height - 2.0));
}
__device__ glm::vec2 cuda_staggered_offset(const int comp) {
    return (comp == 0)  ? glm::vec2(-1.0f, -0.5f)
                        : glm::vec2(-0.5f, -1.0f);
}
__device__ float cuda_bilinear_interpolation(const glm::vec2 &pos, const int low_x, const int low_y, const int comp, glm::vec2* vel, const unsigned int w) {
    const glm::vec2 off = cuda_staggered_offset(comp);

    const float x0 = low_x + off.x;
    const float y0 = low_y + off.y;

    const float tx = pos.x - x0;
    const float ty = pos.y - y0;

    const float v00 = vel[cuda_idx(low_x,     low_y    , w)][comp];
    const float v01 = vel[cuda_idx(low_x,     low_y + 1, w)][comp];
    const float v10 = vel[cuda_idx(low_x + 1, low_y    , w)][comp];
    const float v11 = vel[cuda_idx(low_x + 1, low_y + 1, w)][comp];

    const float vLow  = v00 * (1.0f - ty) + v01 * ty;
    const float vHigh = v10 * (1.0f - ty) + v11 * ty;

    return vLow * (1.0f - tx) + vHigh * tx;
}
__device__ glm::vec2 cuda_sample_vel(const glm::vec2 pos, glm::vec2* vel, const unsigned int w) {
    auto lowIndex = [](const float coord, const float offset) {
        return (int)(std::floor(coord - offset) + 1.0e-10f);
    };

    const glm::vec2 offU = cuda_staggered_offset(0);
    const int low_x_u = lowIndex(pos.x, offU.x);
    const int low_y_u = lowIndex(pos.y, offU.y);
    const float vx = cuda_bilinear_interpolation(pos, low_x_u, low_y_u, 0, vel, w);

    const glm::vec2 offV = cuda_staggered_offset(1);
    const int low_x_v = lowIndex(pos.x, offV.x);
    const int low_y_v = lowIndex(pos.y, offV.y);
    const float vy = cuda_bilinear_interpolation(pos, low_x_v, low_y_v, 1, vel, w);

    return glm::vec2(vx, vy);
}
__device__ glm::vec2 cuda_trace_particle(const glm::vec2 start, const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
    if (cuda_in_wall(start, grid)) return start;

    float subDt = -dt / (float)subSteps;   // backward
    glm::vec2 pos = start;

    for (int s = 0; s < subSteps; s++) {
        glm::vec2 k1   = cuda_sample_vel(pos - 0.5f, grid->vel, grid->width);
        glm::vec2 mid  = cuda_domain_clamp(pos + k1 * (subDt * 0.5f), grid);
        glm::vec2 k2   = cuda_sample_vel(mid - 0.5f, grid->vel, grid->width);
        const glm::vec2 next = pos + k2 * subDt;

        if (cuda_in_wall(next, grid)) {
            // Binary search: find last safe position before the wall
            glm::vec2 safe = pos;
            glm::vec2 unsafe = next;
            glm::vec2 h = safe;
            for (int b = 0; b < 8; b++) {
                h = (safe + unsafe) * 0.5f;
                if (cuda_in_wall(h, grid)) unsafe = h;
                else              safe   = h;
            }
            return safe;
        }
        pos = next;
    }
    return pos;
}

__global__ void cuda_vel_adv_pass_1(const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const glm::vec2 prevPos = cuda_trace_particle(glm::vec2(x, y), grid, dt, subSteps);
    grid->tmp_vel[cuda_idx(x, y, w)] = cuda_sample_vel(prevPos - 0.5f, grid->vel, grid->width);
}
__global__ void cuda_vel_adv_pass_2(const CudaGrid2D::CudaData *grid) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx(x, y, w);

    const glm::vec2 cur = grid->tmp_vel[i];
    auto out = glm::vec2(0);

    const float xBound = max(grid->boundary[i], grid->boundary[cuda_idx(x - 1, y, w)]);
    out.x = xBound * cur.x + (1 - xBound) * 0.5 * (cur.x + grid->tmp_vel[cuda_idx(x-1, y, w)].x);

    const float yBound = max(grid->boundary[i], grid->boundary[cuda_idx(x, y - 1, w)]);
    out.y = yBound * cur.y + (1 - yBound) * 0.5 * (cur.y + grid->tmp_vel[cuda_idx(x, y-1, w)].y);

    grid->vel[i] = out;
}

__global__ void cuda_add_force(const CudaGrid2D::CudaData *grid, const float dt, const float gravity) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx(x, y, grid->width);

    const float valid = (1.0f - grid->boundary[i]) * (grid->rho[i] >= 0.5f ? 1.0f : 0.0f);

    grid->vel[i].x += valid * grid->forces[i].x * dt;
    grid->vel[i].y += valid * (grid->forces[i].y - gravity) * dt;
}

__global__ void cuda_proj_calc_div(const CudaGrid2D::CudaData *grid, const float lambdaP, const float lambdaM, const float eta) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width || y >= grid->height)
        return;

    const unsigned int i = cuda_idx(x, y, w);

    const float r = grid->rho[i];
    grid->divergence[i] = 0;
    if(r < 0.5 || grid->boundary[i] == 1) return;

    const float dx = grid->vel[cuda_idx(x + 1, y, w)].x - grid->vel[cuda_idx(x, y, w)].x;
    const float dy = grid->vel[cuda_idx(x, y + 1, w)].y - grid->vel[cuda_idx(x, y, w)].y;
    float d = dx + dy;

    if (r > 1.f)
        d -= min(lambdaM * (r - 1.f), eta);
    if (r < 1.f)
        d += min(lambdaP * (1.f - r), eta);

    grid->divergence[i] = d;
}
__global__ void cuda_proj_sub_pg(const CudaGrid2D::CudaData *grid) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width || y >= grid->height)
        return;

    const unsigned int i = cuda_idx(x, y, w);
    if(grid->boundary[i] == 1) return;

    auto dPressure = glm::vec2(0);
    if (x != 1)
        dPressure.x = grid->pressure[i] - grid->pressure[cuda_idx(x - 1, y, w)];
    if (y != 1)
        dPressure.y = grid->pressure[i] - grid->pressure[cuda_idx(x, y - 1, w)];

    grid->vel[i] -= dPressure;
}
