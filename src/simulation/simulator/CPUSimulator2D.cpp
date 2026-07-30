#include "simulation/simulator/CPUSimulator2D.h"

#include <algorithm>
#include <queue>
#include <ranges>

#include "simulation/Properties.h"
#include "simulation/solver/CGSolver.h"
#include "simulation/solver/EigenSolver.h"
#include "simulation/solver/JacobiSolver.h"

#include "simulation/display/SimulationDisplay2D.h"
#include "util/Profiler.h"


#define BOUNDARY 1.f

#define LOOP_2D_GRID(x, y, offset)  for (int y = offset; y < _grid->height() - offset; y++) \
									for (int x = offset; x < _grid->width()  - offset; x++)

CPUSimulator2D::CPUSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
    _grid = new Grid2D(width, height);

	const bool validSolver = availableSolvers(std::vector({
		Properties::JACOBI,
		Properties::SELF_CG,
		Properties::EIGEN_CG,
		Properties::EIGEN_BiCGSTAB}));

	if (validSolver) {
		switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
			case Properties::EIGEN_CG:
				_solver = new EigenCGSolver2D(width, height);
				break;
			case Properties::EIGEN_BiCGSTAB:
				_solver = new EigenBiCGSTABSolver2D(width, height);
				break;
			case Properties::JACOBI:
				_solver = new JacobiSolverCPU2D(width, height);
				break;
			case Properties::SELF_CG:
				_solver = new CGSolverCPU2D(width, height);
			default:
				break;
		}
	}

	_data = new SimulationDisplay2D::RenderData2D();
	_data->grid = _grid;
}

CPUSimulator2D::~CPUSimulator2D() {
    delete _grid;
	delete _solver;
}

void CPUSimulator2D::update() {
	ISimulator::update();
}

void* CPUSimulator2D::getRenderData() {
    return _data;
}

void CPUSimulator2D::vel_decay() {
	LOOP_2D_GRID(x, y, 1) {
		if (_grid->rho()(x, y) < 0.5)
			_grid->vel()(x, y) *= PROPERTY_F(VEL_DECAY);
	}
}

void CPUSimulator2D::vel_extrapolate() {
	auto& velField = _grid->vel();
	auto& newVel   = _grid->tmpVec();
	auto& boundary = _grid->boundary();
	auto& rho      = _grid->rho();

	auto validA = ScalField2D(_grid->width(), _grid->height());
	auto validB = ScalField2D(_grid->width(), _grid->height());

	LOOP_2D_GRID(x, y, 1) {
		validA(x, y) = (rho(x, y) >= 0.5f && boundary(x, y) != BOUNDARY) ? 1.f : 0.f;
	}

	auto extrapolate_pass = [&](auto& srcVel, auto& srcValid, auto& dstVel, auto& dstValid) {
		LOOP_2D_GRID(x, y, 1) {
			if (boundary(x, y) == BOUNDARY) {
				dstVel(x, y) = srcVel(x, y);
				dstValid(x, y) = 0.f;
				continue;
			}
			if (srcValid(x, y) > 0.5f) {
				dstVel(x, y) = srcVel(x, y);
				dstValid(x, y) = 1.f;
				continue;
			}

			glm::vec2 sum(0.f);
			float n = 0;
			auto sample = [&](const int nx, const int ny) {
				if (srcValid(nx, ny) > 0.5f) {
					sum += srcVel(nx, ny);
					n++;
				}
			};
			sample(x - 1, y);
			sample(x + 1, y);
			sample(x,     y - 1);
			sample(x,     y + 1);

			dstVel(x, y)   = (n > 0) ? sum / n : srcVel(x, y);
			dstValid(x, y) = (n > 0) ? 1.f : 0.f;
		}
	};

	for (int i = 0; i < 3; i++) {
		extrapolate_pass(velField, validA, newVel,   validB);
		extrapolate_pass(newVel,   validB, velField, validA);
	}
}

