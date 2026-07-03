#include "simulator/CPUSimulator3D.h"

#include <ranges>

#include "simulation/Grid3D.h"
#include "simulation/Properties.h"
#include "solver/CGSolver.h"
#include "util/Profiler.h"

#define BOUNDARY 1.f
#define LOOP_3D_GRID(x, y, z, offset)   for (int z = offset; z < _grid->depth()  - offset; z++) \
                                        for (int y = offset; y < _grid->height() - offset; y++) \
                                        for (int x = offset; x < _grid->width()  - offset; x++)

CPUSimulator3D::CPUSimulator3D(const unsigned int width, const unsigned int height, const  unsigned int depth)
	: ISimulator3D(width, height, depth) {
    _data = new SimulationDisplay3D::RenderData3D;
}

CPUSimulator3D::~CPUSimulator3D() {
    delete _grid;
	delete _solver;
}

void CPUSimulator3D::update() {
    // Setup grid if not initialized
	// TODO - Move this function into layer above? Recreate whole Simulator object on uninitialied
    if (!PROPERTY_B(SIMULATION_INITIALIZED)) {
    	_width = PROPERTY_I(WIDTH_3D);
    	_height = PROPERTY_I(HEIGHT_3D);
    	_depth = PROPERTY_I(DEPTH_3D);

        delete _grid;
        _grid = new Grid3D(_width, _height, _depth);
        _data->grid = _grid;

    	const bool validSolver = availableSolvers(std::vector({
			//Properties::JACOBI,
			Properties::SELF_CG}));

    	delete _solver;

    	if (validSolver)
    		switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
    			//case Properties::JACOBI:
    			//		_solver = new JacobiSolverCPU3D(width, height, depth);
    			//		break;
    			case Properties::SELF_CG:
    					_solver = new CGSolverCPU3D(_width, _height, _depth);
    			default:
    				break;
    	}

        return;
    }

	Profiler* profiler = Profiler::Get();
	profiler->start("1 Vel Extrapolate");
	if (!PROPERTY_B(DISABLE_VEL_EXTRAPOLATE)) vel_extrapolate();
	profiler->stop();

	//diffuse();

	profiler->start("2 Rho Sharpen");
	if (!PROPERTY_B(DISABLE_RHO_SHARPEN)) rho_sharpen();
	profiler->stop();
	profiler->start("3 Rho Advect");
	if (!PROPERTY_B(DISABLE_RHO_ADVECT))  rho_advect();
	profiler->stop();
	profiler->start("4 Vel Advect");
	if (!PROPERTY_B(DISABLE_VEL_ADVECT))  vel_advect();
	profiler->stop();
	profiler->start("5 Add Force");
	if (!PROPERTY_B(DISABLE_ADD_FORCE))   add_force();
	profiler->stop();
	profiler->start("6 Project");
	if (!PROPERTY_B(DISABLE_PROJECT))     project();
	profiler->stop();
}

void* CPUSimulator3D::getRenderData() {
    return _data;
}

void CPUSimulator3D::vel_extrapolate() {
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
	for (int it = 0; it < 7; it++) {
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

		// Y-dimension
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
}

void CPUSimulator3D::add_force() {
    auto& forces = _grid->forces();
    auto& vel = _grid->vel();
    auto& rho = _grid->rho();
    auto& boundary = _grid->boundary();

    LOOP_3D_GRID(x, y, z, 1) {
        if (rho(x, y, z) < PROPERTY_F(LIQUID_THRESHOLD))
            continue;

        if (boundary(x, y, z) == BOUNDARY)
            continue;

        vel(x, y, z) += forces(x, y, z) * PROPERTY_F(DT);

        if (PROPERTY_B(GRAVITY_ENABLED))
            vel(x, y, z).y -= PROPERTY_F(GRAVITY_FORCE) * PROPERTY_F(DT);

        if (PROPERTY_B(PUSH_ENABLED))
            if (x >= 30 && x < 42 && y > 50 && y < 60)
                vel(x, y, z) += glm::vec3(0, -PROPERTY_F(PUSH_FORCE), 0);
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

void CPUSimulator3D::diffuse() {
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
    	if (x != 0)
    		dPressure.x = pressure(x, y, z) - pressure(x - 1, y, z);
    	if (y != 0)
    		dPressure.y = pressure(x, y, z) - pressure(x, y - 1, z);
    	if (z != 0)
    		dPressure.z = pressure(x, y, z) - pressure(x, y, z - 1);

    	vel(x, y, z) -= dPressure;
    }

	enforce_wall_boundaries();
}

void CPUSimulator3D::enforce_wall_boundaries() const {
	auto& vel   = _grid->vel();
	auto& boundary = _grid->boundary();

	LOOP_3D_GRID(x, y, z, 0){
		if (x == 0 || x == 1 || boundary(x, y, z) == BOUNDARY || boundary(x-1, y, z) == BOUNDARY)
			vel(x, y, z).x = 0.f;

		if (y == 0 || y == 1 || boundary(x, y, z) == BOUNDARY || boundary(x, y-1, z) == BOUNDARY)
			vel(x, y, z).y = 0.f;

		if (z == 0 || z == 1 || boundary(x, y, z) == BOUNDARY || boundary(x, y, z-1) == BOUNDARY)
			vel(x, y, z).z = 0.f;
	}
}

#undef BOUNDARY
#undef LOOP_3D_GRID