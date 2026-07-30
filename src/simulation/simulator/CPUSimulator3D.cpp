#include "simulation/simulator/CPUSimulator3D.h"

#include <ranges>

#include "simulation/data/Grid3D.h"
#include "simulation/Properties.h"
#include "simulation/solver/CGSolver.h"
#include "simulation/solver/JacobiSolver.h"
#include "util/Profiler.h"

#define BOUNDARY 1.f
#define LOOP_3D_GRID(x, y, z, offset)   for (int z = offset; z < _grid->depth()  - (offset); z++) \
                                        for (int y = offset; y < _grid->height() - (offset); y++) \
                                        for (int x = offset; x < _grid->width()  - (offset); x++)

CPUSimulator3D::CPUSimulator3D(const unsigned int width, const unsigned int height, const  unsigned int depth)
	: ISimulator3D(width, height, depth) {
	const bool validSolver = availableSolvers(std::vector({
			Properties::JACOBI,
			Properties::SELF_CG}));

	delete _solver;

	if (validSolver)
		switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
			case Properties::JACOBI:
				_solver = new JacobiSolverCPU3D(width, height, depth);
				break;
			case Properties::SELF_CG:
				_solver = new CGSolverCPU3D(_width, _height, _depth);
				break;
		default:
			break;
	}

    _grid = new Grid3D(_width, _height, _depth);
    _data = new SimulationDisplay3D::RenderData3D;
    _data->grid = _grid;
}

CPUSimulator3D::~CPUSimulator3D() {
    delete _grid;
	delete _solver;
}

void CPUSimulator3D::update() {
    ISimulator::update();
}

void* CPUSimulator3D::getRenderData() {
    return _data;
}

void CPUSimulator3D::vel_decay() {
	LOOP_3D_GRID(x, y, z, 1) {
		if (_grid->rho()(x, y, z) < 0.5)
			_grid->vel()(x, y, z) *= PROPERTY_F(VEL_DECAY);
	}
}

void CPUSimulator3D::vel_extrapolate() {
	auto& velField = _grid->vel();
	auto& newVel   = _grid->tmpVec();
	auto& boundary = _grid->boundary();
	auto& rho      = _grid->rho();

	auto validA = ScalField3D(_grid->width(), _grid->height(), _grid->depth());
	auto validB = ScalField3D(_grid->width(), _grid->height(), _grid->depth());

	LOOP_3D_GRID(x, y, z, 1) {
		validA(x, y, z) = (rho(x, y, z) >= 0.5f && boundary(x, y, z) != BOUNDARY) ? 1.f : 0.f;
	}

	auto extrapolate_pass = [&](auto& srcVel, auto& srcValid, auto& dstVel, auto& dstValid) {
		LOOP_3D_GRID(x, y, z, 1) {
			if (boundary(x, y, z) == BOUNDARY) {
				dstVel(x, y, z) = srcVel(x, y, z);
				dstValid(x, y, z) = 0.f;
				continue;
			}
			if (srcValid(x, y, z) > 0.5f) {
				dstVel(x, y, z) = srcVel(x, y, z);
				dstValid(x, y, z) = 1.f;
				continue;
			}

			glm::vec3 sum(0.f);
			float n = 0;
			auto sample = [&](const int nx, const int ny, const int nz) {
				if (srcValid(nx, ny, nz) > 0.5f) {
					sum += srcVel(nx, ny, nz);
					n++;
				}
			};
			sample(x - 1, y,     z);
			sample(x + 1, y,     z);
			sample(x,     y - 1, z);
			sample(x,     y + 1, z);
			sample(x,     y,     z - 1);
			sample(x,     y,     z + 1);

			dstVel(x, y, z)   = (n > 0) ? sum / n : srcVel(x, y, z);
			dstValid(x, y, z) = (n > 0) ? 1.f : 0.f;
		}
	};

	for (int i = 0; i < 3; i++) {
		extrapolate_pass(velField, validA, newVel,   validB);
		extrapolate_pass(newVel,   validB, velField, validA);
	}
}