void CPUSimulator2D::rho_advect() {
    auto& rho   = _grid->rho();
    auto& gamma = _grid->gamma();
	auto& beta = _grid->beta();
	auto& boundary = _grid->boundary();

	auto idx = [&](int x, int y) { return y * _grid->width() + x; };

    //################## Step 1 ##################
	// Advect gamma using backward semi-lagrangian
	auto advectedGamma = ScalField2D(gamma, false);
	LOOP_2D_GRID(x, y, 1){
		if (boundary(x, y) == BOUNDARY)
			continue;

		auto prevPos = _grid->traceParticle(glm::vec2(x, y), -PROPERTY_F(DT), 4);
		advectedGamma(x, y) = gamma.interpolate(prevPos);
	}
	gamma = advectedGamma;

	//################## Step 2 ##################
	// Compute A
	auto& A = _grid->A();
	A.clear();
	LOOP_2D_GRID(x, y, 1) {
		auto prevPos = _grid->traceParticle(glm::vec2(x, y), -PROPERTY_F(DT), 4);

		int p0x = floor(prevPos.x);
		int p0y = floor(prevPos.y);

		auto dx0 = 1 - (prevPos.x - p0x);
		auto dy0 = 1 - (prevPos.y - p0y);
		auto dx1 = 1 - (p0x + 1 - prevPos.x);
		auto dy1 = 1 - (p0y + 1 - prevPos.y);

		float m0 = 0, m1 = 0, m2 = 0, m3 = 0;
		if (boundary(p0x + 0, p0y + 0) != BOUNDARY) m0 = dx0 * dy0;
		if (boundary(p0x + 1, p0y + 0) != BOUNDARY) m1 = dx1 * dy0;
		if (boundary(p0x + 0, p0y + 1) != BOUNDARY) m2 = dx0 * dy1;
		if (boundary(p0x + 1, p0y + 1) != BOUNDARY) m3 = dx1 * dy1;
		const float sum = m0 + m1 + m2 + m3;
		if (sum == 0)
			continue;

		_grid->addA(idx(x, y), idx(p0x,	    p0y),	  m0 / sum);
		_grid->addA(idx(x, y), idx(p0x + 1, p0y),	  m1 / sum);
		_grid->addA(idx(x, y), idx(p0x,	    p0y + 1), m2 / sum);
		_grid->addA(idx(x, y), idx(p0x + 1, p0y + 1), m3 / sum);
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
	LOOP_2D_GRID(x, y, 1) {
		const int j = idx(x, y);
		if (beta[j] >= 1) continue;

		auto nextPos = _grid->traceParticle(glm::vec2(x, y), +PROPERTY_F(DT), 4);

		int p0x = floor(nextPos.x);
		int p0y = floor(nextPos.y);

		auto dx0 = 1 - (nextPos.x - p0x);
		auto dy0 = 1 - (nextPos.y - p0y);
		auto dx1 = 1 - (p0x + 1 - nextPos.x);
		auto dy1 = 1 - (p0y + 1 - nextPos.y);

		float m0 = 0, m1 = 0, m2 = 0, m3 = 0;
		if (boundary(p0x + 0, p0y + 0) != BOUNDARY) m0 = dx0 * dy0;
		if (boundary(p0x + 1, p0y + 0) != BOUNDARY) m1 = dx1 * dy0;
		if (boundary(p0x + 0, p0y + 1) != BOUNDARY) m2 = dx0 * dy1;
		if (boundary(p0x + 1, p0y + 1) != BOUNDARY) m3 = dx1 * dy1;
		const float sum = m0 + m1 + m2 + m3;
		if (sum == 0)
			continue;

		_grid->addA(idx(p0x,	 p0y),		 j, (1-beta[j]) * (m0 / sum));
		_grid->addA(idx(p0x + 1, p0y),	 j, (1-beta[j]) * (m1 / sum));
		_grid->addA(idx(p0x,	 p0y + 1),   j, (1-beta[j]) * (m2 / sum));
		_grid->addA(idx(p0x + 1, p0y + 1), j, (1-beta[j]) * (m3 / sum));
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
	LOOP_2D_GRID(x, y, 1)
		rho(x, y) = advectRho(x, y);

	//################## Step 12 ##################
	// Diffusion of rho and gamma
	for (int it = 0; it < 7; it++) {
		// X-dimension
		for (int y = 1; y < _grid->height() - 1; y++)
		for (int x = 2; x < _grid->width()  - 1; x++) {
			float g_j = gamma(x-1, y);
			float g_i = gamma(x,   y);

			if (boundary(x, y) == BOUNDARY || boundary(x-1, y) == BOUNDARY)
				continue;

			if (g_j < 1e-5f || g_i < 1e-5f) continue;
			if (g_j == g_i) continue;

			float avg = (g_j + g_i) / 2.f;

			if (g_j > g_i) {
				float dif = rho(x-1, y) * (g_j - g_i) / (2.f * g_j);
				rho(x-1, y) -= dif;
				rho(x,   y) += dif;
			} else {
				float dif = rho(x, y) * (g_i - g_j) / (2.f * g_i);
				rho(x,   y) -= dif;
				rho(x-1, y) += dif;
			}

			gamma(x-1, y) = avg;
			gamma(x,   y) = avg;
		}

		// Y-dimension
		for (int x = 1; x < _grid->width()  - 1; x++)
		for (int y = 2; y < _grid->height() - 1; y++) {
			float g_j = gamma(x, y-1);
			float g_i = gamma(x, y  );

			if (boundary(x, y) == BOUNDARY || boundary(x, y-1) == BOUNDARY)
				continue;

			if (g_j < 1e-5f || g_i < 1e-5f) continue;
			if (g_j == g_i) continue;

			float avg = (g_j + g_i) / 2.f;

			if (g_j > g_i) {
				float dif = rho(x, y-1) * (g_j - g_i) / (2.f * g_j);
				rho(x, y-1) -= dif;
				rho(x, y  ) += dif;
			} else {
				float dif = rho(x, y) * (g_i - g_j) / (2.f * g_i);
				rho(x, y  ) -= dif;
				rho(x, y-1) += dif;
			}

			gamma(x, y-1) = avg;
			gamma(x, y  ) = avg;
		}
	}
}

void CPUSimulator2D::rho_sharpen() {
	auto& rho   = _grid->rho();

	const float delT = PROPERTY_F(DT) * 3;

	auto del_rho = ScalField2D(_grid->width(), _grid->height());

	// Calculate the mass change due to unit velocity
	auto del_plus = VecField2D(_grid->width(), _grid->height());
	auto del_minus = VecField2D(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		del_plus(x, y).x = -(rho(x, y) - rho(x - 1, y)) * delT;
		del_minus(x, y).x = -(rho(x + 1, y) - rho(x, y)) * delT;

		del_plus(x, y).y = -(rho(x, y) - rho(x, y - 1)) * delT;
		del_minus(x, y).y = -(rho(x, y + 1) - rho(x, y)) * delT;
	}

	// Calculate the maximum mass increase and mass decrease due to any unit velocity
	auto max_inc = ScalField2D(_grid->width(), _grid->height());
	auto max_dec = ScalField2D(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		max_inc(x, y) = std::sqrt(
			std::max(
				std::pow(std::max(del_plus(x, y).x, 0.f), 2),
				std::pow(std::min(del_minus(x, y).x, 0.f), 2)
			) +
			std::max(
				std::pow(std::max(del_plus(x, y).y, 0.f), 2),
				std::pow(std::min(del_minus(x, y).y, 0.f), 2)
			)
		);

		max_dec(x, y) = std::sqrt(
			std::max(
				std::pow(std::min(del_plus(x, y).x, 0.f), 2),
				std::pow(std::max(del_minus(x, y).x, 0.f), 2)
			) +
			std::max(
				std::pow(std::min(del_plus(x, y).y, 0.f), 2),
				std::pow(std::max(del_minus(x, y).y, 0.f), 2)
			)
		);
	}

	auto w = ScalField2D(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		float max = -INFINITY;
		max = std::max(max, std::abs(rho(x, y) - rho(x - 1, y)));
		max = std::max(max, std::abs(rho(x, y) - rho(x + 1, y)));
		max = std::max(max, std::abs(rho(x, y) - rho(x, y - 1)));
		max = std::max(max, std::abs(rho(x, y) - rho(x, y + 1)));

		w(x, y) = std::pow(rho(x, y) - 0.5f, 3.f) * (1 - std::min(1.f, max / 0.2f));
	}

	LOOP_2D_GRID(x, y, 1) {
		if (w(x, y) >= 0)
			del_rho(x, y) = w(x, y) * max_inc(x, y);
		else
			del_rho(x, y) = w(x, y) * max_dec(x, y);

		if (rho(x, y) + del_rho(x, y) < 0 || rho(x, y) < 1e-5)
			del_rho(x, y) = -rho(x, y);
		else if (rho(x, y) > 0.5f)
			del_rho(x, y) = 0;
	}

	LOOP_2D_GRID(x, y, 1) {
		float dr = del_rho(x, y);
		if (std::abs(dr) < 1e-6f) continue;

		// Step 1: apply Δρ to this cell (eq. 16)
		rho(x, y) += dr;
		rho(x, y) = std::max(rho(x, y), 0.f);  // prevent negative

		// Step 2: scatter −Δρ toward the 0.5 iso-contour (Algorithm 2)
		// Since dr < 0 for air cells (mass was removed), −dr > 0 is deposited
		// at the interface — mass is conserved locally.
		const glm::vec2 p = trace_along_field(glm::vec2(x, y), PROPERTY_F(RHO_D), 10);
		scatter_values(p, -dr);
	}
}

void CPUSimulator2D::add_force() {
	auto& forces = _grid->forces();
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 1) {
		//if (rho(x, y) < PROPERTY_F(LIQUID_THRESHOLD))
		//	continue;

		if (boundary(x, y) == BOUNDARY)
			continue;

		vel(x, y) += forces(x, y) * PROPERTY_F(DT);

		if (PROPERTY_B(GRAVITY_ENABLED))
			vel(x, y).y -= PROPERTY_F(GRAVITY_FORCE) * PROPERTY_F(DT);
	}
}

void CPUSimulator2D::vel_advect() {
	auto& velField = _grid->vel();
	auto& newVel = _grid->tmpVec();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 1){
		glm::vec2 prevPos = _grid->traceParticle(glm::vec2(x, y), -PROPERTY_F(DT), 4);
		newVel(x, y) = velField.interpolate(prevPos - glm::vec2(0.5f));
	}

	LOOP_2D_GRID(x, y, 1){
		if (x == 1 || boundary(x, y) == BOUNDARY || boundary(x-1, y) == BOUNDARY)
			velField(x, y).x = newVel(x, y).x;
		else
			velField(x, y).x = 0.5f * (newVel(x, y).x + newVel(x - 1, y).x);

		if (y == 1 || boundary(x, y) == BOUNDARY || boundary(x, y-1) == BOUNDARY)
			velField(x, y).y = newVel(x, y).y;
		else
			velField(x, y).y = 0.5f * (newVel(x, y).y + newVel(x, y - 1).y);
	}
}

