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

void* CPUSimulator2D::getRenderData() {
    return _data;
}

void CPUSimulator2D::vel_extrapolate() {
    const int w = _grid->width();
    const int h = _grid->height();
    auto&       vel      = _grid->vel();
    const auto& rho      = _grid->rho();
    const auto& boundary = _grid->boundary();
    const int N = w * h;

    const int dx[] = {1,-1,0,0};
    const int dy[] = {0,0,1,-1};

    // ── Step 1: label connected liquid components ────────────────────
    std::vector<int> comp(N, -1);   // -1 = not liquid
    int nComp = 0;

    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {
            int i = y*w+x;
            if (rho(x,y) <= 0.5f || boundary(x,y) > 0.5f || comp[i] != -1)
                continue;

            // BFS flood-fill one liquid component
            std::queue<int> q;
            q.push(i);
            comp[i] = nComp;
            while (!q.empty()) {
                int ci = q.front(); q.pop();
                int cx = ci % w, cy = ci / w;
                for (int n = 0; n < 4; n++) {
                    int nx = cx+dx[n], ny = cy+dy[n];
                    if (nx<1||nx>=w-1||ny<1||ny>=h-1) continue;
                    int ni = ny*w+nx;
                    if (comp[ni] != -1) continue;
                    if (rho(nx,ny) <= 0.5f || boundary(nx,ny) > 0.5f) continue;
                    comp[ni] = nComp;
                    q.push(ni);
                }
            }
            nComp++;
        }
    }

    // ── Step 2: BFS extrapolation — one wavefront per component ─────
    // airComp[i] = which component owns this air cell (-1 = empty, -2 = contested)
    std::vector<int>       airComp(N, -1);
    // queue entries: (flat index, owning component)
    std::queue<std::pair<int,int>> frontier;

    // Seed from every liquid cell that touches at least one air neighbour
    for (int y = 1; y < h-1; y++) {
        for (int x = 1; x < w-1; x++) {
            int c = comp[y*w+x];
            if (c == -1) continue;   // not liquid
            for (int n = 0; n < 4; n++) {
                int nx = x+dx[n], ny = y+dy[n];
                if (nx<1||nx>=w-1||ny<1||ny>=h-1) continue;
                int ni = ny*w+nx;
                if (comp[ni] != -1) continue;        // liquid, not air
                if (boundary(nx,ny) > 0.5f) continue;
                if (airComp[ni] == -2) continue;     // already contested
                if (airComp[ni] == c)  continue;     // already owned by us

                if (airComp[ni] == -1) {
                    // First component to reach this cell — tentatively claim it
                    airComp[ni] = c;
                    frontier.push({ni, c});
                } else {
                    // Two different components meet — zero and mark contested
                    vel(nx, ny) = glm::vec2(0.f);
                    airComp[ni] = -2;
                }
            }
        }
    }

    // BFS expand up to PASSES layers
    const int PASSES = 8;
    for (int pass = 0; pass < PASSES && !frontier.empty(); pass++) {
        int sz = frontier.size();
        while (sz-- > 0) {
            auto [ci, c] = frontier.front(); frontier.pop();
            if (airComp[ci] == -2) continue;   // was overwritten — discard

            int cx = ci % w, cy = ci / w;

            // Compute velocity as average of same-component valid neighbours
            glm::vec2 sum(0.f); int count = 0;
            for (int n = 0; n < 4; n++) {
                int nx = cx+dx[n], ny = cy+dy[n];
                if (nx<0||nx>=w||ny<0||ny>=h) continue;
                if (boundary(nx,ny) > 0.5f) continue;
                int ni = ny*w+nx;
                // Accept: liquid cells of same component, or air cells owned by same component
                if (comp[ni] == c || airComp[ni] == c) {
                    sum += vel(nx, ny);
                    count++;
                }
            }
            if (count > 0) vel(cx, cy) = sum / float(count);

            // Expand to unvisited air neighbours
            for (int n = 0; n < 4; n++) {
                int nx = cx+dx[n], ny = cy+dy[n];
                if (nx<1||nx>=w-1||ny<1||ny>=h-1) continue;
                int ni = ny*w+nx;
                if (comp[ni] != -1) continue;        // liquid
                if (boundary(nx,ny) > 0.5f) continue;
                if (airComp[ni] == -2) continue;

                if (airComp[ni] == -1) {
                    airComp[ni] = c;
                    frontier.push({ni, c});
                } else if (airComp[ni] != c) {
                    vel(nx, ny) = glm::vec2(0.f);
                    airComp[ni] = -2;
                }
            }
        }
    }

    // Air cells never reached by any component keep whatever vel_advect
    // left there — optionally zero them:
    for (int i = 0; i < N; i++)
        if (airComp[i] == -1 && comp[i] == -1)
            vel[i] = glm::vec2(0.f);
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
	/*LOOP_2D_GRID(x, y, 1) {
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
	}*/

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
	/*for (int it = 0; it < 7; it++) {
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
	}*/
}

