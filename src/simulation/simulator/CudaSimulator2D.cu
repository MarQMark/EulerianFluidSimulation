#include "simulation/simulator/CudaSimulator2D.h"

#include <algorithm>

#include "util/CudaUtil.h"
#include "simulation/solver/CudaSolver.h"
#include "util/Profiler.h"

__device__ bool cuda_in_wall(glm::vec2 p, const CudaGrid2D::CudaData *grid);
__device__ glm::vec2 cuda_domain_clamp(glm::vec2 p, const CudaGrid2D::CudaData *grid);
__device__ glm::vec2 cuda_staggered_offset_2D(int comp);
__device__ float cuda_bilinear_interpolation(const glm::vec2 &pos, int low_x, int low_y, int comp, glm::vec2* vel, unsigned int w);
__device__ glm::vec2 cuda_sample_vel(glm::vec2 pos, glm::vec2* vel, unsigned int w);
__device__ glm::vec2 cuda_trace_particle(glm::vec2 start, const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__device__ float cuda_sample_scal_field(glm::vec2 pos, const float* field, unsigned int w, unsigned int h);
__device__ glm::vec2 cuda_trace_along_field(const CudaGrid2D::CudaData* grid, glm::vec2 pos, float D, int substeps);
__device__ void cuda_scatter_rho(const CudaGrid2D::CudaData* grid, glm::vec2 pos, float value);

__global__ void cuda_vel_decay(const CudaGrid2D::CudaData* grid, float decay);
__global__ void cuda_vel_extrap_init(const CudaGrid2D::CudaData* grid, float* valid);
__global__ void cuda_vel_extrapolate(const CudaGrid2D::CudaData* grid, const glm::vec2* srcVel, const float* srcValid, glm::vec2* dstVel, float* dstValid);
__global__ void cuda_rho_sharpen_calc(const CudaGrid2D::CudaData* grid, float dt);
__global__ void cuda_rho_sharpen_apply(const CudaGrid2D::CudaData* grid);
__global__ void cuda_rho_sharpen_scatter(const CudaGrid2D::CudaData* grid, float D, int substeps);
__global__ void cuda_rho_adv_1(const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_2_3(const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_4(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_5(const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_6_10_bwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_6_10_fwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_7_bwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_7_fwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_8_bwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_8_fwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_9_bwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_9_fwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_11_bwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_11_fwd(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_11_cpy(const CudaGrid2D::CudaData *grid);
__global__ void cuda_rho_adv_12_x(const CudaGrid2D::CudaData *grid, int offset);
__global__ void cuda_rho_adv_12_y(const CudaGrid2D::CudaData *grid, int offset);
__global__ void cuda_add_force(const CudaGrid2D::CudaData *grid, float dt, float gravity);
__global__ void cuda_vel_adv_pass_1(const CudaGrid2D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_vel_adv_pass_2(const CudaGrid2D::CudaData *grid);
__global__ void cuda_proj_calc_div(const CudaGrid2D::CudaData *grid, float lambdaP, float lambdaM, float eta);
__global__ void cuda_proj_sub_pg(const CudaGrid2D::CudaData *grid);
__global__ void cuda_proj_enf_bound(const CudaGrid2D::CudaData *grid);


CudaSimulator2D::CudaSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
    _grid = new CudaGrid2D(width, height);

    const bool validSolver = availableSolvers(std::vector({
    Properties::JACOBI,
    Properties::SELF_CG
    }));

    if (validSolver) {
        switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
            case Properties::JACOBI:
                _solver = new JacobiSolverCuda2D(width, height);
                break;
            case Properties::SELF_CG:
                _solver = new CGSolverCuda2D(width, height);
        		break;
            default:
                break;
        }
    }

    _data = new SimulationDisplay2D::RenderData2D();
    _data->grid = _grid;
}

CudaSimulator2D::~CudaSimulator2D() {
    delete _grid;
    delete _solver;
}

void* CudaSimulator2D::getRenderData() {
    return _data;
}

void CudaSimulator2D::update() {
	_grid->upload();

    Profiler* profiler = Profiler::Get();
	profiler->start("0 Vel Decay");
	if (!PROPERTY_B(DISABLE_VEL_DECAY))	vel_decay();
	profiler->stop();

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

	_grid->download();
}

void CudaSimulator2D::vel_decay() const {
	constexpr dim3 blockSize(16, 16);
	const dim3 gridSize(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

	cuda_vel_decay<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(VEL_DECAY));
}

void CudaSimulator2D::vel_extrapolate() {
	constexpr dim3 blockSize(16, 16);
	const dim3 gridSize(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

	const auto* d = _grid->cudaDataCPU();

	// Reuse new_gamma and beta as ping-pong —
	// both are free between rho_advect and the next frame.
	float* validA = d->new_gamma;
	float* validB = d->beta;
	cuda_vel_extrap_init<<<gridSize, blockSize>>>(_grid->cudaData(), validA);

	for (int i = 0; i < 3; i++) {
		cuda_vel_extrapolate<<<gridSize, blockSize>>>(_grid->cudaData(), d->vel,     validA, d->tmp_vel, validB);
		cuda_vel_extrapolate<<<gridSize, blockSize>>>(_grid->cudaData(), d->tmp_vel, validB, d->vel,     validA);
	}
}


__host__
void CudaSimulator2D::rho_advect() {
	constexpr dim3 blockSize(16, 16);
	const dim3 gridSizeCore(
		(_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y);
	const dim3 gridSizeFull(
		(_grid->width() - 1 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 1 + blockSize.y - 1) / blockSize.y);

	_grid->clearA();

	// (1) Advect gamma using backward semi-lagrangian
	cuda_rho_adv_1<<<gridSizeCore, blockSize>>>(_grid->cudaData(), PROPERTY_F(DT), 4);

	// (2) Compute A + (3) Scale A by gamma
	cuda_rho_adv_2_3<<<gridSizeCore, blockSize>>>(_grid->cudaData(), PROPERTY_F(DT), 4);

	// (4) Compute beta
	cuda_rho_adv_4<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (5) forward trace the velocity field to add the weights to A for all cells j where beta j < 1
	cuda_rho_adv_5<<<gridSizeCore, blockSize>>>(_grid->cudaData(), PROPERTY_F(DT), 4);

	// (6) Compute gamma
	cuda_rho_adv_6_10_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_6_10_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (7) Scale A by gamma
	cuda_rho_adv_7_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_7_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (8) Recompute beta from A; !! Order is important - must call fwd before bwd
	cuda_rho_adv_8_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_8_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (9) Clamp beta to 1
	cuda_rho_adv_9_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_9_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (6) Compute gamma
	cuda_rho_adv_6_10_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_6_10_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	// (11) Evaluate p_n+1 = A * p
	cuda_rho_adv_11_bwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_11_fwd<<<gridSizeFull, blockSize>>>(_grid->cudaData());
	cuda_rho_adv_11_cpy<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	//################## Step 12 ##################
	// (12) Diffusion of rho and gamma
	{
		const unsigned int x_working_width = _grid->width() - 2;
		dim3 blocksX(
			( (x_working_width / 2) + blockSize.x - 1) / blockSize.x,
			( (_grid->height() - 2) + blockSize.y - 1) / blockSize.y
		);
		const unsigned int y_working_height = _grid->height() - 2;
		dim3 blocksY(
			( (_grid->width() - 2)   + blockSize.x - 1) / blockSize.x,
			( (y_working_height / 2) + blockSize.y - 1) / blockSize.y
		);

		for (int it = 0; it < 7; it++) {
			cuda_rho_adv_12_x<<<blocksX, blockSize>>>(_grid->cudaData(), 0);
			cuda_rho_adv_12_x<<<blocksX, blockSize>>>(_grid->cudaData(), 1);
			//cudaDeviceSynchronize();

			cuda_rho_adv_12_y<<<blocksY, blockSize>>>(_grid->cudaData(), 0);
			cuda_rho_adv_12_y<<<blocksY, blockSize>>>(_grid->cudaData(), 1);
			//cudaDeviceSynchronize();
		}
	}
}


void CudaSimulator2D::rho_sharpen() {
	constexpr dim3 blockSize(16, 16);
	const dim3 gridSize(
		(_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

	cuda_rho_sharpen_calc<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(DT) * 3.f);
	cuda_rho_sharpen_apply<<<gridSize, blockSize>>>(_grid->cudaData());
	cuda_rho_sharpen_scatter<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(RHO_D), 10);
}

void CudaSimulator2D::add_force() {
    constexpr dim3 blockSize(16, 16);
    const dim3 gridSize(
        (_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 2 + blockSize.y - 1) / blockSize.y);

    cuda_add_force<<<gridSize, blockSize>>>(
        _grid->cudaData(),
        PROPERTY_F(DT),
        PROPERTY_B(GRAVITY_ENABLED) ? PROPERTY_F(GRAVITY_FORCE) : 0.f);
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
	Profiler* profiler = Profiler::Get();
	constexpr dim3 blockSize(16, 16);
	const dim3 gridSizeCore(
		(_grid->width() - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y);
	const dim3 gridSizeFull(
		(_grid->width() - 1 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 1 + blockSize.y - 1) / blockSize.y);

	cuda_proj_enf_bound<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	profiler->start("Calc Divergence");
	_grid->clearProj();
	cuda_proj_calc_div<<<gridSizeCore, blockSize>>>(
	    _grid->cudaData(),
	    PROPERTY_F(LAMBDA_PLUS),
	    PROPERTY_F(LAMBDA_MINUS),
	    PROPERTY_F(ETA));
	profiler->stop();

	profiler->start("Solve");
    const float err = _solver->solve(_grid);
	profiler->stop();
    if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
        SET_PROPERTY_B(PAUSED, true);
        return;
    }

	profiler->start("Sub PG");
	cuda_proj_sub_pg<<<gridSizeCore, blockSize>>>(_grid->cudaData());
	profiler->stop();

	cuda_proj_enf_bound<<<gridSizeFull, blockSize>>>(_grid->cudaData());
}

__device__ bool cuda_in_wall(const glm::vec2 p, const CudaGrid2D::CudaData *grid) {
    if (p.x < 1.0 || p.x > grid->width  - 2.0 ||
        p.y < 1.0 || p.y > grid->height - 2.0)
        return true;
    return grid->boundary[cuda_idx_2D(
        cuda_clamp((unsigned int)p.x, 0u, grid->width-1u),
        cuda_clamp((unsigned int)p.y, 0u, grid->height-1u),
        grid->width)] > 0.5;
}
__device__ glm::vec2 cuda_domain_clamp(const glm::vec2 p, const CudaGrid2D::CudaData *grid) {
    return glm::clamp(p, glm::vec2(1.f), glm::vec2(grid->width - 2.0, grid->height - 2.0));
}
__device__ glm::vec2 cuda_staggered_offset_2D(const int comp) {
    return (comp == 0)  ? glm::vec2(-1.0f, -0.5f)
                        : glm::vec2(-0.5f, -1.0f);
}
__device__ float cuda_bilinear_interpolation(const glm::vec2 &pos, const int low_x, const int low_y, const int comp, glm::vec2* vel, const unsigned int w) {
    const glm::vec2 off = cuda_staggered_offset_2D(comp);

    const float x0 = low_x + off.x;
    const float y0 = low_y + off.y;

    const float tx = pos.x - x0;
    const float ty = pos.y - y0;

    const float v00 = vel[cuda_idx_2D(low_x,     low_y    , w)][comp];
    const float v01 = vel[cuda_idx_2D(low_x,     low_y + 1, w)][comp];
    const float v10 = vel[cuda_idx_2D(low_x + 1, low_y    , w)][comp];
    const float v11 = vel[cuda_idx_2D(low_x + 1, low_y + 1, w)][comp];

    const float vLow  = v00 * (1.0f - ty) + v01 * ty;
    const float vHigh = v10 * (1.0f - ty) + v11 * ty;

    return vLow * (1.0f - tx) + vHigh * tx;
}
__device__ glm::vec2 cuda_sample_vel(const glm::vec2 pos, glm::vec2* vel, const unsigned int w) {
    auto lowIndex = [](const float coord, const float offset) {
        return (int)(std::floor(coord - offset) + 1.0e-10f);
    };

    const glm::vec2 offU = cuda_staggered_offset_2D(0);
    const int low_x_u = lowIndex(pos.x, offU.x);
    const int low_y_u = lowIndex(pos.y, offU.y);
    const float vx = cuda_bilinear_interpolation(pos, low_x_u, low_y_u, 0, vel, w);

    const glm::vec2 offV = cuda_staggered_offset_2D(1);
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
__device__ float cuda_sample_scal_field(glm::vec2 pos, const float *field, const unsigned int w, const unsigned int h) {
	//pos += glm::vec2(1.f);
	const int x = (int)pos.x;
	const int y = (int)pos.y;

	unsigned int ix1 = x;
	unsigned int ix2 = x + 1;
	if(pos.x - (float)x < 0.5f) {
		ix1 = x - 1;
		ix2 = x;
	}
	unsigned int iy1 = y;
	unsigned int iy2 = y + 1;
	if(pos.y - (float)y < 0.5f) {
		iy1 = y - 1;
		iy2 = y;
	}

	ix1 = cuda_clamp(ix1, 0, w - 1);
	ix2 = cuda_clamp(ix2, 0, w - 1);
	iy1 = cuda_clamp(iy1, 0, h - 1);
	iy2 = cuda_clamp(iy2, 0, h - 1);

	const float v[4] = {
		field[cuda_idx_2D(ix1, iy1, w)],
		field[cuda_idx_2D(ix1, iy2, w)],
		field[cuda_idx_2D(ix2, iy1, w)],
		field[cuda_idx_2D(ix2, iy2, w)],
	};

	const float x1 = (float)ix1 + 0.5f;
	const float x2 = (float)ix2 + 0.5f;
	const float y1 = (float)iy1 + 0.5f;
	const float y2 = (float)iy2 + 0.5f;

	const float dx2x1 = x2 - x1;
	float fxy1 = ((x2 - pos.x)/dx2x1) * v[0] + ((pos.x - x1)/dx2x1) * v[2];
	float fxy2 = ((x2 - pos.x)/dx2x1) * v[1] + ((pos.x - x1)/dx2x1) * v[3];
	if(dx2x1 < 1e-9) {
		fxy1 = v[1];
		fxy2 = v[3];
	}

	const float dy2y1 = y2 - y1;
	float fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
	if(dy2y1 < 1e-9)
		fxy = fxy1;

	return fxy;
}

__device__ glm::vec2 cuda_trace_along_field(const CudaGrid2D::CudaData* grid, glm::vec2 pos, const float D, const int substeps) {
	const float stepSize = D / (float)substeps;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;

	for (int s = 0; s < substeps; s++) {
		if (cuda_sample_scal_field(pos, grid->rho, w, h) >= 0.5f)
			break;

		const int ix = max(1, min((int)pos.x, (int)w - 2));
		const int iy = max(1, min((int)pos.y, (int)h - 2));

		glm::vec2 grad(
			grid->rho[cuda_idx_2D(ix+1, iy,   w)] - grid->rho[cuda_idx_2D(ix-1, iy,   w)],
			grid->rho[cuda_idx_2D(ix,   iy+1, w)] - grid->rho[cuda_idx_2D(ix,   iy-1, w)]
		);
		const float len = sqrtf(grad.x*grad.x + grad.y*grad.y);
		if (len < 1e-6f) break;

		glm::vec2 next = pos + (grad / len) * stepSize;
		next.x = cuda_clamp(next.x, 1.f, (float)w - 2.f);
		next.y = cuda_clamp(next.y, 1.f, (float)h - 2.f);

		if (grid->boundary[cuda_idx_2D(next.x, next.y, w)] > 0.5f)
			break;
		pos = next;
	}
	return pos;
}

__device__ void cuda_scatter_rho(const CudaGrid2D::CudaData* grid, const glm::vec2 pos, const float value) {
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;

	const int x0 = max(0, min((int)pos.x,     (int)w - 1));
	const int y0 = max(0, min((int)pos.y,     (int)h - 1));
	const int x1 = max(0, min((int)pos.x + 1, (int)w - 1));
	const int y1 = max(0, min((int)pos.y + 1, (int)h - 1));

	const float fx = pos.x - floorf(pos.x);
	const float fy = pos.y - floorf(pos.y);

	float w00 = (1.f-fx) * (1.f-fy);
	float w10 =      fx  * (1.f-fy);
	float w01 = (1.f-fx) *      fy;
	float w11 =      fx  *      fy;

	if (grid->boundary[cuda_idx_2D(x0, y0, w)] > 0.5f) w00 = 0.f;
	if (grid->boundary[cuda_idx_2D(x1, y0, w)] > 0.5f) w10 = 0.f;
	if (grid->boundary[cuda_idx_2D(x0, y1, w)] > 0.5f) w01 = 0.f;
	if (grid->boundary[cuda_idx_2D(x1, y1, w)] > 0.5f) w11 = 0.f;

	const float total = w00 + w10 + w01 + w11;
	if (total < 1e-6f)
		return;
	const float inv = 1.f / total;

	atomicAdd(&grid->rho[cuda_idx_2D(x0, y0, w)], value * w00 * inv);
	atomicAdd(&grid->rho[cuda_idx_2D(x1, y0, w)], value * w10 * inv);
	atomicAdd(&grid->rho[cuda_idx_2D(x0, y1, w)], value * w01 * inv);
	atomicAdd(&grid->rho[cuda_idx_2D(x1, y1, w)], value * w11 * inv);
}

__global__ void cuda_vel_decay(const CudaGrid2D::CudaData *grid, const float decay) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;

	const unsigned int i = cuda_idx_2D(x, y, grid->width);
	if (grid->rho[i] >= 0.5f)
		return;
	grid->vel[i] *= decay;
}

__global__ void cuda_vel_extrap_init(const CudaGrid2D::CudaData* grid, float* valid){
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;
	const unsigned int i = cuda_idx_2D(x, y, grid->width);
	valid[i] = (grid->rho[i] >= 0.5f && grid->boundary[i] < 0.5f) ? 1.f : 0.f;
}

__global__ void cuda_vel_extrapolate(
	const CudaGrid2D::CudaData* grid,
	const glm::vec2* srcVel, const float* srcValid,
	glm::vec2* dstVel, float* dstValid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;
	const unsigned int i = cuda_idx_2D(x, y, grid->width);

	if (grid->boundary[i] > 0.5f) {
		dstVel[i] = srcVel[i];
		dstValid[i] = 0.f;
		return;
	}
	if (srcValid[i] > 0.5f) {
		dstVel[i] = srcVel[i];
		dstValid[i] = 1.f;
		return;
	}

	glm::vec2 sum(0.f);
	float n = 0;
	auto sample = [&](const unsigned int nx, const unsigned int ny) {
		const unsigned int ni = cuda_idx_2D(nx, ny, grid->width);
		if (srcValid[ni] > 0.5f) {
			sum += srcVel[ni];
			n++;
		}
	};
	sample(x-1, y);
	sample(x+1, y);
	sample(x, y-1);
	sample(x, y+1);

	dstVel[i]   = (n > 0) ? sum / n : srcVel[i];
	dstValid[i] = (n > 0) ? 1.f : 0.f;
}

__global__ void cuda_rho_sharpen_calc(const CudaGrid2D::CudaData *grid, const float dt) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;
	const unsigned int i = cuda_idx_2D(x, y, grid->width);

	grid->del_rho[i] = 0.f;
	if (grid->boundary[i] > 0.5f)
		return;

	const float rC = grid->rho[i];
	const float rL = grid->rho[cuda_idx_2D(x-1, y,   grid->width)];
	const float rR = grid->rho[cuda_idx_2D(x+1, y,   grid->width)];
	const float rD = grid->rho[cuda_idx_2D(x,   y-1, grid->width)];
	const float rU = grid->rho[cuda_idx_2D(x,   y+1, grid->width)];

	const float dpx = -(rC - rL) * dt;
	const float dpy = -(rC - rD) * dt;
	const float dmx = -(rR - rC) * dt;
	const float dmy = -(rU - rC) * dt;

	const float max_inc = sqrtf(
		fmaxf(
			powf(fmaxf(dpx,0.f),2.f),
			powf(fminf(dmx,0.f),2.f)) +
		fmaxf(
			powf(fmaxf(dpy,0.f),2.f),
			powf(fminf(dmy,0.f),2.f)));

	const float max_dec = sqrtf(
		fmaxf(
			powf(fminf(dpx,0.f),2.f),
			powf(fmaxf(dmx,0.f),2.f)) +
		fmaxf(
			powf(fminf(dpy,0.f),2.f),
			powf(fmaxf(dmy,0.f),2.f)));

	const float maxDiff = fmaxf(
							fmaxf(fabsf(rC-rL), fabsf(rC-rR)),
							fmaxf(fabsf(rC-rD), fabsf(rC-rU)));
	const float w = powf(rC - 0.5f, 3.f) * (1.f - fminf(1.f, maxDiff / 0.2f));

	float dr = (w >= 0.f) ? w * max_inc : w * max_dec;

	if (rC + dr < 0.f || rC < 1e-5f)
		dr = -rC;
	else if (rC > 0.5f)
		dr = 0.f;

	grid->del_rho[i] = dr;
}

__global__ void cuda_rho_sharpen_apply(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;
	const unsigned int i = cuda_idx_2D(x, y, grid->width);
	grid->rho[i] = fmaxf(0.f, grid->rho[i] + grid->del_rho[i]);
}

__global__ void cuda_rho_sharpen_scatter(const CudaGrid2D::CudaData *grid, const float D, const int substeps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;
	const unsigned int i = cuda_idx_2D(x, y, grid->width);

	const float dr = grid->del_rho[i];
	if (fabsf(dr) < 1e-6f) return;

	const glm::vec2 p = cuda_trace_along_field(grid, glm::vec2(x, y), D, substeps);
	cuda_scatter_rho(grid, p, -dr);
}

// (1) Advect gamma using backward semi-lagrangian
__global__ void cuda_rho_adv_1(const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int w = grid->width;

	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	if (grid->boundary[i] > 0.5)
		return;

	const glm::vec2 prevPos = cuda_trace_particle(glm::vec2(x, y), grid, dt, subSteps);
	grid->new_gamma[i] = cuda_sample_scal_field(prevPos, grid->gamma, grid->width, grid->height);
}

// (2) Compute A + (3) Scale A by gamma
__global__ void cuda_rho_adv_2_3(const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int w = grid->width;

	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	grid->counts_bwd[i] = 0;

	if (grid->boundary[i] > 0.5f)
		return;

	const glm::vec2 prevPos = cuda_trace_particle(glm::vec2(x, y), grid, dt, subSteps);
	const int p0x = floor(prevPos.x);
	const int p0y = floor(prevPos.y);

	const float dx0 = 1 - (prevPos.x - p0x);
	const float dy0 = 1 - (prevPos.y - p0y);
	const float dx1 = 1 - (p0x + 1 - prevPos.x);
	const float dy1 = 1 - (p0y + 1 - prevPos.y);

	const unsigned int dest[4] = {
		cuda_idx_2D(p0x + 0, p0y + 0, w),
		cuda_idx_2D(p0x + 1, p0y + 0, w),
		cuda_idx_2D(p0x + 0, p0y + 1, w),
		cuda_idx_2D(p0x + 1, p0y + 1, w)
	};

	float m[4] = {0, 0, 0, 0};
	// Boundary = 1 -> multiply by 0 if there is a boundary
	//m[0] = dx0 * dy0 * (1 - grid->boundary[dest[0]]);
	//m[1] = dx1 * dy0 * (1 - grid->boundary[dest[1]]);
	//m[2] = dx0 * dy1 * (1 - grid->boundary[dest[2]]);
	//m[3] = dx1 * dy1 * (1 - grid->boundary[dest[3]]);
	if (grid->boundary[dest[0]] != 1.f) m[0] = dx0 * dy0;
	if (grid->boundary[dest[1]] != 1.f) m[1] = dx1 * dy0;
	if (grid->boundary[dest[2]] != 1.f) m[2] = dx0 * dy1;
	if (grid->boundary[dest[3]] != 1.f) m[3] = dx1 * dy1;

	const float sum = m[0] + m[1] + m[2] + m[3];
	if (sum == 0)
		return;
	float g = grid->new_gamma[i];
	if(g < 1e-5)
		g = 1.0;

	unsigned int k = 0u;
	for (int c = 0; c < 4; c++) {
		if (m[c] > 0.0) {
			grid->A_bwd[i * 4 + k] = CudaGrid2D::AdvCell{
				.j = dest[c],
				.val = m[c]/(sum * g)
			};
			k++;
		}
	}

	grid->counts_bwd[i] = k;
}

// (4) Compute beta
__global__ void cuda_rho_adv_4(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	for (unsigned int k = 0; k < grid->counts_bwd[i]; k++) {
		const uint slot = (i * 4) + k;
		atomicAdd(&grid->beta[grid->A_bwd[slot].j], grid->A_bwd[slot].val);
	}
}

// (5) forward trace the velocity field to add the weights to A for all cells j where beta j < 1
__global__ void cuda_rho_adv_5(const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int w = grid->width;

	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	if (grid->beta[i] >= 1)
		return;

	grid->counts_fwd[i] = 0;
	if (grid->boundary[i] > 0.5f)
		return;

	const glm::vec2 nextPos = cuda_trace_particle(glm::vec2(x, y), grid, -dt, subSteps);
	const int p0x = floor(nextPos.x);
	const int p0y = floor(nextPos.y);

	const float dx0 = 1 - (nextPos.x - p0x);
	const float dy0 = 1 - (nextPos.y - p0y);
	const float dx1 = 1 - (p0x + 1 - nextPos.x);
	const float dy1 = 1 - (p0y + 1 - nextPos.y);

	const unsigned int dest[4] = {
		cuda_idx_2D(p0x + 0, p0y + 0, w),
		cuda_idx_2D(p0x + 1, p0y + 0, w),
		cuda_idx_2D(p0x + 0, p0y + 1, w),
		cuda_idx_2D(p0x + 1, p0y + 1, w)
	};

	float m[4] = {0, 0, 0, 0};
	// Boundary = 1 -> multiply by 0 if there is a boundary
	m[0] = dx0 * dy0 * (1 - grid->boundary[dest[0]]);
	m[1] = dx1 * dy0 * (1 - grid->boundary[dest[1]]);
	m[2] = dx0 * dy1 * (1 - grid->boundary[dest[2]]);
	m[3] = dx1 * dy1 * (1 - grid->boundary[dest[3]]);

	const float sum = m[0] + m[1] + m[2] + m[3];
	if (sum == 0)
		return;

	const float b = 1.f - grid->beta[i];
	unsigned int k = 0;
	for (int c = 0; c < 4; c++) {
		if (m[c] > 0.0) {
			grid->A_fwd[i * 4 + k] = CudaGrid2D::AdvCell{
				.i = dest[c],
				.val = b * (m[c] / sum)
			};
			k++;
		}
	}

	grid->counts_fwd[i] = k;
}

// (6) Compute gamma
__global__ void cuda_rho_adv_6_10_bwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	float g = 0;
	for (int k = 0; k < grid->counts_bwd[i]; k++)
		g += grid->A_bwd[(i * 4) + k].val;

	grid->gamma[i] = g;
}
__global__ void cuda_rho_adv_6_10_fwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int j = cuda_idx_2D(x, y, w);
	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		atomicAdd(&grid->gamma[grid->A_fwd[(j * 4) + k].i], grid->A_fwd[(j * 4) + k].val);
}

// (7) Scale A by gamma
__global__ void cuda_rho_adv_7_bwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	const float g = grid->gamma[i];
	if (g < 1e-5)
		return;

	for (uint k = 0; k < grid->counts_bwd[i]; k++)
		grid->A_bwd[(i * 4) + k].val /= g;
}
__global__ void cuda_rho_adv_7_fwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int j = cuda_idx_2D(x, y, w);

	for (uint k = 0u; k < grid->counts_fwd[j]; k++) {
		const float g = grid->gamma[grid->A_fwd[(j * 4) + k].j];
		if (g > 1e-5)
			grid->A_fwd[(j * 4) + k].val /= g;
	}
}

// (8) Recompute beta from A; !! Order is important - must call fwd before bwd
__global__ void cuda_rho_adv_8_bwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);

	for (uint k = 0u; k < grid->counts_bwd[i]; k++)
		atomicAdd(&grid->beta[grid->A_bwd[(i * 4) + k].j], grid->A_bwd[(i * 4) + k].val);
}
__global__ void cuda_rho_adv_8_fwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int j = cuda_idx_2D(x, y, w);

	float b = 0.0;
	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		b += grid->A_fwd[(j * 4) + k].val;

	grid->beta[j] = b;
}