void CPUSimulator2D::project() {
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
    LOOP_2D_GRID(x, y, 1){
    	if (rho(x, y) < liquidThreshold)
    		continue;
		if (boundary(x, y) == 1)
			continue;

		const float dx = vel(x + 1, y).x - vel(x, y).x;
		const float dy = vel(x, y + 1).y - vel(x, y).y;
    	divergence(x, y) = dx + dy;

		if (rho(x, y) > 1.f) {
			divergence(x, y) -= std::min(lambdaM * (rho(x, y) - 1.f), eta);
		}
		if (rho(x, y) < 1.f) {
			divergence(x, y) += std::min(lambdaP * (1.f - rho(x, y)), eta);
		}
    }

	const float err = _solver->solve(_grid);
    if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
    	SET_PROPERTY_B(PAUSED, true);
    	return;
    }

    // Calculate the new velocity field as v - grad(p)
    LOOP_2D_GRID(x, y, 1){
    	if (boundary(x, y) == 1)
    		continue;

    	auto dPressure = glm::vec2(0);
    	if (x > 1)
    		dPressure.x = pressure(x, y) - pressure(x - 1, y);
    	if (y > 1)
    		dPressure.y = pressure(x, y) - pressure(x, y - 1);

    	vel(x, y) -= dPressure;
    }

	if (PROPERTY_B(RE_CALC_DIV)) {
		LOOP_2D_GRID(x, y, 1){
			if (rho(x, y) < liquidThreshold)
				continue;
			if (boundary(x, y) == 1)
				continue;

			const float dx = vel(x + 1, y).x - vel(x, y).x;
			const float dy = vel(x, y + 1).y - vel(x, y).y;
			divergence(x, y) = dx + dy;

			if (rho(x, y) > 1.f) {
				divergence(x, y) -= std::min(lambdaM * (rho(x, y) - 1.f), eta);
			}
			if (rho(x, y) < 1.f) {
				divergence(x, y) += std::min(lambdaP * (1.f - rho(x, y)), eta);
			}
		}
	}

	enforce_wall_boundaries();
}

