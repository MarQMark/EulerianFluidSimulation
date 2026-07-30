#include "simulation/simulator/OpenGLSimulator2D.h"

#include <ranges>

#include "simulation/Properties.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/solver/CGSolver.h"
#include "simulation/solver/JacobiSolver.h"
#include "util/Logger.h"
#include "util/Profiler.h"

OpenGLSimulator2D::OpenGLSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
	_vel_decay			= new Shader({{"assets/shaders/openGL_2D/vel_decay.comp"			, Shader::COMPUTE}});
	_vel_extrap_init	= new Shader({{"assets/shaders/openGL_2D/vel_extrap_init.comp"		, Shader::COMPUTE}});
	_vel_extrapolate	= new Shader({{"assets/shaders/openGL_2D/vel_extrapolate.comp"		, Shader::COMPUTE}});
	_rho_sharpen_calc	= new Shader({{"assets/shaders/openGL_2D/rho_sharpen_calc.comp"		, Shader::COMPUTE}});
	_rho_sharpen_apply	= new Shader({{"assets/shaders/openGL_2D/rho_sharpen_apply.comp"	, Shader::COMPUTE}});
	_rho_sharpen_scatter= new Shader({{"assets/shaders/openGL_2D/rho_sharpen_scatter.comp"	, Shader::COMPUTE}});
	_rho_adv_1			= new Shader({{"assets/shaders/openGL_2D/rho_adv_1.comp"			, Shader::COMPUTE}});
	_rho_adv_2_3		= new Shader({{"assets/shaders/openGL_2D/rho_adv_2_3.comp"			, Shader::COMPUTE}});
	_rho_adv_4			= new Shader({{"assets/shaders/openGL_2D/rho_adv_4.comp"			, Shader::COMPUTE}});
	_rho_adv_5			= new Shader({{"assets/shaders/openGL_2D/rho_adv_5.comp"			, Shader::COMPUTE}});
	_rho_adv_6_10_bwd	= new Shader({{"assets/shaders/openGL_2D/rho_adv_6_10_bwd.comp"		, Shader::COMPUTE}});
	_rho_adv_6_10_fwd	= new Shader({{"assets/shaders/openGL_2D/rho_adv_6_10_fwd.comp"		, Shader::COMPUTE}});
	_rho_adv_7_bwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_7_bwd.comp"		, Shader::COMPUTE}});
	_rho_adv_7_fwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_7_fwd.comp"		, Shader::COMPUTE}});
	_rho_adv_8_bwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_8_bwd.comp"		, Shader::COMPUTE}});
	_rho_adv_8_fwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_8_fwd.comp"		, Shader::COMPUTE}});
	_rho_adv_9_bwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_9_bwd.comp"		, Shader::COMPUTE}});
	_rho_adv_9_fwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_9_fwd.comp"		, Shader::COMPUTE}});
	_rho_adv_11_bwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_11_bwd.comp"		, Shader::COMPUTE}});
	_rho_adv_11_fwd		= new Shader({{"assets/shaders/openGL_2D/rho_adv_11_fwd.comp"		, Shader::COMPUTE}});
	_rho_adv_11_cpy		= new Shader({{"assets/shaders/openGL_2D/rho_adv_11_cpy.comp"		, Shader::COMPUTE}});
	_rho_adv_12_x		= new Shader({{"assets/shaders/openGL_2D/rho_adv_12_x.comp"			, Shader::COMPUTE}});
	_rho_adv_12_y		= new Shader({{"assets/shaders/openGL_2D/rho_adv_12_y.comp"			, Shader::COMPUTE}});
	_add_force			= new Shader({{"assets/shaders/openGL_2D/add_force.comp"			, Shader::COMPUTE}});
	_vel_adv_pass_1		= new Shader({{"assets/shaders/openGL_2D/vel_adv_pass_1.comp"		, Shader::COMPUTE}});
	_vel_adv_pass_2		= new Shader({{"assets/shaders/openGL_2D/vel_adv_pass_2.comp"		, Shader::COMPUTE}});
	_proj_calc_div		= new Shader({{"assets/shaders/openGL_2D/proj_calc_div.comp"		, Shader::COMPUTE}});
	_proj_sub_pg		= new Shader({{"assets/shaders/openGL_2D/proj_sub_pg.comp"			, Shader::COMPUTE}});
	_proj_enf_bound		= new Shader({{"assets/shaders/openGL_2D/proj_enf_bound.comp"		, Shader::COMPUTE}});

    _grid = new OpenGLGrid2D(width, height);

    const bool validSolver = availableSolvers(std::vector({
    Properties::JACOBI,
    Properties::SELF_CG}));

    if (validSolver) {
	    switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
	        case Properties::JACOBI:
	            _solver = new JacobiSolverOpenGL2D(width, height);
	            break;
	        case Properties::SELF_CG:
	            _solver = new CGSolverOpenGL2D(width, height);
	        default:
	            break;
	    }
    }

    _data = new SimulationDisplay2D::RenderData2D();
    _data->grid = _grid;
}