void CPUSimulator3D::rho_advect() {
	auto& rho   = _grid->rho();
    auto& gamma = _grid->gamma();
	auto& beta = _grid->beta();
	auto& boundary = _grid->boundary();

	auto idx = [&](const int x, const int y, const int z) { return z * _width * _height + y * _width + x; };

    //################## Step 1 ##################
	// Advect gamma using backward semi-lagrangian
	auto advectedGamma = ScalField3D(gamma, false);
	LOOP_3D_GRID(x, y, z, 1){
		if (boundary(x, y, z) == BOUNDARY)
			continue;

		auto prevPos = _grid->traceParticle(glm::vec3(x, y, z), -PROPERTY_F(DT), 4);
		advectedGamma(x, y, z) = gamma.interpolate(prevPos);
	}
	gamma = advectedGamma;

	//################## Step 2 ##################
	// Compute A
	auto& A = _grid->A();
	A.clear();
	LOOP_3D_GRID(x, y, z, 1) {
		auto prevPos = _grid->traceParticle(glm::vec3(x, y, z), -PROPERTY_F(DT), 4);

		int p0x = floor(prevPos.x);
		int p0y = floor(prevPos.y);
		int p0z = floor(prevPos.z);

		auto dx0 = 1 - (prevPos.x - p0x);
		auto dy0 = 1 - (prevPos.y - p0y);
		auto dz0 = 1 - (prevPos.z - p0z);
		auto dx1 = 1 - (p0x + 1 - prevPos.x);
		auto dy1 = 1 - (p0y + 1 - prevPos.y);
		auto dz1 = 1 - (p0z + 1 - prevPos.z);

		float m[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		if (boundary(p0x + 0, p0y + 0, p0z + 0) != BOUNDARY) m[0] = dx0 * dy0 * dz0;
		if (boundary(p0x + 1, p0y + 0, p0z + 0) != BOUNDARY) m[1] = dx1 * dy0 * dz0;
		if (boundary(p0x + 0, p0y + 1, p0z + 0) != BOUNDARY) m[2] = dx0 * dy1 * dz0;
		if (boundary(p0x + 1, p0y + 1, p0z + 0) != BOUNDARY) m[3] = dx1 * dy1 * dz0;
		if (boundary(p0x + 0, p0y + 0, p0z + 1) != BOUNDARY) m[4] = dx0 * dy0 * dz1;
		if (boundary(p0x + 1, p0y + 0, p0z + 1) != BOUNDARY) m[5] = dx1 * dy0 * dz1;
		if (boundary(p0x + 0, p0y + 1, p0z + 1) != BOUNDARY) m[6] = dx0 * dy1 * dz1;
		if (boundary(p0x + 1, p0y + 1, p0z + 1) != BOUNDARY) m[7] = dx1 * dy1 * dz1;
		float sum = 0;
		for (int i = 0; i < 8; i++) sum += m[i];
		if (sum == 0)
			continue;

		_grid->addA(idx(x, y, z), idx(p0x + 0, p0y + 0, p0z + 0), m[0] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 1, p0y + 0, p0z + 0), m[1] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 0, p0y + 1, p0z + 0), m[2] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 1, p0y + 1, p0z + 0), m[3] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 0, p0y + 0, p0z + 1), m[4] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 1, p0y + 0, p0z + 1), m[5] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 0, p0y + 1, p0z + 1), m[6] / sum);
		_grid->addA(idx(x, y, z), idx(p0x + 1, p0y + 1, p0z + 1), m[7] / sum);
	}

	//################## Step 3 ##################
	// Scale A by gamma
	for (auto &cell: A | std::views::values)
	if (gamma[cell.i] > 1.e-5f)
		cell.val /= gamma[cell.i];

	//################## Step 4 ##################
	// Compute beta
	beta.clear();
	for (auto &cell: A | std::views::values)
		beta[cell.j] += cell.val;

	//################## Step 5 ##################
	// Forward trace the velocity field to add the weights to A for all cells j where beta j < 1
	LOOP_3D_GRID(x, y, z, 1) {
		const int j = idx(x, y, z);
		if (beta[j] >= 1) continue;

		auto nextPos = _grid->traceParticle(glm::vec3(x, y, z), +PROPERTY_F(DT), 4);

		int p0x = floor(nextPos.x);
		int p0y = floor(nextPos.y);
		int p0z = floor(nextPos.z);

		auto dx0 = 1 - (nextPos.x - p0x);
		auto dy0 = 1 - (nextPos.y - p0y);
		auto dz0 = 1 - (nextPos.z - p0z);
		auto dx1 = 1 - (p0x + 1 - nextPos.x);
		auto dy1 = 1 - (p0y + 1 - nextPos.y);
		auto dz1 = 1 - (p0z + 1 - nextPos.z);

		float m[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		if (boundary(p0x + 0, p0y + 0, p0z + 0) != BOUNDARY) m[0] = dx0 * dy0 * dz0;
		if (boundary(p0x + 1, p0y + 0, p0z + 0) != BOUNDARY) m[1] = dx1 * dy0 * dz0;
		if (boundary(p0x + 0, p0y + 1, p0z + 0) != BOUNDARY) m[2] = dx0 * dy1 * dz0;
		if (boundary(p0x + 1, p0y + 1, p0z + 0) != BOUNDARY) m[3] = dx1 * dy1 * dz0;
		if (boundary(p0x + 0, p0y + 0, p0z + 1) != BOUNDARY) m[4] = dx0 * dy0 * dz1;
		if (boundary(p0x + 1, p0y + 0, p0z + 1) != BOUNDARY) m[5] = dx1 * dy0 * dz1;
		if (boundary(p0x + 0, p0y + 1, p0z + 1) != BOUNDARY) m[6] = dx0 * dy1 * dz1;
		if (boundary(p0x + 1, p0y + 1, p0z + 1) != BOUNDARY) m[7] = dx1 * dy1 * dz1;
		float sum = 0;
		for (int i = 0; i < 8; i++) sum += m[i];
		if (sum == 0)
			continue;

		_grid->addA(idx(p0x + 0, p0y + 0, p0z + 0), j, (1-beta[j]) * (m[0] / sum));
		_grid->addA(idx(p0x + 1, p0y + 0, p0z + 0), j, (1-beta[j]) * (m[1] / sum));
		_grid->addA(idx(p0x + 0, p0y + 1, p0z + 0), j, (1-beta[j]) * (m[2] / sum));
		_grid->addA(idx(p0x + 1, p0y + 1, p0z + 0), j, (1-beta[j]) * (m[3] / sum));
		_grid->addA(idx(p0x + 0, p0y + 0, p0z + 1), j, (1-beta[j]) * (m[4] / sum));
		_grid->addA(idx(p0x + 1, p0y + 0, p0z + 1), j, (1-beta[j]) * (m[5] / sum));
		_grid->addA(idx(p0x + 0, p0y + 1, p0z + 1), j, (1-beta[j]) * (m[6] / sum));
		_grid->addA(idx(p0x + 1, p0y + 1, p0z + 1), j, (1-beta[j]) * (m[7] / sum));
	}

	//################## Step 6 ##################
	// Recompute gamma
	gamma.clear();
	for (auto &cell: A | std::views::values)
		gamma[cell.i] += cell.val;

	//################## Step 7 ##################
	// Scale A by gamma
	for (auto &cell: A | std::views::values)
	if (gamma[cell.i] > 1.e-5f)
		cell.val /= gamma[cell.i];

	//################## Step 8 ##################
	// Recompute beta from A
	beta.clear();
	for (auto &cell: A | std::views::values)
		beta[cell.j] += cell.val;

	//################## Step 9 ##################
	// Clamp beta to 1
	for (auto &cell: A | std::views::values)
	if (beta[cell.j] > 1.e-5f)
		cell.val /= beta[cell.j];

	/*beta.clear();
	for (unsigned int j = 0; j < _grid->size(); j++)
	for (unsigned int i = 0; i < _grid->size(); i++)
		beta[j] += A(i, j);*/

	//################## Step 10 ##################
	// Recompute gamma
	gamma.clear();
	for (auto &cell: A | std::views::values)
		gamma[cell.i] += cell.val;

	//################## Step 11 ##################
	// Evaluate p_n+1 = A * p
	auto& advectRho = _grid->tmpScal();
	advectRho.clear();
	for (auto &cell: A | std::views::values)
		advectRho[cell.i] += cell.val * rho[cell.j];
	LOOP_3D_GRID(x, y, z, 1)
		rho(x, y, z) = advectRho(x, y, z);

	//################## Step 12 ##################
	// Diffusion of rho and gamma
	for (int it = 0; it < 3; it++) {
		// X-dimension
		for (int z = 1; z < _grid->depth()  - 1; z++)
		for (int y = 1; y < _grid->height() - 1; y++)
		for (int x = 2; x < _grid->width()  - 1; x++) {
			float g_j = gamma(x-1, y, z);
			float g_i = gamma(x,   y, z);

			if (boundary(x, y, z) == BOUNDARY || boundary(x-1, y, z) == BOUNDARY)
				continue;

			if (g_j < 1e-5f || g_i < 1e-5f) continue;
			if (g_j == g_i) continue;

			float avg = (g_j + g_i) / 2.f;

			if (g_j > g_i) {
				float dif = rho(x-1, y, z) * (g_j - g_i) / (2.f * g_j);
				rho(x-1, y, z) -= dif;
				rho(x,   y, z) += dif;
			} else {
				float dif = rho(x, y, z) * (g_i - g_j) / (2.f * g_i);
				rho(x,   y, z) -= dif;
				rho(x-1, y, z) += dif;
			}

			gamma(x-1, y, z) = avg;
			gamma(x  , y, z) = avg;
		}

		// Y-dimension
		for (int z = 1; z < _grid->depth()  - 1; z++)
		for (int x = 1; x < _grid->width()  - 1; x++)
		for (int y = 2; y < _grid->height() - 1; y++) {
			float g_j = gamma(x, y-1, z);
			float g_i = gamma(x, y  , z);

			if (boundary(x, y, z) == BOUNDARY || boundary(x, y-1, z) == BOUNDARY)
				continue;

			if (g_j < 1e-5f || g_i < 1e-5f) continue;
			if (g_j == g_i) continue;

			float avg = (g_j + g_i) / 2.f;

			if (g_j > g_i) {
				float dif = rho(x, y-1, z) * (g_j - g_i) / (2.f * g_j);
				rho(x, y-1, z) -= dif;
				rho(x, y  , z) += dif;
			} else {
				float dif = rho(x, y, z) * (g_i - g_j) / (2.f * g_i);
				rho(x, y  , z) -= dif;
				rho(x, y-1, z) += dif;
			}

			gamma(x, y-1, z) = avg;
			gamma(x, y  , z) = avg;
		}

		// Z-dimension
		for (int y = 1; y < _grid->height() - 1; y++)
		for (int x = 1; x < _grid->width()  - 1; x++)
		for (int z = 2; z < _grid->depth()  - 1; z++) {
			float g_j = gamma(x, y, z-1);
			float g_i = gamma(x, y, z);

			if (boundary(x, y, z) == BOUNDARY || boundary(x, y, z-1) == BOUNDARY)
				continue;

			if (g_j < 1e-5f || g_i < 1e-5f) continue;
			if (g_j == g_i) continue;

			float avg = (g_j + g_i) / 2.f;

			if (g_j > g_i) {
				float dif = rho(x, y, z-1) * (g_j - g_i) / (2.f * g_j);
				rho(x, y, z-1) -= dif;
				rho(x, y, z  ) += dif;
			} else {
				float dif = rho(x, y, z) * (g_i - g_j) / (2.f * g_i);
				rho(x, y, z  ) -= dif;
				rho(x, y, z-1) += dif;
			}

			gamma(x, y, z-1) = avg;
			gamma(x, y, z  ) = avg;
		}
	}
}

