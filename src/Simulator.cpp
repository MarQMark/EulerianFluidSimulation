#include "Simulator.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <views/SimulationView.h>

#include "simulation/Properties.h"
#include "simulation/ScalField.h"
#include "solver/ConjugateGradient.h"
#include "solver/LinearSolver.hpp"
#include "util/Time.h"
#include "views/FieldViewerView.h"
#include "views/FPSView.h"
#include "views/PropertiesView.h"
#include "views/StatsView.h"

Simulator::Simulator() {
	_window = new Window(1280, 720, "StableFluids");
	_views = new Views(_window);
	_views->addView("Simulation", new SimulationView());
	_views->addView("Properties", new PropertiesView());
	_views->addView("Performance", new FPSView());
	_views->addView("Stats", new StatsView());

	recreate();

	Time::Get()->targetFps = 600;
}

Simulator::~Simulator() {
	delete _grid;
	delete _renderer;
	delete _views;
	delete _window;
}

#define LOOP_2D_GRID(x, y, offset)  for (int y = offset; y < _grid->height() - offset; y++) \
									for (int x = offset; x < _grid->width() - offset; x++)

void Simulator::recreate() {
	if (_grid) delete _grid;
	if (_renderer) delete _renderer;

	_renderer = new Renderer(Properties::Get()->WIDTH, Properties::Get()->HEIGHT);
	_grid = new Grid(Properties::Get()->WIDTH, Properties::Get()->HEIGHT);

	if (_views->getView("FieldViewerVel"))
		_views->replaceView("FieldViewerVel", new FieldViewerView(&_grid->vel(), "Velocity Viewer"));
	if (_views->getView("FieldViewerRho"))
		_views->replaceView("FieldViewerRho", new FieldViewerView(&_grid->rho(), "Rho Viewer"));
	if (_views->getView("FieldViewerGamma"))
		_views->replaceView("FieldViewerGamma", new FieldViewerView(&_grid->gamma(), "Gamma Viewer"));
	if (_views->getView("FieldViewerBeta"))
		_views->replaceView("FieldViewerBeta", new FieldViewerView(&_grid->beta(), "Beta Viewer"));
	if (_views->getView("FieldViewerDiv"))
		_views->replaceView("FieldViewerDiv", new FieldViewerView(&_grid->divergence(), "Divergence Viewer"));
	if (_views->getView("FieldViewerPressure"))
		_views->replaceView("FieldViewerPressure", new FieldViewerView(&_grid->pressure(), "Pressure Viewer"));

	_views->getView<SimulationView>("Simulation")->recreate(
		_renderer->getTexture2D()->get(), _grid);
	_views->getView<StatsView>("Stats")->recreate(_grid);

	switch (Properties::Get()->INIT_SHAPE) {
		case Properties::FULL:
			_grid->rho() = 1.f;
			break;
		case Properties::HALF:
			for (int y = 0; y < _grid->height() / 2; y++)
			for (int x = 0; x < _grid->width(); x++)
				_grid->rho()(x, y) = 1.f;
			break;
		case Properties::SQUARE:
			if (_grid->width() < 50 || _grid->height() < 50)
				break;
			for (int y = 40; y < 50; y++)
			for (int x = 40; x < 50; x++)
				_grid->rho()(x, y) = 1.f;
			break;
		case Properties::EMPTY:
			_grid->rho() = 0.f;
			break;
	}

	LOOP_2D_GRID(x, y, 0) {
		if (y == 0) _grid->rho()(x, y) = 0.f;
		if (x == 0) _grid->rho()(x, y) = 0.f;
		if (x == _grid->width() - 1) _grid->rho()(x, y) = 0.f;
		if (x == _grid->height() - 1) _grid->rho()(x, y) = 0.f;
	}

	Properties::Get()->TOTAL_RHO = 0;
	LOOP_2D_GRID(x, y, 0) {
		Properties::Get()->TOTAL_RHO += _grid->rho()(x, y);
	}
}