OpenGLSimulator2D::~OpenGLSimulator2D() {
	delete _vel_decay;
	delete _vel_extrap_init;
	delete _vel_extrapolate;
	delete _rho_sharpen_calc;
	delete _rho_sharpen_apply;
	delete _rho_sharpen_scatter;
	delete _rho_adv_1;
	delete _rho_adv_2_3;
	delete _rho_adv_4;
	delete _rho_adv_5;
	delete _rho_adv_6_10_bwd;
	delete _rho_adv_6_10_fwd;
	delete _rho_adv_7_bwd;
	delete _rho_adv_7_fwd;
	delete _rho_adv_8_bwd;
	delete _rho_adv_8_fwd;
	delete _rho_adv_9_bwd;
	delete _rho_adv_9_fwd;
	delete _rho_adv_11_bwd;
	delete _rho_adv_11_fwd;
	delete _rho_adv_11_cpy;
	delete _rho_adv_12_x;
	delete _rho_adv_12_y;
	delete _add_force;
	delete _vel_adv_pass_1;
	delete _vel_adv_pass_2;
	delete _proj_calc_div;
	delete _proj_sub_pg;
	delete _proj_enf_bound;

	delete _solver;
	delete _grid;
	delete _data;
}

void* OpenGLSimulator2D::getRenderData() {
    return _data;
}

void OpenGLSimulator2D::update() {
	//_grid->setDirty();
	Profiler* profiler = Profiler::Get();

	PROFILE("Upload",
		if (!PROPERTY_B(DISABLE_UDLOAD)) _grid->upload();
	)

	ISimulator::update();

	PROFILE("Download",
		if (!PROPERTY_B(DISABLE_UDLOAD)) _grid->download();
	)
}

