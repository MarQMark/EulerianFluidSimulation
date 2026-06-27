#include "Simulator.h"
#include <algorithm>
#include <ranges>
#include <views/Simulation2DView.h>
#include <views/Simulation3DView.h>

#include "core/Editor.h"
#include "core/InputManager.h"
#include "simulation/Properties.h"
#include "simulation/ScalField.h"
#include "solver/EigenSolver.h"
#include "solver/JacobiSolver.h"
#include "util/Profiler.h"
#include "util/Time.h"
#include "views/DistributionView.h"
#include "views/FieldViewerView.h"
#include "views/FPSView.h"
#include "views/ProfilerView.h"
#include "views/PropertiesView.h"
#include "views/SolverStepsView.h"
#include "views/StatsView.h"
#include "glad/gl.h"
#include "renderer/Renderer3D.h"

Simulator::Simulator() {
	_window = new Window(1280, 720, "StableFluids");
	Input.addInputWindow(_window);

	// --- Setup Simulator ---
	_renderer3D = new Renderer3D;

	_editor = new Editor;

	_scene = new Scene(_window);
	_editor->setScene(_scene);

	// --- Setup UI ---
	_views = new Views(_window);
	//_views->addView("Simulation2D", new Simulation2DView());
	_views->addView("Simulation3D", new Simulation3DView(_editor, _renderer3D));
	_views->addView("Properties", new PropertiesView());
	_views->addView("Performance", new FPSView());
	_views->addView("Stats", new StatsView());
	_views->addView("Distribution", new DistributionView());
	_views->addView("Profiler", new ProfilerView());
	_views->addView("SolverSteps", new SolverStepsView());

	recreate();

	Time::Get()->targetFps = 165;
}

Simulator::~Simulator() {
#ifdef USE_GPU
	delete _vel_pass_1;
	delete _vel_pass_2;
	delete _add_force;
#endif

	delete _grid;
	delete _renderer2D;
	delete _renderer3D;
	delete _views;
	delete _window;
	delete _solver;
}

#define BOUNDARY 1.f

#define LOOP_2D_GRID(x, y, offset)  for (int y = offset; y < _grid->height() - offset; y++) \
									for (int x = offset; x < _grid->width()  - offset; x++)

void Simulator::recreate() {
	if (_grid) delete _grid;
	if (_renderer2D) delete _renderer2D;
	if (_solver) delete _solver;

	const int WIDTH = PROPERTY_I(WIDTH);
	const int HEIGHT = PROPERTY_I(HEIGHT);

	_renderer2D = new Renderer2D(WIDTH, HEIGHT);
	_grid = new Grid(WIDTH, HEIGHT);

	switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
		case Properties::EIGEN_CG:
			_solver = new EigenCGSolver(WIDTH, HEIGHT);
			break;
		case Properties::EIGEN_BiCGSTAB:
			_solver = new EigenBiCGSTABSolver(WIDTH, HEIGHT);
			break;
		case Properties::JACOBI:
			_solver = new JacobiSolver(WIDTH, HEIGHT);
			break;
		case Properties::SELF_CG:
		default:
			_solver = new CGSolver(WIDTH, HEIGHT);
	}

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

	//_views->getView<Simulation2DView>("Simulation2D")->recreate(
	//	_renderer2D->getTexture2D()->get(), _grid);
	_views->getView<StatsView>("Stats")->recreate(_grid);
	_views->getView<DistributionView>("Distribution")->recreate(_grid);

	switch (PROPERTY(INIT_SHAPE, Properties::InitShape)) {
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

	SET_PROPERTY_F(TOTAL_RHO, 0.f);
	LOOP_2D_GRID(x, y, 0) {
		ADD_PROPERTY_F(TOTAL_RHO, _grid->rho()(x, y));
	}

#ifdef USE_GPU
	if (_vel_pass_1) delete _vel_pass_1;
	if (_vel_pass_2) delete _vel_pass_2;
	if (_add_force) delete _add_force;

	_vel_pass_1 = new Shader({{ "assets/shaders/vel_advect_pass1.comp", Shader::COMPUTE }});
	_vel_pass_2 = new Shader({{ "assets/shaders/vel_advect_pass2.comp", Shader::COMPUTE }});
	_add_force = new Shader({{ "assets/shaders/add_force.comp", Shader::COMPUTE }});
#endif
}