void CPUSimulator3D::rho_sharpen() {
	auto& rho   = _grid->rho();
	const float delT = PROPERTY_F(DT) * 3;

	auto del_rho = ScalField3D(_grid->width(), _grid->height(), _grid->depth());

	// Calculate the mass change due to unit velocity
	auto del_plus = VecField3D(_grid->width(), _grid->height(), _grid->depth());
	auto del_minus = VecField3D(_grid->width(), _grid->height(), _grid->depth());
	LOOP_3D_GRID(x, y, z, 1) {
		del_plus(x, y, z).x = -(rho(x, y, z) - rho(x - 1, y, z)) * delT;
		del_minus(x, y, z).x = -(rho(x + 1, y, z) - rho(x, y, z)) * delT;

		del_plus(x, y, z).y = -(rho(x, y, z) - rho(x, y - 1, z)) * delT;
		del_minus(x, y, z).y = -(rho(x, y + 1, z) - rho(x, y, z)) * delT;

		del_plus(x, y, z).z = -(rho(x, y, z) - rho(x, y, z - 1)) * delT;
		del_minus(x, y, z).z = -(rho(x, y, z + 1) - rho(x, y, z)) * delT;
	}

	// Calculate the maximum mass increase and mass decrease due to any unit velocity
	auto max_inc = ScalField3D(_grid->width(), _grid->height(), _grid->depth());
	auto max_dec = ScalField3D(_grid->width(), _grid->height(), _grid->depth());
	LOOP_3D_GRID(x, y, z, 1) {
		max_inc(x, y, z) = std::sqrt(
			std::max(
				std::pow(std::max(del_plus(x, y, z).x, 0.f), 2),
				std::pow(std::min(del_minus(x, y, z).x, 0.f), 2)
			) +
			std::max(
				std::pow(std::max(del_plus(x, y, z).y, 0.f), 2),
				std::pow(std::min(del_minus(x, y, z).y, 0.f), 2)
			) +
			std::max(
				std::pow(std::max(del_plus(x, y, z).z, 0.f), 2),
				std::pow(std::min(del_minus(x, y, z).z, 0.f), 2)
			)
		);

		max_dec(x, y, z) = std::sqrt(
			std::max(
				std::pow(std::min(del_plus(x, y, z).x, 0.f), 2),
				std::pow(std::max(del_minus(x, y, z).x, 0.f), 2)
			) +
			std::max(
				std::pow(std::min(del_plus(x, y, z).y, 0.f), 2),
				std::pow(std::max(del_minus(x, y, z).y, 0.f), 2)
			) +
			std::max(
				std::pow(std::min(del_plus(x, y, z).z, 0.f), 2),
				std::pow(std::max(del_minus(x, y, z).z, 0.f), 2)
			)
		);
	}

	auto w = ScalField3D(_grid->width(), _grid->height(), _grid->depth());
	LOOP_3D_GRID(x, y, z, 1) {
		float max = -INFINITY;
		max = std::max(max, std::abs(rho(x, y, z) - rho(x - 1, y, z)));
		max = std::max(max, std::abs(rho(x, y, z) - rho(x + 1, y, z)));
		max = std::max(max, std::abs(rho(x, y, z) - rho(x, y - 1, z)));
		max = std::max(max, std::abs(rho(x, y, z) - rho(x, y + 1, z)));
		max = std::max(max, std::abs(rho(x, y, z) - rho(x, y, z - 1)));
		max = std::max(max, std::abs(rho(x, y, z) - rho(x, y, z + 1)));

		w(x, y, z) = std::pow(rho(x, y, z) - 0.5f, 3.f) * (1 - std::min(1.f, max / 0.2f));
	}

	LOOP_3D_GRID(x, y, z, 1) {
		if (w(x, y, z) >= 0)
			del_rho(x, y, z) = w(x, y, z) * max_inc(x, y, z);
		else
			del_rho(x, y, z) = w(x, y, z) * max_dec(x, y, z);

		if (rho(x, y, z) + del_rho(x, y, z) < 0 || rho(x, y, z) < 1e-5)
			del_rho(x, y, z) = -rho(x, y, z);
		else if (rho(x, y, z) > 0.5f)
			del_rho(x, y, z) = 0;
	}

	LOOP_3D_GRID(x, y, z, 1) {
		float dr = del_rho(x, y, z);
		if (std::abs(dr) < 1e-6f) continue;

		rho(x, y, z) += dr;
		rho(x, y, z) = std::max(rho(x, y, z), 0.f);

		const glm::vec3 p = trace_along_field(glm::vec3(x, y, z), PROPERTY_F(RHO_D), 10);
		scatter_values(p, -dr);
	}
}