glm::vec2 CPUSimulator2D::trace_along_field(const glm::vec2 start, const float D, const int substeps) const {
	const float maxDist  = D;                  // D * dx, dx = 1 in our grid
	const float stepSize = maxDist / substeps; // size of each Euler step
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	glm::vec2 pos = start;

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

		glm::vec2 grad(
			rho(ix+1, iy  ) - rho(ix-1, iy  ),   // ∂ρ/∂x (central diff)
			rho(ix,   iy+1) - rho(ix,   iy-1)    // ∂ρ/∂y (central diff)
		);

		const float len = glm::length(grad);
		if (len < 1e-6f) break;  // no gradient — rho is flat, nowhere to go

		// Normalise so each step is exactly stepSize cells long
		glm::vec2 dir  = grad / len;
		glm::vec2 next = pos + dir * stepSize;

		// Clamp to valid domain
		next.x = std::clamp(next.x, 1.f, (float)rho.width()  - 2.f);
		next.y = std::clamp(next.y, 1.f, (float)rho.height() - 2.f);

		if (boundary(next.x, next.y) > 0.5f)
			break;

		pos = next;
	}

	return pos;
}

void CPUSimulator2D::scatter_values(const glm::vec2 pos, const float value) const {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	const int x0 = std::clamp((int)pos.x,     0, (int)rho.width()  - 1);
	const int y0 = std::clamp((int)pos.y,     0, (int)rho.height() - 1);
	const int x1 = std::clamp((int)pos.x + 1, 0, (int)rho.width()  - 1);
	const int y1 = std::clamp((int)pos.y + 1, 0, (int)rho.height() - 1);

	// Bilinear weights
	const float fx = pos.x - (float)(int)pos.x;
	const float fy = pos.y - (float)(int)pos.y;

	float w00 = (1.f-fx) * (1.f-fy);
	float w10 =      fx  * (1.f-fy);
	float w01 = (1.f-fx) *      fy;
	float w11 =      fx  *      fy;

	// Zero weights for solid cells — mass must not enter walls
	if (boundary(x0, y0) > 0.5f) w00 = 0.f;
	if (boundary(x1, y0) > 0.5f) w10 = 0.f;
	if (boundary(x0, y1) > 0.5f) w01 = 0.f;
	if (boundary(x1, y1) > 0.5f) w11 = 0.f;

	// Renormalise so total weight stays 1 — deposited mass is conserved
	const float total = w00 + w10 + w01 + w11;
	if (total < 1e-6f)
		return;  // surrounded by walls — mass is lost (unavoidable)

	const float inv = 1.f / total;
	rho(x0, y0) += value * w00 * inv;
	rho(x1, y0) += value * w10 * inv;
	rho(x0, y1) += value * w01 * inv;
	rho(x1, y1) += value * w11 * inv;
}

void CPUSimulator2D::enforce_wall_boundaries() const {
	auto& vel   = _grid->vel();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 0){
		if (boundary(x, y) == 1.f && vel(x, y).x > 0)
			vel(x, y).x = 0.f;
		if (boundary(x - 1, y) == 1.f && vel(x, y).x < 0)
			vel(x, y).x = 0.f;

		if (boundary(x, y) == 1.f && vel(x, y).y > 0)
			vel(x, y).y = 0.f;
		if (boundary(x, y - 1) == 1.f && vel(x, y).y < 0)
			vel(x, y).y = 0.f;
	}
}

#undef BOUNDARY
#undef LOOP_2D_GRID