void Simulator::update() {
    while (!_window->shouldClose()) {
    	Time::Get()->update();

    	if (!PROPERTY_B(PAUSED) || PROPERTY_I(SIMULATION_STEPS) > 0) {
			simulate();

    		if (PROPERTY_I(SIMULATION_STEPS) > 0)
    			ADD_PROPERTY_I(SIMULATION_STEPS, -1);
    	}

    	_views->getView<Simulation2DView>("Simulation2D")->setVelField(&_grid->vel());

    	_scene->update(Time::Get()->dt);

        _renderer2D->render(_grid);
    	_renderer3D->render(_scene);
        _views->update();
        _window->update();

		const bool searching = _views->getView<PropertiesView>("Properties")->searching();

        if(ImGui::IsKeyPressed(ImGuiKey_R) && !searching)
        	_reset = true;
        if (ImGui::IsKeyPressed(ImGuiKey_P) && !searching)
        	SET_PROPERTY_B(PAUSED, !PROPERTY_B(PAUSED));
    	if (ImGui::IsKeyPressed(ImGuiKey_S) && !searching)
    		ADD_PROPERTY_I(SIMULATION_STEPS, 1);
    	if (ImGui::IsKeyPressed(ImGuiKey_G) && !searching)
    		SET_PROPERTY_B(GRAVITY_ENABLED, !PROPERTY_B(GRAVITY_ENABLED));

    	if (ImGui::IsKeyPressed(ImGuiKey_F1) && !searching) {
    		if (!_views->getView("FieldViewerVel"))
    			_views->addView("FieldViewerVel", new FieldViewerView(&_grid->vel(), "Velocity Viewer"));
    		else
    			_views->deleteView("FieldViewerVel");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_F2) && !searching) {
    		if (!_views->getView("FieldViewerRho"))
    			_views->addView("FieldViewerRho", new FieldViewerView(&_grid->rho(), "Rho Viewer"));
    		else
    			_views->deleteView("FieldViewerRho");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_F3) && !searching) {
    		if (!_views->getView("FieldViewerGamma"))
    			_views->addView("FieldViewerGamma", new FieldViewerView(&_grid->gamma(), "Gamma Viewer"));
    		else
    			_views->deleteView("FieldViewerGamma");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_F4) && !searching) {
    		if (!_views->getView("FieldViewerBeta"))
    			_views->addView("FieldViewerBeta", new FieldViewerView(&_grid->beta(), "Beta Viewer"));
    		else
    			_views->deleteView("FieldViewerBeta");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_F5) && !searching) {
    		if (!_views->getView("FieldViewerDiv"))
    			_views->addView("FieldViewerDiv", new FieldViewerView(&_grid->divergence(), "Divergence Viewer"));
    		else
    			_views->deleteView("FieldViewerDiv");
    	}
    	if (ImGui::IsKeyPressed(ImGuiKey_F6) && !searching) {
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
    	Input.reset();
    }
}