void CPUSimulator3D::add_force() {
    auto& forces = _grid->forces();
    auto& vel = _grid->vel();
    auto& rho = _grid->rho();
    auto& boundary = _grid->boundary();

    LOOP_3D_GRID(x, y, z, 1) {
        //if (rho(x, y, z) < PROPERTY_F(LIQUID_THRESHOLD))
        //    continue;

        if (boundary(x, y, z) == BOUNDARY)
            continue;

        vel(x, y, z) += forces(x, y, z) * PROPERTY_F(DT);

        if (PROPERTY_B(GRAVITY_ENABLED))
            vel(x, y, z).y -= PROPERTY_F(GRAVITY_FORCE) * PROPERTY_F(DT);
    }
}

void CPUSimulator3D::vel_advect() {
    auto& velField = _grid->vel();
    auto& newVel = _grid->tmpVec();
    auto& boundary = _grid->boundary();

    LOOP_3D_GRID(x, y, z, 1){
        glm::vec3 prevPos = _grid->traceParticle(glm::vec3(x, y, z), -PROPERTY_F(DT), 4);
        newVel(x, y, z) = velField.interpolate(prevPos - glm::vec3(0.5f));
    }

    LOOP_3D_GRID(x, y, z, 1){
        if (x == 1 || boundary(x, y, z) == BOUNDARY || boundary(x-1, y, z) == BOUNDARY)
            velField(x, y, z).x = newVel(x, y, z).x;
        else
            velField(x, y, z).x = 0.5f * (newVel(x, y, z).x + newVel(x - 1, y, z).x);

        if (y == 1 || boundary(x, y, z) == BOUNDARY || boundary(x, y-1, z) == BOUNDARY)
            velField(x, y, z).y = newVel(x, y, z).y;
        else
            velField(x, y, z).y = 0.5f * (newVel(x, y, z).y + newVel(x, y - 1, z).y);

        if (z == 1 || boundary(x, y, z) == BOUNDARY || boundary(x, y, z-1) == BOUNDARY)
            velField(x, y, z).z = newVel(x, y, z).z;
        else
            velField(x, y, z).z = 0.5f * (newVel(x, y, z).z + newVel(x, y, z - 1).z);
    }
}

