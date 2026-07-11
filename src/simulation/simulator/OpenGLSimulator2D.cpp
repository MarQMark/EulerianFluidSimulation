#include "simulation/simulator/OpenGLSimulator2D.h"

#include <ranges>

#include "simulation/Properties.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/solver/CGSolver.h"
#include "simulation/solver/JacobiSolver.h"
#include "util/Logger.h"

OpenGLSimulator2D::OpenGLSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
	_rv_1_adv_gamma = new Shader({{ "assets/shaders/openGL_2D/rv_1_adv_gamma.comp", Shader::COMPUTE }});
	_rv_2_comp_a = new Shader({{ "assets/shaders/openGL_2D/rv_2_comp_a.comp", Shader::COMPUTE }});
	_rv_4_comp_beta = new Shader({{ "assets/shaders/openGL_2D/rv_4_comp_beta.comp", Shader::COMPUTE }});
	_rv_5_fwd_trace = new Shader({{ "assets/shaders/openGL_2D/rv_5_fwd_trace.comp", Shader::COMPUTE }});
	_rv_6_comp_gamma = new Shader({{ "assets/shaders/openGL_2D/rv_6_comp_gamma.comp", Shader::COMPUTE }});
	_rv_7_scale_a = new Shader({{ "assets/shaders/openGL_2D/rv_7_scale_a.comp", Shader::COMPUTE }});
	_rv_9_clamp_beta = new Shader({{ "assets/shaders/openGL_2D/rv_9_clamp_beta.comp", Shader::COMPUTE }});
	_rv_11_Ap = new Shader({{ "assets/shaders/openGL_2D/rv_11_Ap.comp", Shader::COMPUTE }});

	_add_force = new Shader({{ "assets/shaders/openGL_2D/add_force.comp", Shader::COMPUTE }});

	_vel_pass_1 = new Shader({{ "assets/shaders/openGL_2D/vel_advect_pass1.comp", Shader::COMPUTE }});
    _vel_pass_2 = new Shader({{ "assets/shaders/openGL_2D/vel_advect_pass2.comp", Shader::COMPUTE }});

	_proj_calc_div = new Shader({{ "assets/shaders/openGL_2D/proj_calc_div.comp", Shader::COMPUTE }});
    _proj_sub_pg = new Shader({{ "assets/shaders/openGL_2D/proj_sub_pg.comp", Shader::COMPUTE }});

    _grid = new OpenGLGrid2D(width, height);

    const bool validSolver = availableSolvers(std::vector({
    Properties::JACOBI,
    Properties::SELF_CG}));

    if (validSolver)
    switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
        case Properties::JACOBI:
            _solver = new JacobiSolverOpenGL2D(width, height);
            break;
        case Properties::SELF_CG:
            _solver = new CGSolverOpenGL2D(width, height);
        default:
            break;
    }

    _data = new SimulationDisplay2D::RenderData2D();
    _data->grid = _grid;
}

OpenGLSimulator2D::~OpenGLSimulator2D() {
	delete _rv_1_adv_gamma;
	delete _rv_2_comp_a;
	delete _rv_4_comp_beta;
	delete _rv_5_fwd_trace;
	delete _rv_6_comp_gamma;
	delete _rv_7_scale_a;
	delete _rv_9_clamp_beta;
	delete _rv_11_Ap;
    delete _add_force;
    delete _vel_pass_1;
    delete _vel_pass_2;
    delete _proj_calc_div;
    delete _proj_sub_pg;
}

void* OpenGLSimulator2D::getRenderData() {
    return _data;
}

void OpenGLSimulator2D::update() {
	_grid->setDirty();
    _grid->upload();

	rho_advect();
	vel_advect();
	add_force();
	project();

    _grid->download();
}

void OpenGLSimulator2D::vel_extrapolate() {
}

#define BOUNDARY 1.f

