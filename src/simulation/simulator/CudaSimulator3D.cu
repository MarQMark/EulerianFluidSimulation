#include "simulation/simulator/CudaSimulator3D.h"

#include "glm/common.hpp"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"
#include "simulation/solver/CGSolver.h"
#include "util/Profiler.h"
#include "util/CudaUtil.h"

__device__ bool cuda_in_wall(glm::vec3 p, const CudaGrid3D::CudaData *grid);
__device__ glm::vec3 cuda_domain_clamp(glm::vec3 p, const CudaGrid3D::CudaData *grid);
__device__ glm::vec3 cuda_staggered_offset_3D(int comp);
__device__ float cuda_trilinear_interpolation(const glm::vec3 &pos, int low_x, int low_y, int low_z, int comp, glm::vec3* vel, unsigned int w, unsigned int h);
__device__ glm::vec3 cuda_sample_vel(glm::vec3 pos, glm::vec3* vel, unsigned int w, unsigned int h);
__device__ glm::vec3 cuda_trace_particle(glm::vec3 start, const CudaGrid3D::CudaData *grid, float dt, int subSteps);
__device__ float cuda_sample_scal_field(glm::vec3 pos, const float* field, unsigned int w, unsigned int h, unsigned int d);
__device__ glm::vec3 cuda_trace_along_field(const CudaGrid3D::CudaData* grid, glm::vec3 pos, float D, int substeps);
__device__ void cuda_scatter_rho(const CudaGrid3D::CudaData* grid, glm::vec3 pos, float value);