void CPUSimulator3D::project() {
    auto& vel = _grid->vel();
	auto& rho = _grid->rho();
	auto& boundary = _grid->boundary();

	auto& pressure = _grid->pressure();
    auto& divergence = _grid->divergence();
	pressure.clear();
	divergence.clear();

	enforce_wall_boundaries();

	const float liquidThreshold = PROPERTY_F(LIQUID_THRESHOLD);
	const float lambdaP = PROPERTY_F(LAMBDA_PLUS);
	const float lambdaM = PROPERTY_F(LAMBDA_MINUS);
	const float eta = PROPERTY_F(ETA);
    LOOP_3D_GRID(x, y, z, 1){
    	if (rho(x, y, z) < liquidThreshold)
    		continue;
		if (boundary(x, y, z) == BOUNDARY)
			continue;

		const float dx = vel(x + 1, y, z).x - vel(x, y, z).x;
		const float dy = vel(x, y + 1, z).y - vel(x, y, z).y;
    	const float dz = vel(x, y, z + 1).z - vel(x, y, z).z;
    	divergence(x, y, z) = dx + dy + dz;

		if (rho(x, y, z) > 1.f) {
			divergence(x, y, z) -= std::min(lambdaM * (rho(x, y, z) - 1.f), eta);
		}
		if (rho(x, y, z) < 1.f) {
			divergence(x, y, z) += std::min(lambdaP * (1.f - rho(x, y, z)), eta);
		}
    }

	const float err = _solver->solve(_grid);
    if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
    	//_reset = true;
    	SET_PROPERTY_B(PAUSED, true);
    	return;
    }

    // Calculate the new velocity field as v - grad(p)
    LOOP_3D_GRID(x, y, z, 1){
    	if (boundary(x, y, z) == BOUNDARY)
    		continue;

		auto dPressure = glm::vec3(0);
    	if (x > 1)
    		dPressure.x = pressure(x, y, z) - pressure(x - 1, y, z);
    	if (y > 1)
    		dPressure.y = pressure(x, y, z) - pressure(x, y - 1, z);
    	if (z > 1)
    		dPressure.z = pressure(x, y, z) - pressure(x, y, z - 1);

    	vel(x, y, z) -= dPressure;
    }

	enforce_wall_boundaries();
}