// (9) Clamp beta to 1
__global__ void cuda_rho_adv_9_bwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	for (uint k = 0u; k < grid->counts_bwd[i]; k++) {
		const float b = grid->beta[grid->A_bwd[(i * 4) + k].j];
		if (b > 1e-5)
			grid->A_bwd[(i * 4) + k].val /= b;
	}
}
__global__ void cuda_rho_adv_9_fwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int j = cuda_idx_2D(x, y, w);
	const float b = grid->beta[j];
	if (b <= 1e-5)
		return;

	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		grid->A_fwd[(j * 4) + k].val /= b;
}

// (11) Evaluate p_n+1 = A * p
__global__ void cuda_rho_adv_11_bwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	float acc  = 0.0;
	for (uint k = 0u; k < grid->counts_bwd[i]; k++) {
		const CudaGrid2D::AdvCell c = grid->A_bwd[(i * 4) + k];
		acc += c.val * grid->rho[c.j];
	}

	grid->tmp_rho[i] = acc;
}
__global__ void cuda_rho_adv_11_fwd(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int j = cuda_idx_2D(x, y, w);
	const float rho_j = grid->rho[j];
	for (uint k = 0u; k < grid->counts_fwd[j]; k++) {
		const CudaGrid2D::AdvCell c = grid->A_fwd[(j * 4) + k];
		atomicAdd(&grid->tmp_rho[c.j], c.val * rho_j);
	}
}