void Simulator::simulate() {
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

void Simulator::vel_extrapolate()
{
	auto& vel = _grid->vel();
	auto& rho = _grid->rho();

	const int w = _grid->width();
	const int h = _grid->height();

	// Cells that already have valid velocities
	ScalField known(w, h);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			if (rho(x, y) > 0.5f)
				known(x, y) = 1;
		}
	}

	constexpr int extrapolationIterations = 15;

	for (int iter = 0; iter < extrapolationIterations; ++iter)
	{
		auto velTmp = vel;
		auto knownTmp = known;

		bool addedCells = false;

		for (int y = 1; y < h - 1; ++y)
		{
			for (int x = 1; x < w - 1; ++x)
			{
				if (known(x, y))
					continue;

				glm::vec2 sum(0.0f);
				int count = 0;

				auto sampleNeighbor = [&](int nx, int ny)
				{
					if (known(nx, ny))
					{
						sum += vel(nx, ny);
						++count;
					}
				};

				sampleNeighbor(x - 1, y);
				sampleNeighbor(x + 1, y);
				sampleNeighbor(x, y - 1);
				sampleNeighbor(x, y + 1);

				if (count > 0)
				{
					velTmp(x, y) = sum / static_cast<float>(count);
					knownTmp(x, y) = 1;
					addedCells = true;
				}
			}
		}

		vel = std::move(velTmp);
		known = std::move(knownTmp);

		if (!addedCells)
			break;
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

void Simulator::rho_sharpen() {
	auto& rho   = _grid->rho();
	auto& boundary = _grid->boundary();

	float delT = PROPERTY_F(DT) * 3;

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

	auto w = ScalField(_grid->width(), _grid->height());
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
		glm::vec2 p = traceAlongField(glm::vec2(x, y), rho, boundary, PROPERTY_F(RHO_D), 10);
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
#ifdef USE_GPU
	const int w     = _grid->width();
	const int h     = _grid->height();
	const int gx = (w - 2 + 15) / 16;
	const int gy = (h - 2 + 15) / 16;

	_add_force->bind();
	_grid->gpuVel     ->bind(0);
	_grid->gpuRho     ->bind(1);
	_grid->gpuForces  ->bind(2);
	_grid->gpuBoundary->bind(3);
	_add_force->setInt  ("uWidth",    w);
	_add_force->setInt  ("uHeight",   h);
	_add_force->setFloat("uDt",       PROPERTY_F(DT));
	_add_force->setFloat("uGravity", PROPERTY_B(GRAVITY_ENABLED) ? PROPERTY_F(GRAVITY_FORCE) : 0.f);
	_add_force->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	_grid->gpuDownload();
#else
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
#endif
}

void Simulator::vel_advect() {

#ifdef USE_GPU
	_grid->gpuUpload();

	const int w     = _grid->width();
	const int h     = _grid->height();
	const int gx = (w - 2 + 15) / 16;
	const int gy = (h - 2 + 15) / 16;

	_vel_pass_1->bind();
	_grid->gpuVel     ->bind(0);
	_grid->gpuTmpVel  ->bind(1);
	_grid->gpuBoundary->bind(2);
	_vel_pass_1->setInt  ("uWidth",    w);
	_vel_pass_1->setInt  ("uHeight",   h);
	_vel_pass_1->setFloat("uDt",       PROPERTY_F(DT));
	_vel_pass_1->setInt  ("uSubsteps", 4);
	_vel_pass_1->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	_vel_pass_2->bind();
	_grid->gpuVel     ->bind(0);   // read-write output
	_grid->gpuTmpVel  ->bind(1);   // read-only input from pass 1
	_grid->gpuBoundary->bind(2);
	_vel_pass_2->setInt("uWidth",  w);
	_vel_pass_2->setInt("uHeight", h);
	_vel_pass_2->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
#else
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
#endif
}

void Simulator::diffuse() {
	auto& field = _grid->vel();
	const float visc = PROPERTY_F(VISCOSITY);
	const float dt   = PROPERTY_F(DT);

	if (visc == 0.0f) return;

	int w = field.width();
	int h = field.height();
	int n = w * h;

	//ImplicitMatrixDiff diffMat(w, h, visc, dt);

	// Solve for each velocity component separately
	// (they decouple — ∇² acts independently on each)
	for (int c = 0; c < 2; c++) {
		ScalField rhs(w, h);
		ScalField sol(w, h);

		LOOP_2D_GRID(x, y, 0) {
			rhs(x, y) = (c == 0) ? field(x, y).x : field(x, y).y;
			sol(x, y) = rhs(x, y);
		}

		int steps = PROPERTY_I(MAX_STEPS);
		/*double err = ConjGrad(n, &diffMat, sol.data(), rhs.data(),
							  PROPERTY_F(SOLVER_THRESHOLD), &steps);

		if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
			fprintf(stderr, "diffuse did not converge! error: %f\n", err);
			_reset = true;
		}*/

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
    	_reset = true;
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