glm::vec3 CPUSimulator3D::trace_along_field(const glm::vec3 start, const float D, const int substeps) const {
	const float maxDist  = D;                  // D * dx, dx = 1 in our grid
	const float stepSize = maxDist / substeps; // size of each Euler step
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	glm::vec3 pos = start;

	for (int s = 0; s < substeps; s++) {
		// Sample rho at current position.
		// If we are already at or past the interface, stop immediately.
		if (rho.interpolate(pos) >= 0.5f)
			break;

		// Compute gradient of rho at current position using central differences
		// at the surrounding integer cell. The gradient points toward
		// higher density — i.e., toward the liquid interior.
		const int ix = std::clamp((int)pos.x, 1, (int)rho.width()  - 2);
		const int iy = std::clamp((int)pos.y, 1, (int)rho.height() - 2);
		const int iz = std::clamp((int)pos.z, 1, (int)rho.depth()  - 2);

		glm::vec3 grad(
			rho(ix+1, iy,   iz)   - rho(ix-1, iy,   iz),   // ∂ρ/∂x (central diff)
			rho(ix,   iy+1, iz)   - rho(ix,   iy-1, iz),   // ∂ρ/∂y (central diff)
			rho(ix,   iy,   iz+1) - rho(ix,   iy  , iz-1)  // ∂ρ/∂z (central diff)
		);

		const float len = glm::length(grad);
		if (len < 1e-6f) break;  // no gradient — rho is flat, nowhere to go

		// Normalise so each step is exactly stepSize cells long
		glm::vec3 dir  = grad / len;
		glm::vec3 next = pos + dir * stepSize;

		// Clamp to valid domain
		next.x = std::clamp(next.x, 1.f, (float)rho.width()  - 2.f);
		next.y = std::clamp(next.y, 1.f, (float)rho.height() - 2.f);
		next.z = std::clamp(next.z, 1.f, (float)rho.depth()  - 2.f);

		if (boundary(next.x, next.y, next.z) > 0.5f)
			break;

		pos = next;
	}

	return pos;
}

