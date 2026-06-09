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

#define BOUNDARY 1.f

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
	//diffuse();
	rho_sharpen();
	rho_advect();
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
    auto& rho   = _grid->rho();
    auto& gamma = _grid->gamma();
	auto& beta = _grid->beta();
	auto& boundary = _grid->boundary();

	auto idx = [&](int x, int y) { return y * _grid->width() + x; };

    //################## Step 1 ##################
	// Advect gamma using backward semi-lagrangian
	auto advectedGamma = ScalField(gamma, false);
	LOOP_2D_GRID(x, y, 1){
		if (boundary(x, y) == BOUNDARY)
			continue;

		auto prevPos = _grid->traceParticle(glm::vec2(x, y), -Properties::Get()->DT, 4);
		advectedGamma(x, y) = gamma.interpolate(prevPos);
	}
	gamma = advectedGamma;

	//################## Step 2 ##################
	// Compute A
	auto& A = _grid->A();
	A.clear();
	LOOP_2D_GRID(x, y, 1) {
		auto prevPos = _grid->traceParticle(glm::vec2(x, y), -Properties::Get()->DT, 4);

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

		auto nextPos = _grid->traceParticle(glm::vec2(x, y), +Properties::Get()->DT, 4);

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

void Simulator::rho_sharpen() {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	float delT = Properties::Get()->DT * 3;

	auto del_rho = ScalField(_grid->width(), _grid->height());

	// Calculate the mass change due to unit velocity
	auto del_plus = VecField(_grid->width(), _grid->height());
	auto del_minus = VecField(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		del_plus(x, y).x = -(rho(x, y) - rho(x - 1, y)) * delT;
		del_minus(x, y).x = -(rho(x + 1, y) - rho(x, y)) * delT;

		del_plus(x, y).y = -(rho(x, y) - rho(x, y - 1)) * delT;
		del_minus(x, y).y = -(rho(x, y + 1) - rho(x, y)) * delT;
	}

	// Calculate the maximum mass increase and mass decrease due to any unit velocity
	auto max_inc = ScalField(_grid->width(), _grid->height());
	auto max_dec = ScalField(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		max_inc(x, y) = std::sqrt(
			std::max(
				std::powf(std::max(del_plus(x, y).x, 0.f), 2),
				std::powf(std::min(del_minus(x, y).x, 0.f), 2)
			) +
			std::max(
				std::powf(std::max(del_plus(x, y).y, 0.f), 2),
				std::powf(std::min(del_minus(x, y).y, 0.f), 2)
			)
		);

		max_dec(x, y) = std::sqrt(
			std::max(
				std::powf(std::min(del_plus(x, y).x, 0.f), 2),
				std::powf(std::max(del_minus(x, y).x, 0.f), 2)
			) +
			std::max(
				std::powf(std::min(del_plus(x, y).y, 0.f), 2),
				std::powf(std::max(del_minus(x, y).y, 0.f), 2)
			)
		);
	}

	auto w = ScalField(_grid->width(), _grid->height());
	LOOP_2D_GRID(x, y, 1) {
		float max = -INFINITY;
		max = std::max(max, std::abs(rho(x, y) - rho(x - 1, y)));
		max = std::max(max, std::abs(rho(x, y) - rho(x + 1, y)));
		max = std::max(max, std::abs(rho(x, y) - rho(x, y - 1)));
		max = std::max(max, std::abs(rho(x, y) - rho(x, y + 1)));

		w(x, y) = std::powf(rho(x, y) - 0.5f, 3.f) * (1 - std::min(1.f, max / 0.2f));
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
		glm::vec2 p = traceAlongField(glm::vec2(x, y), rho, boundary, 3.f, 10);
		scatterValue(rho, boundary, p, -dr);
	}
}

// Follows the gradient of rho from start toward the 0.5 iso-contour.
// Stops at: the interface (rho >= 0.5), solid wall, or max distance D (in cells).
// substeps: number of forward Euler steps — more = doesn't skip thin interfaces.
glm::vec2 Simulator::traceAlongField(
    glm::vec2        start,
    const ScalField& rho,
    const ScalField& walls,
    float            D,
    int              substeps) const
{
    const float maxDist  = D;                  // D * dx, dx = 1 in our grid
    const float stepSize = maxDist / substeps; // size of each Euler step

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
        int nx = std::clamp((int)next.x, 0, (int)walls.width()  - 1);
        int ny = std::clamp((int)next.y, 0, (int)walls.height() - 1);
        if (walls(nx, ny) > 0.5f) break;

        pos = next;
    }

    return pos;
}

// Deposits 'value' at continuous position 'pos' using bilinear weights.
// Wall cells receive zero weight; remaining weights are renormalised
// so the total deposited equals exactly 'value'.
void Simulator::scatterValue(
    ScalField&       rho,
    const ScalField& walls,
    glm::vec2        pos,
    float            value)
{
    // The 4 surrounding cell-center indices
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
    if (walls(x0, y0) > 0.5f) w00 = 0.f;
    if (walls(x1, y0) > 0.5f) w10 = 0.f;
    if (walls(x0, y1) > 0.5f) w01 = 0.f;
    if (walls(x1, y1) > 0.5f) w11 = 0.f;

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

void Simulator::add_force() const {
	auto& forces = _grid->forces();
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 1) {
		if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
			continue;

		if (boundary(x, y) == BOUNDARY)
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
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 1){
		glm::vec2 prevPos = _grid->traceParticle(glm::vec2(x, y), -Properties::Get()->DT, 4);
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

void Simulator::enforce_wall_boundaries() const {
	auto& vel   = _grid->vel();
	auto& boundary = _grid->boundary();

	LOOP_2D_GRID(x, y, 0){
		if (x == 0 || x == 1 || boundary(x, y) == BOUNDARY || boundary(x-1, y) == BOUNDARY)
			vel(x, y).x = 0.f;

		if (y == 0 || y == 1 || boundary(x, y) == BOUNDARY || boundary(x, y-1) == BOUNDARY)
			vel(x, y).y = 0.f;
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

    LOOP_2D_GRID(x, y, 1){
    	if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
    		continue;
		if (boundary(x, y) == 1)
			continue;

		const float dx = vel(x + 1, y).x - vel(x, y).x;
		const float dy = vel(x, y + 1).y - vel(x, y).y;
    	divergence(x, y) = dx + dy;

		if (rho(x, y) > 1.f) {
			const float lambda = Properties::Get()->LAMBDA;
			const float eta = Properties::Get()->ETA;
			divergence(x, y) -= std::min(lambda * (rho(x, y) - 1.f), eta);
		}
		if (rho(x, y) < 1.f) {
			const float lambda = Properties::Get()->LAMBDA2;
			const float eta = Properties::Get()->ETA;
			divergence(x, y) += std::min(lambda * (1.f - rho(x, y)), eta);
		}
    }

	const float err = ConjugateGradient2(pressure, divergence, rho, boundary, Properties::Get()->MAX_STEPS);
    if (err > Properties::Get()->THRESHOLD) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
    	_reset = true;
    	Properties::Get()->PAUSED = true;
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

	if (Properties::Get()->RE_CALC_DIV) {
		LOOP_2D_GRID(x, y, 1){
			if (rho(x, y) < Properties::Get()->LIQUID_THRESHOLD)
				continue;

			divergence(x, y) = vel(x + 1, y).x - vel(x, y).x
							 + vel(x, y + 1).y - vel(x, y).y;
		}
	}

	enforce_wall_boundaries();
}