__global__ void cuda_vel_decay(const CudaGrid3D::CudaData* grid, float decay);
__global__ void cuda_vel_extrap_init(const CudaGrid3D::CudaData* grid, glm::vec3* valid);
__global__ void cuda_vel_extrapolate(const CudaGrid3D::CudaData* grid, const glm::vec3* srcVel, const glm::vec3* srcValid, glm::vec3* dstVel, glm::vec3* dstValid);
__global__ void cuda_rho_sharpen_calc(const CudaGrid3D::CudaData* grid, float dt);
__global__ void cuda_rho_sharpen_apply(const CudaGrid3D::CudaData* grid);
__global__ void cuda_rho_sharpen_scatter(const CudaGrid3D::CudaData* grid, float D, int substeps);
__global__ void cuda_rho_adv_1(const CudaGrid3D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_2_3(const CudaGrid3D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_4(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_5(const CudaGrid3D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_rho_adv_6_10_bwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_6_10_fwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_7_bwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_7_fwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_8_bwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_8_fwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_9_bwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_9_fwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_11_bwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_11_fwd(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_11_cpy(const CudaGrid3D::CudaData *grid);
__global__ void cuda_rho_adv_12_x(const CudaGrid3D::CudaData *grid, int offset);
__global__ void cuda_rho_adv_12_y(const CudaGrid3D::CudaData *grid, int offset);
__global__ void cuda_rho_adv_12_z(const CudaGrid3D::CudaData *grid, int offset);
__global__ void cuda_add_force(const CudaGrid3D::CudaData *grid, float dt, float gravity, bool incAir);
__global__ void cuda_vel_adv_pass_1(const CudaGrid3D::CudaData *grid, float dt, int subSteps);
__global__ void cuda_vel_adv_pass_2(const CudaGrid3D::CudaData *grid);
__global__ void cuda_proj_calc_div(const CudaGrid3D::CudaData *grid, float lambdaP, float lambdaM, float eta);
__global__ void cuda_proj_sub_pg(const CudaGrid3D::CudaData *grid);
__global__ void cuda_proj_enf_bound(const CudaGrid3D::CudaData *grid);

CudaSimulator3D::CudaSimulator3D(const unsigned int width, const unsigned int height, const unsigned int depth)
    : ISimulator3D(width, height, depth) {
    _grid = new CudaGrid3D(width, height, depth);

    const bool validSolver = availableSolvers(std::vector({
        Properties::JACOBI,
        Properties::SELF_CG
    }));

    if (validSolver) {
        switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
            case Properties::JACOBI:
                _solver = new JacobiSolverCuda3D(width, height, depth);
                break;
            case Properties::SELF_CG:
                _solver = new CGSolverCuda3D(width, height, depth);
        		break;
            default:
                break;
        }
    }

    _data = new SimulationDisplay3D::RenderData3D();
    _data->grid = _grid;
}

CudaSimulator3D::~CudaSimulator3D() {
    delete _grid;
    delete _solver;
}

void * CudaSimulator3D::getRenderData() {
    return _data;
}

void CudaSimulator3D::update() {
    Profiler* profiler = Profiler::Get();

	PROFILE("Upload",
		if (!PROPERTY_B(DISABLE_UDLOAD)) _grid->upload();
	)

	ISimulator::update();

	PROFILE("Download",
		if (!PROPERTY_B(DISABLE_UDLOAD)) _grid->download();
	)
}

void CudaSimulator3D::vel_decay() {
    constexpr dim3 blockSize(4, 4, 4);
    const dim3 gridSize(
        (_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
        (_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);

    cuda_vel_decay<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(VEL_DECAY));
}

void CudaSimulator3D::vel_extrapolate() {
    constexpr dim3 blockSize(4, 4, 4);
    const dim3 gridSize(
        (_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
        (_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
        (_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);

    const auto* d = _grid->cudaDataCPU();
    cuda_vel_extrap_init<<<gridSize, blockSize>>>(_grid->cudaData(), _grid->validA);

    for (int i = 0; i < PROPERTY_I(VEL_EXTRAPOLATE_ITER); i++) {
        cuda_vel_extrapolate<<<gridSize, blockSize>>>(_grid->cudaData(), d->vel,      _grid->validA, d->tmp_vel, _grid->validB);
        cuda_vel_extrapolate<<<gridSize, blockSize>>>(_grid->cudaData(), d->tmp_vel,  _grid->validB, d->vel,     _grid->validA);
    }
}

void CudaSimulator3D::rho_advect() {
	constexpr dim3 blockSize(4, 4, 4);
	const dim3 gridSizeCore(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);
	const dim3 gridSizeFull(
		(_grid->width()  - 1 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 1 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 1 + blockSize.z - 1) / blockSize.z);

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
			( (_grid->height() - 2) + blockSize.y - 1) / blockSize.y,
			( (_grid->depth()  - 2) + blockSize.z - 1) / blockSize.z
		);
		const unsigned int y_working_height = _grid->height() - 2;
		dim3 blocksY(
			( (_grid->width() - 2)   + blockSize.x - 1) / blockSize.x,
			( (y_working_height / 2) + blockSize.y - 1) / blockSize.y,
			( (_grid->depth()  - 2)  + blockSize.z - 1) / blockSize.z
		);
		const unsigned int y_working_depth = _grid->depth() - 2;
		dim3 blocksZ(
			( (_grid->width() - 2)  + blockSize.x - 1) / blockSize.x,
			( (_grid->height() - 2) + blockSize.y - 1) / blockSize.y,
			( (y_working_depth / 2) + blockSize.z - 1) / blockSize.z
		);

		for (int it = 0; it < 7; it++) {
			cuda_rho_adv_12_x<<<blocksX, blockSize>>>(_grid->cudaData(), 0);
			cuda_rho_adv_12_x<<<blocksX, blockSize>>>(_grid->cudaData(), 1);
			//cudaDeviceSynchronize();

			cuda_rho_adv_12_y<<<blocksY, blockSize>>>(_grid->cudaData(), 0);
			cuda_rho_adv_12_y<<<blocksY, blockSize>>>(_grid->cudaData(), 1);
			//cudaDeviceSynchronize();

			cuda_rho_adv_12_z<<<blocksZ, blockSize>>>(_grid->cudaData(), 0);
			cuda_rho_adv_12_z<<<blocksZ, blockSize>>>(_grid->cudaData(), 1);
			//cudaDeviceSynchronize();
		}
	}
}

void CudaSimulator3D::rho_sharpen() {
	constexpr dim3 blockSize(4, 4, 4);
	const dim3 gridSize(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);

	cuda_rho_sharpen_calc<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(DT) * 3.f);
	cuda_rho_sharpen_apply<<<gridSize, blockSize>>>(_grid->cudaData());
	cuda_rho_sharpen_scatter<<<gridSize, blockSize>>>(_grid->cudaData(), PROPERTY_F(RHO_D), 10);
}

void CudaSimulator3D::add_force() {
	constexpr dim3 blockSize(4, 4, 4);
	const dim3 gridSize(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);

	cuda_add_force<<<gridSize, blockSize>>>(
		_grid->cudaData(),
		PROPERTY_F(DT),
		PROPERTY_B(GRAVITY_ENABLED) ? PROPERTY_F(GRAVITY_FORCE) : 0.f,
		!PROPERTY_B(DISABLE_VEL_DECAY)
	);
}

void CudaSimulator3D::vel_advect() {
	constexpr dim3 blockSize(4, 4, 4);
	const dim3 gridSize(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);

	cuda_vel_adv_pass_1<<<gridSize, blockSize>>>(
		_grid->cudaData(),
		PROPERTY_F(DT),
		4);

	cuda_vel_adv_pass_2<<<gridSize, blockSize>>>(_grid->cudaData());
}

void CudaSimulator3D::project() {
	Profiler* profiler = Profiler::Get();
	constexpr dim3 blockSize(4, 4, 4);
	const dim3 gridSizeCore(
		(_grid->width()  - 2 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 2 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 2 + blockSize.z - 1) / blockSize.z);
	const dim3 gridSizeFull(
		(_grid->width()  - 1 + blockSize.x - 1) / blockSize.x,
		(_grid->height() - 1 + blockSize.y - 1) / blockSize.y,
		(_grid->depth()  - 1 + blockSize.z - 1) / blockSize.z);

	cuda_proj_enf_bound<<<gridSizeFull, blockSize>>>(_grid->cudaData());

	profiler->start("Calc Divergence");
	_grid->clearProj();
	cuda_proj_calc_div<<<gridSizeCore, blockSize>>>(
		_grid->cudaData(),
		PROPERTY_F(LAMBDA_PLUS),
		PROPERTY_F(LAMBDA_MINUS),
		PROPERTY_F(ETA));
	profiler->stop("Calc Divergence");

	profiler->start("Solve");
	const float err = _solver->solve(_grid);
	profiler->stop("Solve");
	if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
		fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
		SET_PROPERTY_B(PAUSED, true);
		return;
	}

	profiler->start("Sub PG");
	cuda_proj_sub_pg<<<gridSizeCore, blockSize>>>(_grid->cudaData());
	profiler->stop("Sub PG");

	cuda_proj_enf_bound<<<gridSizeFull, blockSize>>>(_grid->cudaData());
}

__device__ bool cuda_in_wall(const glm::vec3 p, const CudaGrid3D::CudaData *grid) {
	if (p.x < 1.0 || p.x > grid->width  - 2.0 ||
	    p.y < 1.0 || p.y > grid->height - 2.0 ||
		p.z < 1.0 || p.z > grid->depth  - 2.0 )
		return true;
	return grid->boundary[cuda_idx_3D(
		cuda_clamp((unsigned int)p.x, 0u, grid->width-1u),
		cuda_clamp((unsigned int)p.y, 0u, grid->height-1u),
		cuda_clamp((unsigned int)p.z, 0u, grid->depth-1u),
		grid->width, grid->height)] > 0.5f;
}
__device__ glm::vec3 cuda_domain_clamp(const glm::vec3 p, const CudaGrid3D::CudaData *grid) {
	return {
		cuda_clamp(p.x, 1.0f, static_cast<float>(grid->width)  - 2.0f),
		cuda_clamp(p.y, 1.0f, static_cast<float>(grid->height) - 2.0f),
		cuda_clamp(p.z, 1.0f, static_cast<float>(grid->depth)  - 2.0f)
	};
}
__device__ glm::vec3 cuda_staggered_offset_3D(const int comp) {
	switch (comp) {
		case 0:  return glm::vec3(-1.0f, -0.5f, -0.5f);
		case 1:  return glm::vec3(-0.5f, -1.0f, -0.5f);
		case 2:
		default: return glm::vec3(-0.5f, -0.5f, -1.0f);
	}
}
__device__ float cuda_trilinear_interpolation(const glm::vec3 &pos,
	const int low_x, const int low_y, const int low_z, const int comp,
	glm::vec3* vel, const unsigned int w, const unsigned int h)
{
	const glm::vec3 off = cuda_staggered_offset_3D(comp);

	const float x0 = low_x + off.x;
	const float y0 = low_y + off.y;
	const float z0 = low_z + off.z;

	const float tx = pos.x - x0;
	const float ty = pos.y - y0;
	const float tz = pos.z - z0;

	const float v000 = vel[cuda_idx_3D(low_x,     low_y,     low_z    , w, h)][comp];
	const float v001 = vel[cuda_idx_3D(low_x,     low_y,     low_z + 1, w, h)][comp];
	const float v010 = vel[cuda_idx_3D(low_x,     low_y + 1, low_z    , w, h)][comp];
	const float v011 = vel[cuda_idx_3D(low_x,     low_y + 1, low_z + 1, w, h)][comp];
	const float v100 = vel[cuda_idx_3D(low_x + 1, low_y,     low_z    , w, h)][comp];
	const float v101 = vel[cuda_idx_3D(low_x + 1, low_y,     low_z + 1, w, h)][comp];
	const float v110 = vel[cuda_idx_3D(low_x + 1, low_y + 1, low_z    , w, h)][comp];
	const float v111 = vel[cuda_idx_3D(low_x + 1, low_y + 1, low_z + 1, w, h)][comp];

	const float v00 = v000 * (1.0f - tz) + v001 * tz;
	const float v01 = v010 * (1.0f - tz) + v011 * tz;
	const float v10 = v100 * (1.0f - tz) + v101 * tz;
	const float v11 = v110 * (1.0f - tz) + v111 * tz;

	const float vLow  = v00 * (1.0f - ty) + v01 * ty;
	const float vHigh = v10 * (1.0f - ty) + v11 * ty;

	return vLow * (1.0f - tx) + vHigh * tx;
}
__device__ glm::vec3 cuda_sample_vel(glm::vec3 pos, glm::vec3* vel, unsigned int w, unsigned int h) {
	auto lowIndex = [](const float coord, const float offset) {
		return (int)(std::floor(coord - offset) + 1.0e-10f);
	};

	const glm::vec3 offU = cuda_staggered_offset_3D(0);
	const int lx_u = lowIndex(pos.x, offU.x);
	const int ly_u = lowIndex(pos.y, offU.y);
	const int lz_u = lowIndex(pos.z, offU.z);
	const float vx = cuda_trilinear_interpolation(pos, lx_u, ly_u, lz_u, 0, vel, w, h);

	const glm::vec3 offV = cuda_staggered_offset_3D(1);
	const int lx_v = lowIndex(pos.x, offV.x);
	const int ly_v = lowIndex(pos.y, offV.y);
	const int lz_v = lowIndex(pos.z, offV.z);
	const float vy = cuda_trilinear_interpolation(pos, lx_v, ly_v, lz_v, 1, vel, w, h);

	const glm::vec3 offW = cuda_staggered_offset_3D(2);
	const int lx_w = lowIndex(pos.x, offW.x);
	const int ly_w = lowIndex(pos.y, offW.y);
	const int lz_w = lowIndex(pos.z, offW.z);
	const float vz = cuda_trilinear_interpolation(pos, lx_w, ly_w, lz_w, 2, vel, w, h);

	return glm::vec3(vx, vy, vz);
}
__device__ glm::vec3 cuda_trace_particle(const glm::vec3 start, const CudaGrid3D::CudaData *grid, const float dt, const int subSteps) {
	if (cuda_in_wall(start, grid)) return start;

	float subDt = -dt / (float)subSteps;   // backward
	glm::vec3 pos = start;

	for (int s = 0; s < subSteps; s++) {
		glm::vec3 k1   = cuda_sample_vel(pos - 0.5f, grid->vel, grid->width, grid->height);
		glm::vec3 mid  = cuda_domain_clamp(pos + k1 * (subDt * 0.5f), grid);
		glm::vec3 k2   = cuda_sample_vel(mid - 0.5f, grid->vel, grid->width, grid->height);
		const glm::vec3 next = pos + k2 * subDt;

		if (cuda_in_wall(next, grid)) {
			// Binary search: find last safe position before the wall
			glm::vec3 safe = pos;
			glm::vec3 unsafe = next;
			for (int b = 0; b < 8; b++) {
				glm::vec3 h = (safe + unsafe) * 0.5f;
				if (cuda_in_wall(h, grid))
					unsafe = h;
				else
					safe   = h;
			}
			return safe;
		}
		pos = next;
	}
	return pos;
}
__device__ float cuda_sample_scal_field(glm::vec3 pos, const float* field, unsigned int w, unsigned int h, unsigned int d) {
	const int x = (int)pos.x;
    const int y = (int)pos.y;
    const int z = (int)pos.z;

    int ix1 = x;
    int ix2 = x + 1;
    if (pos.x - (float)x < 0.5f) {
        ix1 = x - 1;
        ix2 = x;
    }
    int iy1 = y;
    int iy2 = y + 1;
    if (pos.y - (float)y < 0.5f) {
        iy1 = y - 1;
        iy2 = y;
    }
    int iz1 = z;
    int iz2 = z + 1;
    if (pos.z - (float)z < 0.5f) {
        iz1 = z - 1;
        iz2 = z;
    }

    ix1 = max(0, min(ix1, static_cast<int>(w) - 1));
    ix2 = max(0, min(ix2, static_cast<int>(w) - 1));
    iy1 = max(0, min(iy1, static_cast<int>(h) - 1));
    iy2 = max(0, min(iy2, static_cast<int>(h) - 1));
    iz1 = max(0, min(iz1, static_cast<int>(d) - 1));
    iz2 = max(0, min(iz2, static_cast<int>(d) - 1));

    const float v[8] = {
        field[cuda_idx_3D(ix1, iy1, iz1, w, h)],
        field[cuda_idx_3D(ix1, iy1, iz2, w, h)],
        field[cuda_idx_3D(ix1, iy2, iz1, w, h)],
        field[cuda_idx_3D(ix1, iy2, iz2, w, h)],
        field[cuda_idx_3D(ix2, iy1, iz1, w, h)],
        field[cuda_idx_3D(ix2, iy1, iz2, w, h)],
        field[cuda_idx_3D(ix2, iy2, iz1, w, h)],
        field[cuda_idx_3D(ix2, iy2, iz2, w, h)],
    };

    const float x1 = (float)ix1 + 0.5f;
    const float x2 = (float)ix2 + 0.5f;
    const float y1 = (float)iy1 + 0.5f;
    const float y2 = (float)iy2 + 0.5f;
    const float z1 = (float)iz1 + 0.5f;
    const float z2 = (float)iz2 + 0.5f;

    const float dz2z1 = z2 - z1;
    float f00, f01, f10, f11;
    if (dz2z1 < 1e-9f) {
        f00 = v[1];
        f01 = v[3];
        f10 = v[5];
        f11 = v[7];
    } else {
        const float wz1 = (z2 - pos.z) / dz2z1;
        const float wz2 = (pos.z - z1) / dz2z1;
        f00 = wz1 * v[0] + wz2 * v[1];
        f01 = wz1 * v[2] + wz2 * v[3];
        f10 = wz1 * v[4] + wz2 * v[5];
        f11 = wz1 * v[6] + wz2 * v[7];
    }

    const float dy2y1 = y2 - y1;
    float fx1, fx2; // at ix1, ix2
    if (dy2y1 < 1e-9f) {
        fx1 = f01;
        fx2 = f11;
    } else {
        const float wy1 = (y2 - pos.y) / dy2y1;
        const float wy2 = (pos.y - y1) / dy2y1;
        fx1 = wy1 * f00 + wy2 * f01;
        fx2 = wy1 * f10 + wy2 * f11;
    }

    const float dx2x1 = x2 - x1;
    float fxyz;
    if (dx2x1 < 1e-9f) {
        fxyz = fx2;
    } else {
        const float wx1 = (x2 - pos.x) / dx2x1;
        const float wx2 = (pos.x - x1) / dx2x1;
        fxyz = wx1 * fx1 + wx2 * fx2;
    }

    return fxyz;
}
__device__ glm::vec3 cuda_trace_along_field(const CudaGrid3D::CudaData* grid, glm::vec3 pos, float D, int substeps) {
	const float stepSize = D / (float)substeps;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;
	const unsigned int d = grid->depth;

	for (int s = 0; s < substeps; s++) {
		if (cuda_sample_scal_field(pos, grid->rho, w, h, d) >= 0.5f)
			break;

		const unsigned int ix = max(1u, min((unsigned int)pos.x, (unsigned int)w - 2));
		const unsigned int iy = max(1u, min((unsigned int)pos.y, (unsigned int)h - 2));
		const unsigned int iz = max(1u, min((unsigned int)pos.z, (unsigned int)d - 2));

		glm::vec3 grad(
			grid->rho[cuda_idx_3D(ix+1, iy,   iz,   w, h)] - grid->rho[cuda_idx_3D(ix-1, iy,   iz,   w, h)],
			grid->rho[cuda_idx_3D(ix,   iy+1, iz,   w, h)] - grid->rho[cuda_idx_3D(ix,   iy-1, iz,   w, h)],
			grid->rho[cuda_idx_3D(ix,   iy,   iz+1, w, h)] - grid->rho[cuda_idx_3D(ix,   iy,   iz-1, w, h)]
		);
		const float len = sqrtf(grad.x*grad.x + grad.y*grad.y + grad.z*grad.z);
		if (len < 1e-6f) break;

		glm::vec3 next = pos + (grad / len) * stepSize;
		next.x = cuda_clamp(next.x, 1.f, (float)w - 2.f);
		next.y = cuda_clamp(next.y, 1.f, (float)h - 2.f);
		next.z = cuda_clamp(next.z, 1.f, (float)d - 2.f);

		if (grid->boundary[cuda_idx_3D(next.x, next.y, next.z, w, h)] > 0.5f)
			break;
		pos = next;
	}
	return pos;
}
__device__ void cuda_scatter_rho(const CudaGrid3D::CudaData* grid, glm::vec3 pos, float value) {
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;
	const unsigned int d = grid->depth;

	const int x0 = max(0, min((int)pos.x,     (int)w - 1));
	const int y0 = max(0, min((int)pos.y,     (int)h - 1));
	const int z0 = max(0, min((int)pos.z,     (int)d - 1));
	const int x1 = max(0, min((int)pos.x + 1, (int)w - 1));
	const int y1 = max(0, min((int)pos.y + 1, (int)h - 1));
	const int z1 = max(0, min((int)pos.z + 1, (int)d - 1));

	const float fx = pos.x - floorf(pos.x);
	const float fy = pos.y - floorf(pos.y);
	const float fz = pos.z - floorf(pos.z);

	float w000 = (1.f-fx) * (1.f-fy) * (1.f-fz);
	float w100 =      fx  * (1.f-fy) * (1.f-fz);
	float w010 = (1.f-fx) *      fy  * (1.f-fz);
	float w110 =      fx  *      fy  * (1.f-fz);
	float w001 = (1.f-fx) * (1.f-fy) *      fz;
	float w101 =      fx  * (1.f-fy) *      fz;
	float w011 = (1.f-fx) *      fy  *      fz;
	float w111 =      fx  *      fy  *      fz;

	if (grid->boundary[cuda_idx_3D(x0, y0, z0, w, h)] > 0.5f) w000 = 0.f;
	if (grid->boundary[cuda_idx_3D(x1, y0, z0, w, h)] > 0.5f) w100 = 0.f;
	if (grid->boundary[cuda_idx_3D(x0, y1, z0, w, h)] > 0.5f) w010 = 0.f;
	if (grid->boundary[cuda_idx_3D(x1, y1, z0, w, h)] > 0.5f) w110 = 0.f;
	if (grid->boundary[cuda_idx_3D(x0, y0, z1, w, h)] > 0.5f) w001 = 0.f;
	if (grid->boundary[cuda_idx_3D(x1, y0, z1, w, h)] > 0.5f) w101 = 0.f;
	if (grid->boundary[cuda_idx_3D(x0, y1, z1, w, h)] > 0.5f) w011 = 0.f;
	if (grid->boundary[cuda_idx_3D(x1, y1, z1, w, h)] > 0.5f) w111 = 0.f;

	const float total = w000 + w100 + w010 + w110 + w001 + w101 + w011 + w111;
	if (total < 1e-6f)
	    return;
	const float inv = 1.f / total;

	atomicAdd(&grid->rho[cuda_idx_3D(x0, y0, z0, w, h)], value * w000 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x1, y0, z0, w, h)], value * w100 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x0, y1, z0, w, h)], value * w010 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x1, y1, z0, w, h)], value * w110 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x0, y0, z1, w, h)], value * w001 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x1, y0, z1, w, h)], value * w101 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x0, y1, z1, w, h)], value * w011 * inv);
	atomicAdd(&grid->rho[cuda_idx_3D(x1, y1, z1, w, h)], value * w111 * inv);
}