void OpenGLSimulator2D::vel_decay() {
	const unsigned int w = _grid->width();
	const unsigned int h = _grid->height();
	const unsigned int gx = (w - 2 + 15) / 16;
	const unsigned int gy = (h - 2 + 15) / 16;

	_vel_decay->bind();
	_grid->bindAll();
	_vel_decay->setUInt("uWidth", w);
	_vel_decay->setUInt("uHeight", h);
	_vel_decay->setFloat("uDecay", PROPERTY_F(VEL_DECAY));
	_vel_decay->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::vel_extrapolate() {
	const unsigned int w = _grid->width();
	const unsigned int h = _grid->height();
	const unsigned int gx = (w - 2 + 15) / 16;
	const unsigned int gy = (h - 2 + 15) / 16;

	exec_shader(_vel_extrap_init, gx, gy);

	_vel_extrapolate->bind();
	_vel_extrapolate->setUInt("uWidth", w);
	_vel_extrapolate->setUInt("uHeight", h);
	_grid->gpuBoundary().bind(4);
	for (int i = 0; i < 3; i++) {
		// Pass 1
		_grid->gpuVel().bind(0);
		_grid->gpuNewGamma().bind(1);
		_grid->gpuTmpVel().bind(2);
		_grid->gpuBeta().bind(3);
		_vel_extrapolate->dispatch(gx, gy);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Pass 2
		_grid->gpuTmpVel().bind(0);
		_grid->gpuBeta().bind(1);
		_grid->gpuVel().bind(2);
		_grid->gpuNewGamma().bind(3);
		_vel_extrapolate->dispatch(gx, gy);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}


void OpenGLSimulator2D::rho_advect() {
	const unsigned int w = _grid->width();
	const unsigned int h = _grid->height();
	const unsigned int gxCore = (w - 2 + 15) / 16;
	const unsigned int gyCore = (h - 2 + 15) / 16;
	const unsigned int gxFull = (w - 1 + 15) / 16;
	const unsigned int gyFull = (h - 1 + 15) / 16;

	_grid->clearA();

	// (1) Advect gamma using backward semi-lagrangian
	exec_shader_adv(_rho_adv_1, gxCore, gyCore);

	// (2) Compute A + (3) Scale A by gamma
	exec_shader_adv(_rho_adv_2_3, gxCore, gyCore);

	// (4) Compute beta
	exec_shader(_rho_adv_4, gxFull, gyFull);

	// (5) forward trace the velocity field to add the weights to A for all cells j where beta j < 1
	exec_shader_adv(_rho_adv_5, gxCore, gyCore);

	// (6) Compute gamma
	exec_shader(_rho_adv_6_10_bwd, gxFull, gyFull);
	exec_shader(_rho_adv_6_10_fwd, gxFull, gyFull);

	// (7) Scale A by gamma
	exec_shader(_rho_adv_7_bwd, gxFull, gyFull);
	exec_shader(_rho_adv_7_fwd, gxFull, gyFull);

	// (8) Recompute beta from A; !! Order is important - must call fwd before bwd
	exec_shader(_rho_adv_8_fwd, gxFull, gyFull);
	exec_shader(_rho_adv_8_bwd, gxFull, gyFull);

	// (9) Clamp beta to 1
	exec_shader(_rho_adv_9_bwd, gxFull, gyFull);
	exec_shader(_rho_adv_9_fwd, gxFull, gyFull);

	// (6) Compute gamma
	exec_shader(_rho_adv_6_10_bwd, gxFull, gyFull);
	exec_shader(_rho_adv_6_10_fwd, gxFull, gyFull);

	// (11) Evaluate p_n+1 = A * p
	exec_shader(_rho_adv_11_bwd, gxFull, gyFull);
	exec_shader(_rho_adv_11_fwd, gxFull, gyFull);
	exec_shader(_rho_adv_11_cpy, gxFull, gyFull);

	//################## Step 12 ##################
	// (12) Diffusion of rho and gamma
	{
		const unsigned int x_working_width = _grid->width() - 2;
		const unsigned int gx_x = ( (x_working_width / 2) + 15) / 16;
		const unsigned int gx_y = ( (_grid->height() - 2) + 15) / 16;
		const unsigned int y_working_height = _grid->height() - 2;
		const unsigned int gy_x = ( (_grid->width() - 2)   + 15) / 16;
		const unsigned int gy_y = ( (y_working_height / 2) + 15) / 16;

		for (int it = 0; it < 7; it++) {
			exec_shader_off(_rho_adv_12_x, gx_x, gx_y, 0);
			exec_shader_off(_rho_adv_12_x, gx_x, gx_y, 1);

			exec_shader_off(_rho_adv_12_y, gy_x, gy_y, 0);
			exec_shader_off(_rho_adv_12_y, gy_x, gy_y, 1);
		}
	}
}

void OpenGLSimulator2D::rho_sharpen() {
	const unsigned int w = _grid->width();
	const unsigned int h = _grid->height();
	const unsigned int gx = (w - 2 + 15) / 16;
	const unsigned int gy = (h - 2 + 15) / 16;

	_rho_sharpen_calc->bind();
	_grid->bindAll();
	_rho_sharpen_calc->setUInt("uWidth", w);
	_rho_sharpen_calc->setUInt("uHeight", h);
	_rho_sharpen_calc->setFloat("uDt", PROPERTY_F(DT) * 3.f);
	_rho_sharpen_calc->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	exec_shader(_rho_sharpen_apply, gx, gy);

	_rho_sharpen_scatter->bind();
	_grid->bindAll();
	_rho_sharpen_scatter->setUInt("uWidth", w);
	_rho_sharpen_scatter->setUInt("uHeight", h);
	_rho_sharpen_scatter->setFloat("uRhoD", PROPERTY_F(RHO_D));
	_rho_sharpen_scatter->setUInt("uSubsteps", 10);
	_rho_sharpen_scatter->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::add_force() {
    const unsigned int w = _grid->width();
    const unsigned int h = _grid->height();
    const unsigned int gx = (w - 2 + 15) / 16;
    const unsigned int gy = (h - 2 + 15) / 16;

    _add_force->bind();
    _grid->bindAll();
    _add_force->setUInt("uWidth", w);
    _add_force->setUInt("uHeight", h);
    _add_force->setFloat("uDt", PROPERTY_F(DT));
    _add_force->setFloat("uGravity", PROPERTY_B(GRAVITY_ENABLED) ? PROPERTY_F(GRAVITY_FORCE) : 0.f);
    _add_force->dispatch(gx, gy);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::vel_advect() {
    const unsigned int w = _grid->width();
    const unsigned int h = _grid->height();
    const unsigned int gx = (w - 2 + 15) / 16;
    const unsigned int gy = (h - 2 + 15) / 16;

	exec_shader_adv(_vel_adv_pass_1, gx, gy);
	exec_shader(_vel_adv_pass_2, gx, gy);
}

void OpenGLSimulator2D::project() {
    const unsigned int w = _grid->width();
    const unsigned int h = _grid->height();
    const unsigned int gx = (w - 2 + 15) / 16;
    const unsigned int gy = (h - 2 + 15) / 16;

	_grid->gpuDivergence().clear();
	_grid->gpuPressure().clear();

	exec_shader(_proj_enf_bound, gx, gy);

    _proj_calc_div->bind();
    _grid->bindAll();
    _proj_calc_div->setUInt("uWidth", w);
    _proj_calc_div->setUInt("uHeight", h);
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

	exec_shader(_proj_sub_pg, gx, gy);

	exec_shader(_proj_enf_bound, gx, gy);
}

void OpenGLSimulator2D::exec_shader(const Shader *shader, const unsigned int gx, const unsigned int gy) const {
	shader->bind();
	_grid->bindAll();
	shader->setUInt("uWidth", _grid->width());
	shader->setUInt("uHeight", _grid->height());
	shader->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::exec_shader_adv(const Shader *shader, const unsigned int gx, const unsigned int gy) const {
	shader->bind();
	_grid->bindAll();
	shader->setUInt("uWidth", _grid->width());
	shader->setUInt("uHeight", _grid->height());
	shader->setFloat("uDt", PROPERTY_F(DT));
	shader->setInt("uSubsteps", 4);
	shader->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLSimulator2D::exec_shader_off(const Shader *shader, const unsigned int gx, const unsigned int gy, const unsigned int offset) const {
	shader->bind();
	_grid->bindAll();
	shader->setUInt("uWidth", _grid->width());
	shader->setUInt("uHeight", _grid->height());
	shader->setUInt("uOffset", offset);
	shader->dispatch(gx, gy);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