__global__ void cuda_rho_adv_11_cpy(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int w = grid->width;

	if (x >= grid->width || y >= grid->height)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	grid->rho[i] = grid->tmp_rho[i];
}

// (12) Diffusion of rho and gamma
__global__ void cuda_rho_adv_12_x(const CudaGrid2D::CudaData *grid, const int offset) {
    // We only launch half the threads in the X dimension.
    // offset will be 0 for the Even pass, and 1 for the Odd pass.
    const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int iy = blockIdx.y * blockDim.y + threadIdx.y;

    const unsigned int x = 2 + (ix * 2) + offset;
    const unsigned int y = 1 + iy;

    if (x >= grid->width - 1 || y >= grid->height - 1)
    	return;

    const unsigned int idx_j = cuda_idx_2D(x - 1, y, grid->width);
    const unsigned int idx_i = cuda_idx_2D(x,     y, grid->width);

    if (grid->boundary[idx_i] == 1 || grid->boundary[idx_j] == 1)
    	return;

    const float g_j = grid->gamma[idx_j];
    const float g_i = grid->gamma[idx_i];

    if (g_j < 1e-5f || g_i < 1e-5f || g_j == g_i)
    	return;

    const float avg = (g_j + g_i) * 0.5f;
    const float r_j = grid->rho[idx_j];
    const float r_i = grid->rho[idx_i];
    if (g_j > g_i) {
        const float dif = r_j * (g_j - g_i) / (2.0f * g_j);
        grid->rho[idx_j] = r_j - dif;
        grid->rho[idx_i] = r_i + dif;
    } else {
        const float dif = r_i * (g_i - g_j) / (2.0f * g_i);
        grid->rho[idx_i] = r_i - dif;
        grid->rho[idx_j] = r_j + dif;
    }

    grid->gamma[idx_j] = avg;
    grid->gamma[idx_i] = avg;
}
__global__ void cuda_rho_adv_12_y(const CudaGrid2D::CudaData *grid, const int offset) {
    // We only launch half the threads in the Y dimension.
    const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int iy = blockIdx.y * blockDim.y + threadIdx.y;

    const unsigned int x = 1 + ix;
    const unsigned int y = 2 + (iy * 2) + offset;

    if (x >= grid->width - 1 || y >= grid->height - 1)
    	return;

    const unsigned int idx_j = cuda_idx_2D(x, y - 1, grid->width);
    const unsigned int idx_i = cuda_idx_2D(x, y,     grid->width);

    if (grid->boundary[idx_i] == 1.f || grid->boundary[idx_j] == 1.f)
    	return;

    const float g_j = grid->gamma[idx_j];
    const float g_i = grid->gamma[idx_i];

    if (g_j < 1e-5f || g_i < 1e-5f || g_j == g_i)
    	return;

    const float avg = (g_j + g_i) * 0.5f;
    const float r_j = grid->rho[idx_j];
    const float r_i = grid->rho[idx_i];
    if (g_j > g_i) {
        const float dif = r_j * (g_j - g_i) / (2.0f * g_j);
        grid->rho[idx_j] = r_j - dif;
        grid->rho[idx_i] = r_i + dif;
    } else {
        const float dif = r_i * (g_i - g_j) / (2.0f * g_i);
        grid->rho[idx_i] = r_i - dif;
        grid->rho[idx_j] = r_j + dif;
    }

    grid->gamma[idx_j] = avg;
    grid->gamma[idx_i] = avg;
}