void CPUSimulator2D::rho_sharpen() {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	float delT = PROPERTY_F(DT) * 3;

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
		if (rho(x, y) < PROPERTY_F(LIQUID_THRESHOLD))
			continue;

		if (boundary(x, y) == BOUNDARY)
			continue;

		vel(x, y) += forces(x, y) * PROPERTY_F(DT);

		if (PROPERTY_B(GRAVITY_ENABLED))
			vel(x, y).y -= PROPERTY_F(GRAVITY_FORCE) * PROPERTY_F(DT);

		if (PROPERTY_B(PUSH_ENABLED))
			if (x >= 30 && x < 42 && y > 50 && y < 60)
				vel(x, y) += glm::vec2(0, -PROPERTY_F(PUSH_FORCE));
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

void CPUSimulator2D::diffuse() {
}

void CPUSimulator2D::project() {
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();
	auto& boundary = _grid->boundary();

	auto& pressure = _grid->pressure();
    auto& divergence = _grid->divergence();
	pressure.clear();
	divergence.clear();

	/*LOOP_2D_GRID(x, y, 1) {
		if (y == 1) vel(x, y).y = 0.f;
		if (x == 1) vel(x, y).x = 0.f;

		if (boundary(x, y) == 1)
			vel(x, y) = glm::vec2(0.f);
		if (boundary(x-1, y) == 1)
			vel(x, y).x = 0.f;
		if (boundary(x, y-1) == 1)
			vel(x, y).y = 0.f;
	}*/


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

    	if (x == 1) {
    		if (y == 1) {
                 //field(x, y) = glm::vec2(0, 0);
    		}
    		else {
    			const float dyPressure = pressure(x, y) - pressure(x, y - 1);
    			vel(x, y) = vel(x, y) - glm::vec2(0, dyPressure);
    		}
    	}
    	else {
    		if (y == 1) {
    			const float dxPressure = pressure(x, y) - pressure(x - 1, y);
    			vel(x, y) = vel(x, y) - glm::vec2(dxPressure, 0);
    		}
    		else {
    			const float dxPressure = pressure(x, y) - pressure(x - 1, y);
    			const float dyPressure = pressure(x, y) - pressure(x, y - 1);
    			vel(x, y) = vel(x, y) - glm::vec2(dxPressure, dyPressure);
    		}
    	}
    }

	if (PROPERTY_B(RE_CALC_DIV)) {
		LOOP_2D_GRID(x, y, 1){
			if (rho(x, y) < liquidThreshold)
				continue;

			divergence(x, y) = vel(x + 1, y).x - vel(x, y).x
							 + vel(x, y + 1).y - vel(x, y).y;
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
		float rhoHere = rho.interpolate(pos);
		if (rhoHere >= 0.5f) break;

		// Compute gradient of rho at current position using central differences
		// at the surrounding integer cell. The gradient points toward
		// higher density — i.e., toward the liquid interior.
		int ix = std::clamp((int)pos.x, 1, (int)rho.width()  - 2);
		int iy = std::clamp((int)pos.y, 1, (int)rho.height() - 2);

		glm::vec2 grad(
			rho(ix+1, iy  ) - rho(ix-1, iy  ),   // ∂ρ/∂x (central diff)
			rho(ix,   iy+1) - rho(ix,   iy-1)    // ∂ρ/∂y (central diff)
		);

		float len = glm::length(grad);
		if (len < 1e-6f) break;  // no gradient — rho is flat, nowhere to go

		// Normalise so each step is exactly stepSize cells long
		glm::vec2 dir  = grad / len;
		glm::vec2 next = pos + dir * stepSize;

		// Clamp to valid domain
		next.x = std::clamp(next.x, 1.f, (float)rho.width()  - 2.f);
		next.y = std::clamp(next.y, 1.f, (float)rho.height() - 2.f);

		// Stop if next position is inside a solid
		int nx = std::clamp((int)next.x, 0, (int)boundary.width()  - 1);
		int ny = std::clamp((int)next.y, 0, (int)boundary.height() - 1);
		if (boundary(nx, ny) > 0.5f) break;

		pos = next;
	}

	return pos;
}

void CPUSimulator2D::scatter_values(const glm::vec2 pos, const float value) const {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	int x0 = std::clamp((int)pos.x,     0, (int)rho.width()  - 1);
	int y0 = std::clamp((int)pos.y,     0, (int)rho.height() - 1);
	int x1 = std::clamp((int)pos.x + 1, 0, (int)rho.width()  - 1);
	int y1 = std::clamp((int)pos.y + 1, 0, (int)rho.height() - 1);

	// Bilinear weights
	float fx = pos.x - (float)(int)pos.x;
	float fy = pos.y - (float)(int)pos.y;

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
	float total = w00 + w10 + w01 + w11;
	if (total < 1e-6f) return;  // surrounded by walls — mass is lost (unavoidable)

	float inv = 1.f / total;
	w00 *= inv;  w10 *= inv;
	w01 *= inv;  w11 *= inv;

	// Deposit
	rho(x0, y0) += value * w00;
	rho(x1, y0) += value * w10;
	rho(x0, y1) += value * w01;
	rho(x1, y1) += value * w11;
}

void CPUSimulator2D::enforce_wall_boundaries() const {
	auto& vel   = _grid->vel();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 0){
		if (x == 0 || x == 1 || boundary(x, y) == BOUNDARY || boundary(x-1, y) == BOUNDARY)
			vel(x, y).x = 0.f;

		if (y == 0 || y == 1 || boundary(x, y) == BOUNDARY || boundary(x, y-1) == BOUNDARY)
			vel(x, y).y = 0.f;
	}
}

#undef BOUNDARY
#undef LOOP_2D_GRID