void Simulator::update() {
    while (!_window->shouldClose()) {
    	Time::Get()->update();

    	if (!Properties::Get()->PAUSED || Properties::Get()->SIMULATION_STEPS > 0) {
			simulate();

    		if (Properties::Get()->SIMULATION_STEPS > 0)
    			Properties::Get()->SIMULATION_STEPS--;
    	}

    	_views->getView<SimulationView>("Simulation")->setVelField(&_grid->vel());

        _renderer->render(_grid);
        _views->render();
        _window->update();

        if(ImGui::IsKeyPressed(ImGuiKey_R))
        	_reset = true;
        if (ImGui::IsKeyPressed(ImGuiKey_P))
            Properties::Get()->PAUSED = !Properties::Get()->PAUSED;
    	if (ImGui::IsKeyPressed(ImGuiKey_S))
    		Properties::Get()->SIMULATION_STEPS++;
    	if (ImGui::IsKeyPressed(ImGuiKey_G))
    		Properties::Get()->GRAVITY = !Properties::Get()->GRAVITY;

    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad1)) {
    		if (!_views->getView("FieldViewerVel"))
    			_views->addView("FieldViewerVel", new FieldViewerView(&_grid->vel(), "Velocity Viewer"));
    		else
    			_views->deleteView("FieldViewerVel");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad2)) {
    		if (!_views->getView("FieldViewerRho"))
    			_views->addView("FieldViewerRho", new FieldViewerView(&_grid->rho(), "Rho Viewer"));
    		else
    			_views->deleteView("FieldViewerRho");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad3)) {
    		if (!_views->getView("FieldViewerGamma"))
    			_views->addView("FieldViewerGamma", new FieldViewerView(&_grid->gamma(), "Gamma Viewer"));
    		else
    			_views->deleteView("FieldViewerGamma");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad4)) {
    		if (!_views->getView("FieldViewerBeta"))
    			_views->addView("FieldViewerBeta", new FieldViewerView(&_grid->beta(), "Beta Viewer"));
    		else
    			_views->deleteView("FieldViewerBeta");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad5)) {
    		if (!_views->getView("FieldViewerDiv"))
    			_views->addView("FieldViewerDiv", new FieldViewerView(&_grid->divergence(), "Divergence Viewer"));
    		else
    			_views->deleteView("FieldViewerDiv");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_Keypad6)) {
    		if (!_views->getView("FieldViewerPressure"))
    			_views->addView("FieldViewerPressure", new FieldViewerView(&_grid->pressure(), "Pressure Viewer"));
    		else
    			_views->deleteView("FieldViewerPressure");
    	}

    	if (_reset) {
    		_reset = false;
        	recreate();
    	}

    	Time::Get()->limitFPS();
    }
}

void Simulator::simulate() {
	//vel_extrapolate();
	rho_advect();
	//diffuse();
	//rho_sharpen();
	vel_advect();
	add_force();
	project();
}



void Simulator::vel_extrapolate() {
	return;
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();

	// A few passes is enough to push velocity a few cells into the air
	const int PASSES = 10;

	for (int pass = 0; pass < PASSES; pass++) {
		auto& tmp = _grid->tmpVec();

		LOOP_2D_GRID(x, y, 1) {
			// Only fill air cells (rho < 0.5) that have at least one liquid neighbor
			if (rho(x, y) >= 0.5f) {
				tmp(x, y) = vel(x, y); // liquid cells stay unchanged
				continue;
			}

			glm::vec2 sum(0.f);
			int count = 0;

			// Average over neighbors that are inside the liquid
			auto sample = [&](int nx, int ny) {
				if (rho(nx, ny) >= 0.5f) { sum += vel(nx, ny); count++; }
			};
			sample(x-1, y); sample(x+1, y);
			sample(x, y-1); sample(x, y+1);

			tmp(x, y) = (count > 0) ? sum / float(count) : vel(x, y);
		}

		LOOP_2D_GRID(x, y, 1) {
			vel(x, y) = tmp(x, y);
		}
	}
}