__global__ void cuda_vel_decay(const CudaGrid3D::CudaData* grid, const float decay) {
    const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
    if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
        return;

    const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
    if (grid->rho[i] >= 0.5f)
        return;
    grid->vel[i] *= decay;

}
__global__ void cuda_vel_extrap_init(const CudaGrid3D::CudaData* grid, glm::vec3* valid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	auto isFluid = [&](unsigned int cx, unsigned int cy, unsigned int cz) {
		const unsigned int ci = cuda_idx_3D(cx, cy, cz, grid->width, grid->height);
		return grid->rho[ci] >= 0.5f && grid->boundary[ci] < 0.5f;
	};

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const bool self = isFluid(x, y, z);

	valid[i] = glm::vec3(
		(self || isFluid(x - 1, y, z)) ? 1.f : 0.f,
		(self || isFluid(x, y - 1, z)) ? 1.f : 0.f,
		(self || isFluid(x, y, z - 1)) ? 1.f : 0.f
	);
}
__global__ void cuda_vel_extrapolate(const CudaGrid3D::CudaData* grid, const glm::vec3* srcVel, const glm::vec3* srcValid, glm::vec3* dstVel, glm::vec3* dstValid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	if (grid->boundary[i] > 0.5f) {
		dstVel[i]   = srcVel[i];
		dstValid[i] = glm::vec3(0.f);
		return;
	}

	glm::vec3 result   = srcVel[i];
	glm::vec3 validOut = srcValid[i];

	glm::vec3 sum(0.f);
	glm::vec3 n(0.f);
	auto sample = [&](unsigned int nx, unsigned int ny, unsigned int nz) {
		const unsigned int ni = cuda_idx_3D(nx, ny, nz, grid->width, grid->height);
		const glm::vec3& nv = srcValid[ni];
		const glm::vec3& nvel = srcVel[ni];
		if (nv.x > 0.5f) { sum.x += nvel.x; n.x++; }
		if (nv.y > 0.5f) { sum.y += nvel.y; n.y++; }
		if (nv.z > 0.5f) { sum.z += nvel.z; n.z++; }
	};
	sample(x - 1, y, z); sample(x + 1, y, z);
	sample(x, y - 1, z); sample(x, y + 1, z);
	sample(x, y, z - 1); sample(x, y, z + 1);

	if (validOut.x < 0.5f && n.x > 0.f) { result.x = sum.x / n.x; validOut.x = 1.f; }
	if (validOut.y < 0.5f && n.y > 0.f) { result.y = sum.y / n.y; validOut.y = 1.f; }
	if (validOut.z < 0.5f && n.z > 0.f) { result.z = sum.z / n.z; validOut.z = 1.f; }

	dstVel[i]   = result;
	dstValid[i] = validOut;
}
__global__ void cuda_rho_sharpen_calc(const CudaGrid3D::CudaData* grid, const float dt) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	grid->del_rho[i] = 0.f;
	if (grid->boundary[i] > 0.5f)
		return;

	const float rC = grid->rho[i];
	const float rL = grid->rho[cuda_idx_3D(x-1, y,   z  , grid->width, grid->height)];
	const float rR = grid->rho[cuda_idx_3D(x+1, y,   z  , grid->width, grid->height)];
	const float rD = grid->rho[cuda_idx_3D(x,   y-1, z  , grid->width, grid->height)];
	const float rU = grid->rho[cuda_idx_3D(x,   y+1, z  , grid->width, grid->height)];
	const float rB = grid->rho[cuda_idx_3D(x,   y,   z-1, grid->width, grid->height)];
	const float rF = grid->rho[cuda_idx_3D(x,   y,   z+1, grid->width, grid->height)];

	const float dpx = -(rC - rL) * dt;
	const float dpy = -(rC - rD) * dt;
	const float dpz = -(rC - rB) * dt;
	const float dmx = -(rR - rC) * dt;
	const float dmy = -(rU - rC) * dt;
	const float dmz = -(rF - rC) * dt;

	const float max_inc = sqrtf(
	fmaxf(
		powf(fmaxf(dpx,0.f),2.f),
		powf(fminf(dmx,0.f),2.f)) +
	fmaxf(
		powf(fmaxf(dpy,0.f),2.f),
		powf(fminf(dmy,0.f),2.f)) +
	fmaxf(
		powf(fmaxf(dpz,0.f),2.f),
		powf(fminf(dmz,0.f),2.f)));

	const float max_dec = sqrtf(
		fmaxf(
			powf(fminf(dpx,0.f),2.f),
			powf(fmaxf(dmx,0.f),2.f)) +
		fmaxf(
			powf(fminf(dpy,0.f),2.f),
			powf(fmaxf(dmy,0.f),2.f)) +
		fmaxf(
			powf(fminf(dpz,0.f),2.f),
			powf(fmaxf(dmz,0.f),2.f)));

	const float maxDiff = fmaxf(fmaxf(
							fmaxf(fabsf(rC-rL), fabsf(rC-rR)),
							fmaxf(fabsf(rC-rD), fabsf(rC-rU))),
							fmaxf(fabsf(rC-rB), fabsf(rC-rF)));

	const float w = powf(rC - 0.5f, 3.f) * (1.f - fminf(1.f, maxDiff / 0.2f));

	float dr = (w >= 0.f) ? w * max_inc : w * max_dec;

	if (rC + dr < 0.f || rC < 1e-5f)
		dr = -rC;
	else if (rC > 0.5f)
		dr = 0.f;

	grid->del_rho[i] = dr;
}
__global__ void cuda_rho_sharpen_apply(const CudaGrid3D::CudaData* grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	grid->rho[i] = fmaxf(0.f, grid->rho[i] + grid->del_rho[i]);
}
__global__ void cuda_rho_sharpen_scatter(const CudaGrid3D::CudaData* grid, const float D, const int substeps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const float dr = grid->del_rho[i];
	if (fabsf(dr) < 1e-6f) return;

	const glm::vec3 p = cuda_trace_along_field(grid, glm::vec3(x, y, z), D, substeps);
	cuda_scatter_rho(grid, p, -dr);
}
// (1) Advect gamma using backward semi-lagrangian
__global__ void cuda_rho_adv_1(const CudaGrid3D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	if (grid->boundary[i] > 0.5)
		return;

	const glm::vec3 prevPos = cuda_trace_particle(glm::vec3(x, y, z), grid, dt, subSteps);
	grid->new_gamma[i] = cuda_sample_scal_field(prevPos, grid->gamma, grid->width, grid->height, grid->depth);

}
// (2) Compute A + (3) Scale A by gamma
__global__ void cuda_rho_adv_2_3(const CudaGrid3D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	grid->counts_bwd[i] = 0;

	if (grid->boundary[i] > 0.5f)
		return;

	const glm::vec3 prevPos = cuda_trace_particle(glm::vec3(x, y, z), grid, dt, subSteps);
	const int p0x = floor(prevPos.x);
	const int p0y = floor(prevPos.y);
	const int p0z = floor(prevPos.z);

	const float dx0 = 1 - (prevPos.x - p0x);
	const float dy0 = 1 - (prevPos.y - p0y);
	const float dz0 = 1 - (prevPos.z - p0z);
	const float dx1 = 1 - (p0x + 1 - prevPos.x);
	const float dy1 = 1 - (p0y + 1 - prevPos.y);
	const float dz1 = 1 - (p0z + 1 - prevPos.z);

	const unsigned int dest[8] = {
		cuda_idx_3D(p0x + 0, p0y + 0, p0z + 0, w, h),
		cuda_idx_3D(p0x + 1, p0y + 0, p0z + 0, w, h),
		cuda_idx_3D(p0x + 0, p0y + 1, p0z + 0, w, h),
		cuda_idx_3D(p0x + 1, p0y + 1, p0z + 0, w, h),
		cuda_idx_3D(p0x + 0, p0y + 0, p0z + 1, w, h),
		cuda_idx_3D(p0x + 1, p0y + 0, p0z + 1, w, h),
		cuda_idx_3D(p0x + 0, p0y + 1, p0z + 1, w, h),
		cuda_idx_3D(p0x + 1, p0y + 1, p0z + 1, w, h),
	};

	float m[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	m[0] = dx0 * dy0 * dz0 * (1 - grid->boundary[dest[0]]);
	m[1] = dx1 * dy0 * dz0 * (1 - grid->boundary[dest[1]]);
	m[2] = dx0 * dy1 * dz0 * (1 - grid->boundary[dest[2]]);
	m[3] = dx1 * dy1 * dz0 * (1 - grid->boundary[dest[3]]);
	m[4] = dx0 * dy0 * dz1 * (1 - grid->boundary[dest[4]]);
	m[5] = dx1 * dy0 * dz1 * (1 - grid->boundary[dest[5]]);
	m[6] = dx0 * dy1 * dz1 * (1 - grid->boundary[dest[6]]);
	m[7] = dx1 * dy1 * dz1 * (1 - grid->boundary[dest[7]]);
	float sum = 0;
	for (int k = 0; k < 8; k++)
		sum += m[k];
	if (sum == 0)
		return;
	float g = grid->new_gamma[i];
	if(g < 1e-5)
		g = 1.0;

	unsigned int k = 0u;
	for (int c = 0; c < 8; c++) {
		if (m[c] > 0.0) {
			grid->A_bwd[i * 8 + k] = CudaGrid3D::AdvCell{
				.j = dest[c],
				.val = m[c]/(sum * g)
			};
			k++;
		}
	}

	grid->counts_bwd[i] = k;
}
// (4) Compute beta
__global__ void cuda_rho_adv_4(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	for (unsigned int k = 0; k < grid->counts_bwd[i]; k++) {
		const uint slot = (i * 8) + k;
		atomicAdd(&grid->beta[grid->A_bwd[slot].j], grid->A_bwd[slot].val);
	}
}
// (5) forward trace the velocity field to add the weights to A for all cells j where beta j < 1
__global__ void cuda_rho_adv_5(const CudaGrid3D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	if (grid->beta[i] >= 1)
		return;

	grid->counts_fwd[i] = 0;
	if (grid->boundary[i] > 0.5f)
		return;

	const glm::vec3 nextPos = cuda_trace_particle(glm::vec3(x, y, z), grid, -dt, subSteps);
	const int p0x = floor(nextPos.x);
	const int p0y = floor(nextPos.y);
	const int p0z = floor(nextPos.z);

	const float dx0 = 1 - (nextPos.x - p0x);
	const float dy0 = 1 - (nextPos.y - p0y);
	const float dz0 = 1 - (nextPos.z - p0z);
	const float dx1 = 1 - (p0x + 1 - nextPos.x);
	const float dy1 = 1 - (p0y + 1 - nextPos.y);
	const float dz1 = 1 - (p0z + 1 - nextPos.z);

	const unsigned int dest[8] = {
		cuda_idx_3D(p0x + 0, p0y + 0, p0z + 0, w, h),
		cuda_idx_3D(p0x + 1, p0y + 0, p0z + 0, w, h),
		cuda_idx_3D(p0x + 0, p0y + 1, p0z + 0, w, h),
		cuda_idx_3D(p0x + 1, p0y + 1, p0z + 0, w, h),
		cuda_idx_3D(p0x + 0, p0y + 0, p0z + 1, w, h),
		cuda_idx_3D(p0x + 1, p0y + 0, p0z + 1, w, h),
		cuda_idx_3D(p0x + 0, p0y + 1, p0z + 1, w, h),
		cuda_idx_3D(p0x + 1, p0y + 1, p0z + 1, w, h),
	};

	float m[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	m[0] = dx0 * dy0 * dz0 * (1 - grid->boundary[dest[0]]);
	m[1] = dx1 * dy0 * dz0 * (1 - grid->boundary[dest[1]]);
	m[2] = dx0 * dy1 * dz0 * (1 - grid->boundary[dest[2]]);
	m[3] = dx1 * dy1 * dz0 * (1 - grid->boundary[dest[3]]);
	m[4] = dx0 * dy0 * dz1 * (1 - grid->boundary[dest[4]]);
	m[5] = dx1 * dy0 * dz1 * (1 - grid->boundary[dest[5]]);
	m[6] = dx0 * dy1 * dz1 * (1 - grid->boundary[dest[6]]);
	m[7] = dx1 * dy1 * dz1 * (1 - grid->boundary[dest[7]]);
	float sum = 0;
	for (int k = 0; k < 8; k++)
		sum += m[k];
	if (sum == 0)
		return;

	const float b = 1.f - grid->beta[i];
	unsigned int k = 0;
	for (int c = 0; c < 8; c++) {
		if (m[c] > 0.0) {
			grid->A_fwd[i * 8 + k] = CudaGrid3D::AdvCell{
				.i = dest[c],
				.val = b * (m[c] / sum)
			};
			k++;
		}
	}

	grid->counts_fwd[i] = k;
}
// (6) Compute gamma
__global__ void cuda_rho_adv_6_10_bwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	float g = 0;
	for (int k = 0; k < grid->counts_bwd[i]; k++)
		g += grid->A_bwd[(i * 8) + k].val;

	grid->gamma[i] = g;
}
__global__ void cuda_rho_adv_6_10_fwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int j = cuda_idx_3D(x, y, z, grid->width, grid->height);
	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		atomicAdd(&grid->gamma[grid->A_fwd[(j * 8) + k].i], grid->A_fwd[(j * 8) + k].val);
}
// (7) Scale A by gamma
__global__ void cuda_rho_adv_7_bwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const float g = grid->gamma[i];
	if (g < 1e-5)
		return;

	for (uint k = 0; k < grid->counts_bwd[i]; k++)
		grid->A_bwd[(i * 8) + k].val /= g;
}
__global__ void cuda_rho_adv_7_fwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int j = cuda_idx_3D(x, y, z, grid->width, grid->height);
	for (uint k = 0u; k < grid->counts_fwd[j]; k++) {
		const float g = grid->gamma[grid->A_fwd[(j * 8) + k].j];
		if (g > 1e-5)
			grid->A_fwd[(j * 8) + k].val /= g;
	}
}
// (8) Recompute beta from A; !! Order is important - must call fwd before bwd
__global__ void cuda_rho_adv_8_bwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	for (uint k = 0u; k < grid->counts_bwd[i]; k++)
		atomicAdd(&grid->beta[grid->A_bwd[(i * 8) + k].j], grid->A_bwd[(i * 8) + k].val);
}
__global__ void cuda_rho_adv_8_fwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int j = cuda_idx_3D(x, y, z, grid->width, grid->height);
	float b = 0.0;
	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		b += grid->A_fwd[(j * 8) + k].val;

	grid->beta[j] = b;
}
// (9) Clamp beta to 1
__global__ void cuda_rho_adv_9_bwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	for (uint k = 0u; k < grid->counts_bwd[i]; k++) {
		const float b = grid->beta[grid->A_bwd[(i * 8) + k].j];
		if (b > 1e-5)
			grid->A_bwd[(i * 8) + k].val /= b;
	}
}
__global__ void cuda_rho_adv_9_fwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int j = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const float b = grid->beta[j];
	if (b <= 1e-5)
		return;

	for (uint k = 0u; k < grid->counts_fwd[j]; k++)
		grid->A_fwd[(j * 8) + k].val /= b;
}
// (11) Evaluate p_n+1 = A * p
__global__ void cuda_rho_adv_11_bwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	float acc  = 0.0;
	for (uint k = 0u; k < grid->counts_bwd[i]; k++) {
		const CudaGrid3D::AdvCell c = grid->A_bwd[(i * 8) + k];
		acc += c.val * grid->rho[c.j];
	}

	grid->tmp_rho[i] = acc;
}
__global__ void cuda_rho_adv_11_fwd(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int j = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const float rho_j = grid->rho[j];
	for (uint k = 0u; k < grid->counts_fwd[j]; k++) {
		const CudaGrid3D::AdvCell c = grid->A_fwd[(j * 8) + k];
		atomicAdd(&grid->tmp_rho[c.j], c.val * rho_j);
	}
}
__global__ void cuda_rho_adv_11_cpy(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;
	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	grid->rho[i] = grid->tmp_rho[i];
}
// (12) Diffusion of rho and gamma
__global__ void cuda_rho_adv_12_x(const CudaGrid3D::CudaData *grid, const int offset) {
	const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int iy = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int iz = blockIdx.z * blockDim.z + threadIdx.z;

	const unsigned int x = 2 + (ix * 2) + offset;
	const unsigned int y = 1 + iy;
	const unsigned int z = 1 + iz;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int idx_j = cuda_idx_3D(x - 1, y, z, grid->width, grid->height);
	const unsigned int idx_i = cuda_idx_3D(x,     y, z, grid->width, grid->height);
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
__global__ void cuda_rho_adv_12_y(const CudaGrid3D::CudaData *grid, const int offset) {
	const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int iy = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int iz = blockIdx.z * blockDim.z + threadIdx.z;

	const unsigned int x = 1 + ix;
	const unsigned int y = 2 + (iy * 2) + offset;
	const unsigned int z = 1 + iz;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int idx_j = cuda_idx_3D(x, y - 1, z, grid->width, grid->height);
	const unsigned int idx_i = cuda_idx_3D(x, y    , z, grid->width, grid->height);
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
__global__ void cuda_rho_adv_12_z(const CudaGrid3D::CudaData *grid, const int offset) {
	const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int iy = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int iz = blockIdx.z * blockDim.z + threadIdx.z;

	const unsigned int x = 1 + ix;
	const unsigned int y = 1 + iy;
	const unsigned int z = 2 + (iz * 2) + offset;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int idx_j = cuda_idx_3D(x, y, z - 1, grid->width, grid->height);
	const unsigned int idx_i = cuda_idx_3D(x, y, z    , grid->width, grid->height);
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
__global__ void cuda_add_force(const CudaGrid3D::CudaData *grid, const float dt, const float gravity, const bool incAir) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	if (grid->rho[i] < 0.5f && !incAir)
		return;

	const unsigned int ix = cuda_idx_3D(x - 1, y, z, grid->width, grid->height);
	const unsigned int iy = cuda_idx_3D(x, y - 1, z, grid->width, grid->height);
	const unsigned int iz = cuda_idx_3D(x, y, z - 1, grid->width, grid->height);
	if (grid->boundary[i] != 1.f || grid->boundary[ix] != 1.f)
		grid->vel[i].x += grid->forces[i].x * dt;
	else
		grid->vel[i].x = 0.f;

	if (grid->boundary[i] != 1.f || grid->boundary[iy] != 1.f)
		grid->vel[i].y += (grid->forces[i].y - gravity) * dt;
	else
		grid->vel[i].y = 0.f;

	if (grid->boundary[i] != 1.f || grid->boundary[iz] != 1.f)
		grid->vel[i].z += grid->forces[i].z * dt;
	else
		grid->vel[i].z = 0.f;

}
__global__ void cuda_vel_adv_pass_1(const CudaGrid3D::CudaData *grid, const float dt, const int subSteps) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const glm::vec3 prevPos = cuda_trace_particle(glm::vec3(x, y, z), grid, dt, subSteps);
	grid->tmp_vel[i] = cuda_sample_vel(prevPos - 0.5f, grid->vel, grid->width, grid->height);
}
__global__ void cuda_vel_adv_pass_2(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);

	const glm::vec3 cur = grid->tmp_vel[i];
	auto out = glm::vec3(0);

	const float xBound = max(grid->boundary[i], grid->boundary[cuda_idx_3D(x-1, y, z, w, h)]);
	out.x = xBound * cur.x + (1 - xBound) * 0.5 * (cur.x + grid->tmp_vel[cuda_idx_3D(x-1, y, z, w, h)].x);

	const float yBound = max(grid->boundary[i], grid->boundary[cuda_idx_3D(x, y-1, z, w, h)]);
	out.y = yBound * cur.y + (1 - yBound) * 0.5 * (cur.y + grid->tmp_vel[cuda_idx_3D(x, y-1, z, w, h)].y);

	const float zBound = max(grid->boundary[i], grid->boundary[cuda_idx_3D(x, y, z-1, w, h)]);
	out.z = zBound * cur.z + (1 - zBound) * 0.5 * (cur.z + grid->tmp_vel[cuda_idx_3D(x, y, z-1, w, h)].z);

	grid->vel[i] = out;
}
__global__ void cuda_proj_calc_div(const CudaGrid3D::CudaData *grid, const float lambdaP, const float lambdaM, const float eta) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;
	const unsigned int w = grid->width;
	const unsigned int h = grid->height;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);

	const float r = grid->rho[i];
	if(r < 0.5f || grid->boundary[i] == 1)
		return;

	const float dx = grid->vel[cuda_idx_3D(x + 1, y, z, w, h)].x - grid->vel[i].x;
	const float dy = grid->vel[cuda_idx_3D(x, y + 1, z, w, h)].y - grid->vel[i].y;
	const float dz = grid->vel[cuda_idx_3D(x, y, z + 1, w, h)].z - grid->vel[i].z;
	float d = dx + dy + dz;

	if (r > 1.f)
		d -= min(lambdaM * (r - 1.f), eta);
	if (r < 1.f)
		d += min(lambdaP * (1.f - r), eta);

	grid->divergence[i] = d;
}
__global__ void cuda_proj_sub_pg(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z + 1;

	if (x >= grid->width - 1 || y >= grid->height - 1 || z >= grid->depth - 1)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const unsigned int ix = cuda_idx_3D(x - 1, y, z, grid->width, grid->height);
	const unsigned int iy = cuda_idx_3D(x, y - 1, z, grid->width, grid->height);
	const unsigned int iz = cuda_idx_3D(x, y, z - 1, grid->width, grid->height);

	if (grid->boundary[i] != 1.f && grid->boundary[ix] != 1.f)
		grid->vel[i].x -= (grid->pressure[i] - grid->pressure[ix]);
	else
		grid->vel[i].x = 0.f;

	if (grid->boundary[i] != 1.f && grid->boundary[iy] != 1.f)
		grid->vel[i].y -= (grid->pressure[i] - grid->pressure[iy]);
	else
		grid->vel[i].y = 0.f;

	if (grid->boundary[i] != 1.f && grid->boundary[iz] != 1.f)
		grid->vel[i].z -= (grid->pressure[i] - grid->pressure[iz]);
	else
		grid->vel[i].z = 0.f;
}
__global__ void cuda_proj_enf_bound(const CudaGrid3D::CudaData *grid) {
	const unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	const unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	const unsigned int z = blockIdx.z * blockDim.z + threadIdx.z;

	if (x >= grid->width || y >= grid->height || z >= grid->depth)
		return;

	const unsigned int i = cuda_idx_3D(x, y, z, grid->width, grid->height);
	const unsigned int ix = cuda_idx_3D(x - 1, y, z, grid->width, grid->height);
	const unsigned int iy = cuda_idx_3D(x, y - 1, z, grid->width, grid->height);
	const unsigned int iz = cuda_idx_3D(x, y, z - 1, grid->width, grid->height);
	if (grid->boundary[i] == 1.f || (x > 0 && grid->boundary[ix] == 1.f))
		grid->vel[i].x = 0.f;
	if (grid->boundary[i] == 1.f || (y > 0 && grid->boundary[iy] == 1.f))
		grid->vel[i].y = 0.f;
	if (grid->boundary[i] == 1.f || (z > 0 && grid->boundary[iz] == 1.f))
		grid->vel[i].z = 0.f;
}