#define LOOP_2D_GRID(x, y, offset)  for (int (y) = offset; (y) < _grid->height() - (offset); (y)++) \
									for (int (x) = offset; (x) < _grid->width()  - (offset); (x)++)

void OpenGLSimulator2D::rho_advect() {
	const int w = _grid->width();
	const int h = _grid->height();
	const int gx = (w - 1 + 15) / 16;
	const int gy = (h - 1 + 15) / 16;

	//################## Step 1 ##################
	// Advect gamma using backward semi-lagrangian
	_rv_1_adv_gamma->bind();
	_grid->gpuNewGamma().bind(0);
	_grid->gpuGamma().bind(1);
	_grid->gpuVel().bind(2);
	_grid->gpuBoundary().bind(3);
	_rv_1_adv_gamma->setInt("uWidth", w);
	_rv_1_adv_gamma->setInt("uHeight", h);
	_rv_1_adv_gamma->setFloat("uDt", PROPERTY_F(DT));
	_rv_1_adv_gamma->setInt("uSubsteps", 4);
	_rv_1_adv_gamma->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 2 ##################
	// Compute A
	//################## Step 3 ##################
	// Scale A by gamma
	_grid->gpuA().clear();
	_grid->gpuCounts().clear();
	_rv_2_comp_a->bind();
	_grid->gpuA().bind(0);
	_grid->gpuCounts().bind(1);
	_grid->gpuNewGamma().bind(2);
	_grid->gpuVel().bind(3);
	_grid->gpuBoundary().bind(4);
	_rv_2_comp_a->setInt("uWidth", w);
	_rv_2_comp_a->setInt("uHeight", h);
	_rv_2_comp_a->setFloat("uDt", PROPERTY_F(DT));
	_rv_2_comp_a->setInt("uSubsteps", 4);
	_rv_2_comp_a->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 4 ##################
	// Compute beta
	_grid->gpuBeta().clear();
	_rv_4_comp_beta->bind();
	_grid->gpuBeta().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_4_comp_beta->setInt("uWidth", w);
	_rv_4_comp_beta->setInt("uHeight", h);
	_rv_4_comp_beta->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 5 ##################
	// Forward trace the velocity field to add the weights to A for all cells j where beta j < 1
	// TODO

	//################## Step 6 ##################
	// Recompute gamma
	_grid->gpuGamma().clear();
	_rv_6_comp_gamma->bind();
	_grid->gpuGamma().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_6_comp_gamma->setInt("uWidth", w);
	_rv_6_comp_gamma->setInt("uHeight", h);
	_rv_6_comp_gamma->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 7 ##################
	// Scale A by gamma
	_rv_7_scale_a->bind();
	_grid->gpuGamma().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_7_scale_a->setInt("uWidth", w);
	_rv_7_scale_a->setInt("uHeight", h);
	_rv_7_scale_a->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 8 ##################
	// Recompute beta from A
	_grid->gpuBeta().clear();
	_rv_4_comp_beta->bind();
	_grid->gpuBeta().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_4_comp_beta->setInt("uWidth", w);
	_rv_4_comp_beta->setInt("uHeight", h);
	_rv_4_comp_beta->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 9 ##################
	// Clamp beta to 1
	_rv_9_clamp_beta->bind();
	_grid->gpuBeta().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_9_clamp_beta->setInt("uWidth", w);
	_rv_9_clamp_beta->setInt("uHeight", h);
	_rv_9_clamp_beta->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	/*_grid->gpuBeta().clear();
	_rv_4_comp_beta->bind();
	_grid->gpuBeta().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_4_comp_beta->setInt("uWidth", w);
	_rv_4_comp_beta->setInt("uHeight", h);
	_rv_4_comp_beta->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);*/

	//################## Step 10 ##################
	// Recompute gamma
	_grid->gpuGamma().clear();
	_rv_6_comp_gamma->bind();
	_grid->gpuGamma().bind(0);
	_grid->gpuA().bind(1);
	_grid->gpuCounts().bind(2);
	_rv_6_comp_gamma->setInt("uWidth", w);
	_rv_6_comp_gamma->setInt("uHeight", h);
	_rv_6_comp_gamma->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//################## Step 11 ##################
	// Evaluate p_n+1 = A * p
	_rv_11_Ap->bind();
	_grid->gpuTmpRho().bind(0);
	_grid->gpuRho().bind(1);
	_grid->gpuA().bind(2);
	_grid->gpuCounts().bind(3);
	_rv_11_Ap->setInt("uWidth", w);
	_rv_11_Ap->setInt("uHeight", h);
	_rv_11_Ap->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	_grid->gpuTmpRho().download(_grid->rho().data());
	_grid->gpuRho().upload(_grid->rho().data());

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

void OpenGLSimulator2D::rho_sharpen() {
}

void OpenGLSimulator2D::add_force() {
    const int w = _grid->width();
    const int h = _grid->height();
    const int gx = (w - 2 + 15) / 16;
    const int gy = (h - 2 + 15) / 16;

    _add_force->bind();
    _grid->gpuVel().bind(0);
    _grid->gpuRho().bind(1);
    _grid->gpuForces().bind(2);
    _grid->gpuBoundary().bind(3);
    _add_force->setInt("uWidth", w);
    _add_force->setInt("uHeight", h);
    _add_force->setFloat("uDt", PROPERTY_F(DT));
    _add_force->setFloat("uGravity", PROPERTY_B(GRAVITY_ENABLED) ? PROPERTY_F(GRAVITY_FORCE) : 0.f);
    _add_force->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::vel_advect() {
    const int w = _grid->width();
    const int h = _grid->height();
    const int gx = (w - 2 + 15) / 16;
    const int gy = (h - 2 + 15) / 16;

    _vel_pass_1->bind();
    _grid->gpuVel().bind(0);
    _grid->gpuTmpVel().bind(1);
    _grid->gpuBoundary().bind(2);
    _vel_pass_1->setInt("uWidth", w);
    _vel_pass_1->setInt("uHeight", h);
    _vel_pass_1->setFloat("uDt", PROPERTY_F(DT));
    _vel_pass_1->setInt("uSubsteps", 4);
    _vel_pass_1->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    _vel_pass_2->bind();
    _grid->gpuVel().bind(0);
    _grid->gpuTmpVel().bind(1);
    _grid->gpuBoundary().bind(2);
    _vel_pass_2->setInt("uWidth", w);
    _vel_pass_2->setInt("uHeight", h);
    _vel_pass_2->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::diffuse() {
}

void OpenGLSimulator2D::project() {
    const int w = _grid->width();
    const int h = _grid->height();
    const int gx = (w - 2 + 15) / 16;
    const int gy = (h - 2 + 15) / 16;

    _proj_calc_div->bind();
    _grid->gpuDivergence().bind(0);
    _grid->gpuVel().bind(1);
    _grid->gpuRho().bind(2);
    _grid->gpuBoundary().bind(3);
    _proj_calc_div->setInt("uWidth", w);
    _proj_calc_div->setInt("uHeight", h);
    _proj_calc_div->setFloat("uLambdaP", PROPERTY_F(LAMBDA_PLUS));
    _proj_calc_div->setFloat("uLambdaM", PROPERTY_F(LAMBDA_MINUS));
    _proj_calc_div->setFloat("uEta", PROPERTY_F(ETA));
    _proj_calc_div->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    const float err = _solver->solve(_grid);
    if (err > PROPERTY_F(SOLVER_THRESHOLD)) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
        SET_PROPERTY_B(PAUSED, true);
        return;
    }

    _proj_sub_pg->bind();
    _grid->gpuVel().bind(0);
    _grid->gpuPressure().bind(1);
    _grid->gpuBoundary().bind(2);
    _proj_sub_pg->setInt("uWidth", w);
    _proj_sub_pg->setInt("uHeight", h);
    _proj_sub_pg->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