void Simulator::rho_advect() {
    auto& velField   = _grid->vel();
    auto& rho   = _grid->rho();
    auto& gamma = _grid->gamma();
	auto& beta = _grid->beta();
	auto& boundary = _grid->boundary();

	auto idx = [&](int x, int y) { return y * _grid->width() + x; };

    //################## Step 1 ##################
	// Advect gamma using backward semi-lagrangian
	auto advectedGamma = ScalField(gamma, false);
	LOOP_2D_GRID(x, y, 1){
		auto curPos = glm::vec2(x, y);
		auto velVec = velField.interpolate(curPos - glm::vec2(.5f));
		auto prevPos = curPos - (velVec * Properties::Get()->DT);

		prevPos.x = std::clamp(prevPos.x, 1.f, _grid->width() - 2.f);
		prevPos.y = std::clamp(prevPos.y, 1.f, _grid->height() - 2.f);

		advectedGamma(x, y) = gamma.interpolate(prevPos);
	}
	gamma = advectedGamma;

	//################## Step 2 ##################
	// Compute A
	auto& A = _grid->A();
	A.clear();
	LOOP_2D_GRID(x, y, 1) {
		auto curPos = glm::vec2(x, y);
		auto velVec = velField.interpolate(curPos - glm::vec2(.5f));
		auto prevPos = curPos - (velVec * Properties::Get()->DT);

		prevPos.x = std::clamp(prevPos.x, 1.f, _grid->width() - 2.f);
		prevPos.y = std::clamp(prevPos.y, 1.f, _grid->height() - 2.f);

		int p0x = floor(prevPos.x);
		int p0y = floor(prevPos.y);

		if (boundary(p0x, p0y) == 1)
			continue;

		auto dx0 = 1 - (prevPos.x - p0x);
		auto dy0 = 1 - (prevPos.y - p0y);
		auto dx1 = 1 - (p0x + 1 - prevPos.x);
		auto dy1 = 1 - (p0y + 1 - prevPos.y);

		_grid->addA(idx(x, y), idx(p0x,	    p0y),	  dx0 * dy0);
		_grid->addA(idx(x, y), idx(p0x + 1, p0y),	  dx1 * dy0);
		_grid->addA(idx(x, y), idx(p0x,	    p0y + 1), dx0 * dy1);
		_grid->addA(idx(x, y), idx(p0x + 1, p0y + 1), dx1 * dy1);
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

		auto curPos = glm::vec2(x, y);
		auto velVec = velField.interpolate(curPos - glm::vec2(.5f));
		auto nextPos = curPos + (velVec * Properties::Get()->DT);

		nextPos.x = std::clamp(nextPos.x, 1.f, _grid->width() - 2.f);
		nextPos.y = std::clamp(nextPos.y, 1.f, _grid->height() - 2.f);

		int p0x = floor(nextPos.x);
		int p0y = floor(nextPos.y);

		if (boundary(p0x, p0y) == 1)
			continue;

		auto dx0 = 1 - (nextPos.x - p0x);
		auto dy0 = 1 - (nextPos.y - p0y);
		auto dx1 = 1 - (p0x + 1 - nextPos.x);
		auto dy1 = 1 - (p0y + 1 - nextPos.y);

		_grid->addA(idx(p0x,	 p0y),		 j, (1-beta[j]) * dx0 * dy0);
		_grid->addA(idx(p0x + 1, p0y),	 j, (1-beta[j]) * dx1 * dy0);
		_grid->addA(idx(p0x,	 p0y + 1),   j, (1-beta[j]) * dx0 * dy1);
		_grid->addA(idx(p0x + 1, p0y + 1), j, (1-beta[j]) * dx1 * dy1);
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

void Simulator::rho_sharpen() {
	return;
	auto& rho = _grid->rho();
    int w = _grid->width();
    int h = _grid->height();

    float dx    = 1.0f;
    float dt    = Properties::Get()->DT;
    float dT    = 3.0f * dt;   // fictitious timestep, paper uses 3x sim timestep
    float tau   = 0.4f;        // max density difference threshold (paper's value)
    float eps   = 1e-5f;

    auto deltaRho = ScalField(w, h);

    LOOP_2D_GRID(x, y, 1) {
        float ri = rho(x, y);

        // Upwind differences for +/- x and y directions (Eq. 6-7 in 2D)
        float dxp = -(ri          - rho(x-1, y)) * dx * dT;
        float dxm = -(rho(x+1, y) - ri         ) * dx * dT;
        float dyp = -(ri          - rho(x, y-1)) * dx * dT;
        float dym = -(rho(x, y+1) - ri         ) * dx * dT;

        // Max possible increase and decrease (Eq. 8-13, 2D)
        float gradPlus  = (1.f / (dx*dx)) * sqrtf(
            std::max(std::max(dxp,0.f)*std::max(dxp,0.f), std::min(dxm,0.f)*std::min(dxm,0.f)) +
            std::max(std::max(dyp,0.f)*std::max(dyp,0.f), std::min(dym,0.f)*std::min(dym,0.f))
        );
        float gradMinus = (1.f / (dx*dx)) * sqrtf(
            std::max(std::min(dxp,0.f)*std::min(dxp,0.f), std::max(dxm,0.f)*std::max(dxm,0.f)) +
            std::max(std::min(dyp,0.f)*std::min(dyp,0.f), std::max(dym,0.f)*std::max(dym,0.f))
        );

        // Maximum neighbor difference (for w_i in Eq. 14)
        float maxDiff = std::max({
            std::abs(ri - rho(x+1, y)),
            std::abs(ri - rho(x-1, y)),
            std::abs(ri - rho(x, y+1)),
            std::abs(ri - rho(x, y-1))
        });

        // Sharpening weight (Eq. 14)
        float wi = powf(ri - 0.5f, 3.f) * (1.f - std::min(1.f, maxDiff / tau));

        // Raw correction (Eq. 15)
        float dRho = wi * (wi >= 0.f ? gradPlus : gradMinus);

        // Local mass conservation clamping (Eq. 17):
        // - don't let rho go below zero
        // - don't modify cells that are already inside the liquid (rho > 0.5)
        if (ri + dRho < 0.f || ri < eps)
            dRho = -ri;
        else if (ri > 0.5f)
            dRho = 0.f;

        deltaRho(x, y) = dRho;
    }

    // Apply correction and scatter lost mass back toward the interface
    LOOP_2D_GRID(x, y, 1) {
        rho(x, y) += deltaRho(x, y);

        // Scatter -deltaRho toward the 0.5 iso-contour along the density gradient
        // (Algorithm 2 in the paper — simplified: deposit in the direction of steepest ascent)
        float dr = -deltaRho(x, y);
        if (std::abs(dr) < eps) continue;

        // Gradient of rho at this cell
        glm::vec2 grad(
            rho(x+1, y) - rho(x-1, y),
            rho(x, y+1) - rho(x, y-1)
        );
        float len = glm::length(grad);
        if (len < eps) continue;

        // Step toward the 0.5 iso-contour
        glm::vec2 dir = grad / len;
        glm::vec2 p = glm::vec2(x, y) + dir * 2.f;

        // Clamp and deposit using bilinear scatter
        p.x = std::clamp(p.x, 1.f, (float)_grid->width()  - 2.f);
        p.y = std::clamp(p.y, 1.f, (float)_grid->height() - 2.f);

        int x0 = (int)p.x, y0 = (int)p.y;
        int x1 = x0 + 1,   y1 = y0 + 1;
        float fx = p.x - x0, fy = p.y - y0;

        // Bilinear weights
        rho(x0, y0) += dr * (1-fx) * (1-fy);
        rho(x1, y0) += dr *    fx  * (1-fy);
        rho(x0, y1) += dr * (1-fx) *    fy;
        rho(x1, y1) += dr *    fx  *    fy;
    }

    // Clamp rho to [0, 1] — shouldn't be needed often but prevents runaway values
    LOOP_2D_GRID(x, y, 1) {
        rho(x, y) = std::clamp(rho(x, y), 0.f, 1.f);
    }
}

void Simulator::add_force() const {
	auto& forces = _grid->forces();
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();

	LOOP_2D_GRID(x, y, 1) {
		if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
			continue;

		vel(x, y) += forces(x, y) *  Properties::Get()->DT;

		if(Properties::Get()->GRAVITY)
			vel(x, y).y -= Properties::Get()->GRAVITY_FORCE * Properties::Get()->DT;

		if (Properties::Get()->PUSH)
			if (x >= 30 && x < 42 && y > 50 && y < 60)
				vel(x, y) += glm::vec2(0, -Properties::Get()->PUSH_FORCE);
	}
}

void Simulator::vel_advect() {
	auto& velField = _grid->vel();
	auto& newVel = _grid->tmpVec();

	LOOP_2D_GRID(x, y, 1){
		auto curPos = glm::vec2(x, y) - glm::vec2(.5f);
		auto velVec = velField.interpolate(curPos);

		auto prevPos = curPos - (velVec * Properties::Get()->DT);

		prevPos.x = std::clamp(prevPos.x, 1.f, _grid->width() - 2.f);
		prevPos.y = std::clamp(prevPos.y, 1.f, _grid->height() - 2.f);

		newVel(x, y) = velField.interpolate(prevPos);
	}

	LOOP_2D_GRID(x, y, 1){
		if (x == 1)
			velField(x, y).x = newVel(x, y).x;
		else
			velField(x, y).x = 0.5f * (newVel(x, y).x + newVel(x - 1, y).x);

		if (y == 1)
			velField(x, y).y = newVel(x, y).y;
		else
			velField(x, y).y = 0.5f * (newVel(x, y).y + newVel(x, y - 1).y);
	}
}

void Simulator::diffuse() {
	auto& field = _grid->vel();
	float visc = Properties::Get()->VISCOSITY;
	float dt   = Properties::Get()->DT;

	if (visc == 0.0f) return;

	int w = field.width();
	int h = field.height();
	int n = w * h;

	ImplicitMatrixDiff diffMat(w, h, visc, dt);

	// Solve for each velocity component separately
	// (they decouple — ∇² acts independently on each)
	for (int c = 0; c < 2; c++) {
		ScalField rhs(w, h);
		ScalField sol(w, h);

		LOOP_2D_GRID(x, y, 0) {
			rhs(x, y) = (c == 0) ? field(x, y).x : field(x, y).y;
			sol(x, y) = rhs(x, y);
		}

		int steps = Properties::Get()->MAX_STEPS;
		double err = ConjGrad(n, &diffMat, sol.data(), rhs.data(),
							  Properties::Get()->THRESHOLD, &steps);

		if (err > Properties::Get()->THRESHOLD) {
			fprintf(stderr, "diffuse did not converge! error: %f\n", err);
			_reset = true;
		}

		LOOP_2D_GRID(x, y, 0) {
			if (c == 0) field(x, y).x = sol(x, y);
			else        field(x, y).y = sol(x, y);
		}
	}
}

void Simulator::project() {
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();
	auto& boundary = _grid->boundary();

	auto& pressure = _grid->pressure();
    auto& divergence = _grid->divergence();
	pressure.clear();
	divergence.clear();

	LOOP_2D_GRID(x, y, 1) {
		if (y == 1) vel(x, y).y = 0.f;
		if (x == 1) vel(x, y).x = 0.f;

		if (boundary(x, y) == 1) vel(x, y) = glm::vec2(0.f);
	}

    LOOP_2D_GRID(x, y, 1){
    	if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
    		continue;

		const float dx = vel(x + 1, y).x - vel(x, y).x;
		const float dy = vel(x, y + 1).y - vel(x, y).y;
    	divergence(x, y) = dx + dy;

		if (rho(x, y) > 1.f) {
			const float lambda = Properties::Get()->LAMBDA;
			const float eta = Properties::Get()->ETA;
			divergence(x, y) -= std::min(lambda * (rho(x, y) - 1.f), eta);
		}
		/*if (rho(x, y) < 1.f) {
			const float lambda = Properties::Get()->LAMBDA;
			const float eta = Properties::Get()->ETA;
			divergence(x, y) += std::min(lambda * (1.f - rho(x, y)), eta);
		}*/
    }

	const float err = ConjugateGradient2(pressure, divergence, rho, Properties::Get()->MAX_STEPS);
    if (err > Properties::Get()->THRESHOLD) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
    	_reset = true;
    	Properties::Get()->PAUSED = true;
    }

    // Calculate the new velocity field as v - grad(p)
    LOOP_2D_GRID(x, y, 1){
         // Checks for boundaries
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

	if (Properties::Get()->RE_CALC_DIV) {
		LOOP_2D_GRID(x, y, 1){
			if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
				continue;

			divergence(x, y) = vel(x + 1, y).x - vel(x, y).x
							 + vel(x, y + 1).y - vel(x, y).y;
		}
	}
}