__global__ void cuda_vel_adv_pass_1(const CudaGrid2D::CudaData *grid, const float dt, const int subSteps) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const glm::vec2 prevPos = cuda_trace_particle(glm::vec2(x, y), grid, dt, subSteps);
    grid->tmp_vel[cuda_idx_2D(x, y, w)] = cuda_sample_vel(prevPos - 0.5f, grid->vel, grid->width);
}


__global__ void cuda_vel_adv_pass_2(const CudaGrid2D::CudaData *grid) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx_2D(x, y, w);

    const glm::vec2 cur = grid->tmp_vel[i];
    auto out = glm::vec2(0);

    const float xBound = max(grid->boundary[i], grid->boundary[cuda_idx_2D(x - 1, y, w)]);
    out.x = xBound * cur.x + (1 - xBound) * 0.5 * (cur.x + grid->tmp_vel[cuda_idx_2D(x-1, y, w)].x);

    const float yBound = max(grid->boundary[i], grid->boundary[cuda_idx_2D(x, y - 1, w)]);
    out.y = yBound * cur.y + (1 - yBound) * 0.5 * (cur.y + grid->tmp_vel[cuda_idx_2D(x, y-1, w)].y);

    grid->vel[i] = out;
}

__global__ void cuda_add_force(const CudaGrid2D::CudaData *grid, const float dt, const float gravity) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx_2D(x, y, grid->width);
	//if (grid->boundary[i] > 0.5f || grid->rho[i] < 0.5f)
	if (grid->boundary[i] > 0.5f)
		return;

	grid->vel[i] += grid->forces[i] * dt;
	grid->vel[i].y -= gravity * dt;
}

