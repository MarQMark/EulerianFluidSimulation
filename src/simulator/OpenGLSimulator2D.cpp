#include "simulator/OpenGLSimulator2D.h"

#include "simulation/Properties.h"
#include "simulation/SimulationDisplay.h"
#include "solver/CGSolver.h"
#include "solver/JacobiSolver.h"
#include "util/Logger.h"

OpenGLSimulator2D::OpenGLSimulator2D(const unsigned width, const unsigned height) : ISimulator2D(width, height) {
    _vel_pass_1 = new Shader({{ "assets/shaders/vel_advect_pass1.comp", Shader::COMPUTE }});
    _vel_pass_2 = new Shader({{ "assets/shaders/vel_advect_pass2.comp", Shader::COMPUTE }});
    _add_force = new Shader({{ "assets/shaders/add_force.comp", Shader::COMPUTE }});

    _grid = new Grid(width, height);

    switch (PROPERTY(SOLVER_TYPE, Properties::SolverType)) {
        case Properties::EIGEN_CG:
        case Properties::EIGEN_BiCGSTAB:
            LOG_WARN("Invalid Solver for Simulation type");
            break;
        case Properties::JACOBI:
            _solver = new JacobiSolver(width, height);
            break;
        case Properties::SELF_CG:
        default:
            _solver = new CGSolver(width, height);
    }

    _data = new SimulationDisplay2D::RenderData2D();
    _data->grid = _grid;
}

OpenGLSimulator2D::~OpenGLSimulator2D() {
    delete _vel_pass_1;
    delete _vel_pass_2;
    delete _add_force;
}

void* OpenGLSimulator2D::getRenderData() {
    return _data;
}

void OpenGLSimulator2D::vel_extrapolate() {
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
}

void OpenGLSimulator2D::rho_advect() {
}

void OpenGLSimulator2D::rho_sharpen() {
}

void OpenGLSimulator2D::add_force() {
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
}

void OpenGLSimulator2D::vel_advect() {
}

void OpenGLSimulator2D::diffuse() {
}

void OpenGLSimulator2D::project() {
}