void CPUSimulator3D::scatter_values(const glm::vec3 pos, const float value) const {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	const int x0 = std::clamp((int)pos.x,     0, (int)rho.width()  - 1);
	const int y0 = std::clamp((int)pos.y,     0, (int)rho.height() - 1);
	const int z0 = std::clamp((int)pos.z,     0, (int)rho.depth()  - 1);
	const int x1 = std::clamp((int)pos.x + 1, 0, (int)rho.width()  - 1);
	const int y1 = std::clamp((int)pos.y + 1, 0, (int)rho.height() - 1);
	const int z1 = std::clamp((int)pos.z + 1, 0, (int)rho.depth()  - 1);

	const float fx = pos.x - (float)(int)pos.x;
	const float fy = pos.y - (float)(int)pos.y;
	const float fz = pos.z - (float)(int)pos.z;

	float w000 = (1.f-fx) * (1.f-fy) * (1.f-fz);
	float w100 =      fx  * (1.f-fy) * (1.f-fz);
	float w010 = (1.f-fx) *      fy  * (1.f-fz);
	float w110 =      fx  *      fy  * (1.f-fz);
	float w001 = (1.f-fx) * (1.f-fy) *      fz;
	float w101 =      fx  * (1.f-fy) *      fz;
	float w011 = (1.f-fx) *      fy  *      fz;
	float w111 =      fx  *      fy  *      fz;

	if (boundary(x0, y0, z0) > 0.5f) w000 = 0.f;
	if (boundary(x1, y0, z0) > 0.5f) w100 = 0.f;
	if (boundary(x0, y1, z0) > 0.5f) w010 = 0.f;
	if (boundary(x1, y1, z0) > 0.5f) w110 = 0.f;
	if (boundary(x0, y0, z1) > 0.5f) w001 = 0.f;
	if (boundary(x1, y0, z1) > 0.5f) w101 = 0.f;
	if (boundary(x0, y1, z1) > 0.5f) w011 = 0.f;
	if (boundary(x1, y1, z1) > 0.5f) w111 = 0.f;

	const float total = w000 + w100 + w010 + w110 + w001 + w101 + w011 + w111;
	if (total < 1e-6f)
		return;
	const float inv = 1.f / total;

	rho(x0, y0, z0) += value * w000 * inv;
	rho(x1, y0, z0) += value * w100 * inv;
	rho(x0, y1, z0) += value * w010 * inv;
	rho(x1, y1, z0) += value * w110 * inv;
	rho(x0, y0, z1) += value * w001 * inv;
	rho(x1, y0, z1) += value * w101 * inv;
	rho(x0, y1, z1) += value * w011 * inv;
	rho(x1, y1, z1) += value * w111 * inv;
}

void CPUSimulator3D::enforce_wall_boundaries() const {
	auto& vel   = _grid->vel();
	auto& boundary = _grid->boundary();

	LOOP_3D_GRID(x, y, z, 0){
		if (x == 0 || y == 0 || z == 0) continue;

		if (boundary(x, y, z) == 1.f && vel(x, y, z).x > 0)
			vel(x, y, z).x = 0.f;
		if (boundary(x - 1, y, z) == 1.f && vel(x, y, z).x < 0)
			vel(x, y, z).x = 0.f;

		if (boundary(x, y, z) == 1.f && vel(x, y, z).y > 0)
			vel(x, y, z).y = 0.f;
		if (boundary(x, y - 1, z) == 1.f && vel(x, y, z).y < 0)
			vel(x, y, z).y = 0.f;

		if (boundary(x, y, z) == 1.f && vel(x, y, z).z > 0)
			vel(x, y, z).z = 0.f;
		if (boundary(x, y, z - 1) == 1.f && vel(x, y, z).z < 0)
			vel(x, y, z).z = 0.f;
	}
}

#undef BOUNDARY
#undef LOOP_3D_GRID