__global__ void cuda_proj_calc_div(const CudaGrid2D::CudaData *grid, const float lambdaP, const float lambdaM, const float eta) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int w = grid->width;

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx_2D(x, y, w);

    const float r = grid->rho[i];
    if(r < 0.5f || grid->boundary[i] == 1)
    	return;

    const float dx = grid->vel[cuda_idx_2D(x + 1, y, w)].x - grid->vel[i].x;
    const float dy = grid->vel[cuda_idx_2D(x, y + 1, w)].y - grid->vel[i].y;
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

    if (x >= grid->width - 1 || y >= grid->height - 1)
        return;

    const unsigned int i = cuda_idx_2D(x, y, w);
    if(grid->boundary[i] == 1)
    	return;

    auto dPressure = glm::vec2(0);
    if (x > 1)
        dPressure.x = grid->pressure[i] - grid->pressure[cuda_idx_2D(x - 1, y, w)];
    if (y > 1)
        dPressure.y = grid->pressure[i] - grid->pressure[cuda_idx_2D(x, y - 1, w)];

    grid->vel[i] -= dPressure;
}

__global__ void cuda_proj_enf_bound(const CudaGrid2D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int w = grid->width;

	if (x >= grid->width - 1 || y >= grid->height - 1)
		return;

	const unsigned int i = cuda_idx_2D(x, y, w);
	const unsigned int ix = cuda_idx_2D(x - 1, y, w);
	if (grid->boundary[i] == 1.f && grid->vel[i].x > 0)
		grid->vel[i].x = 0.f;
	if (grid->boundary[ix] == 1.f && grid->vel[i].x < 0)
		grid->vel[i].x = 0.f;

	const unsigned int iy = cuda_idx_2D(x, y - 1, w);
	if (grid->boundary[i] == 1.f && grid->vel[i].y > 0)
		grid->vel[i].y = 0.f;
	if (grid->boundary[iy] == 1.f && grid->vel[i].y < 0)
		grid->vel[i].y = 0.f;
}
