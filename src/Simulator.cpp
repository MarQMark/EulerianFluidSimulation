#include "Simulator.h"

#include <algorithm>
#include <iostream>
#include <views/SimulationView.h>

#include "simulation/Properties.h"
#include "simulation/ScalField.h"
#include "solver/ConjugateGradient.h"
#include "solver/LinearSolver.hpp"
#include "views/PropertiesView.h"


Simulator::Simulator() {
	_window = new Window(1280, 720, "StableFluids");
	_views = new Views(_window);
	create();
	_views->addView("Simulation",
		new SimulationView(
			_renderer->getTexture2D()->get(),
			Properties::Get()->WIDTH,
			Properties::Get()->HEIGHT));
	_views->addView("Properties", new PropertiesView());
}

Simulator::~Simulator() {
	destroy();
	delete _views;
	delete _window;
}

void Simulator::create() {
	_renderer = new Renderer(Properties::Get()->WIDTH, Properties::Get()->HEIGHT);
	_grid = new Grid(Properties::Get()->WIDTH, Properties::Get()->HEIGHT);
}

void Simulator::destroy() const {
	delete _grid;
	delete _renderer;
}

void Simulator::recreate() {
	destroy();
	create();
	dynamic_cast<SimulationView*>(_views->getView("Simulation"))->recreate(
		_renderer->getTexture2D()->get(),
		Properties::Get()->WIDTH,
		Properties::Get()->HEIGHT);

	for (int y = 40; y < 50; y++)
		for (int x = 40; x < 50; x++)
			_grid->getPres()(x, y) = 1;

	_grid->getPres() = 1.f;
	//_grid->getPres().set(1.f);
}

void Simulator::update() {
    while (!_window->shouldClose()) {
        simulate();
        _views->render();
        _renderer->render(
        	_grid,
        	dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getBoundary()
        	);
        _window->update();

        if(ImGui::IsKeyPressed(ImGuiKey_R)) {
        	recreate();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_P))
            _paused = !_paused;
    }
}


void Simulator::simulate() {
    if (_paused)
        return;

    advect(_grid->getNewVel(), _grid->getVel());
    //_grid->swapVel();
	//VorticityConfinement();

    //projection(_grid->getVel());
	advectVOF(_grid->getVel(), _grid->getPres());
    add_force(_grid->getVel());
	if(Properties::Get()->GRAVITY)
		applyGravity(_grid->getVel(), _grid->getPres());
	projection(_grid->getVel(), _grid->getPres());
	for (int i = 0; i < _grid->getVel().size(); i++) {
		if (dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getBoundary()[i] == 1)
			_grid->getVel()[i] = glm::vec2(0);
	}

    dynamic_cast<SimulationView*>(_views->getView("Simulation"))->setVelField(&_grid->getVel());

    //_grid->swapPres();
    //_grid->swapVel();
}

void Simulator::add_force(VecField &vel) {
    for (int y = 2; y < vel.height() - 2; y++)
        for (int x = 2; x < vel.width() - 2; x++) {
            vel(x, y) += dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getForces()(x, y) *  Properties::Get()->DT;

            if (x >= 30 && x < 42 && y > 50 && y < 60)
                vel(x, y) += glm::vec2(0, -.1);
        }
}

void Simulator::advect2(VecField& velNew, VecField& vel) {
    glm::vec2 rdx(vel.width(), vel.height());
    for (int y = 1; y < vel.height() - 1; y++) {
        for (int x = 1; x < vel.width() - 1; x++)
        {
            const glm::vec2 pos = glm::vec2(x, y) - Properties::Get()->DT * rdx * vel(x, y);
            velNew(x, y) = vel.interpolate(pos - glm::vec2(.5));
        }
    }
}

double CalcTimeToBoundaryAlongAxis(int axis,
                                   const glm::vec2 &vel,
                                   const glm::vec2 &curPos,
                                   int gridSize)
{
    // Out of high bounds (the position we want is at -vel)
    if (vel[axis] < 0)
        return (curPos[axis] - gridSize) / vel[axis];
    else // Out of low bounds
        return curPos[axis] / vel[axis];
}

#define EPSILON 1.0e-10
void ClipPos(glm::vec2 &prevPosGuess, const glm::vec2 &curPos, const glm::vec2 &vel, const int w, const int h) {
    if (prevPosGuess.x > 0 && prevPosGuess.x <= w
        && prevPosGuess.y > 0 && prevPosGuess.y <= h)
        return;

    // Check for components of velocity being zero since we will divide by them
    bool xVelIsZero = fabs(vel.x) < EPSILON;
    bool yVelIsZero = fabs(vel.y) < EPSILON;

	// If the velocity has zero magnitude, we can't clip it
    if (xVelIsZero && yVelIsZero)
        return;

    // Initialize to max to handle cases where the velocity in that axis is 0
    double timeToBoundaryInX = std::numeric_limits<double>::max();
    double timeToBoundaryInY = timeToBoundaryInX;

    if (!xVelIsZero)
        timeToBoundaryInX = CalcTimeToBoundaryAlongAxis(0, vel, curPos, w);

    if (!yVelIsZero)
        timeToBoundaryInY = CalcTimeToBoundaryAlongAxis(1, vel, curPos, h);

    // Correct for the direction that goes out of bounds first
    float timeToBoundary = timeToBoundaryInX;
    if (timeToBoundaryInY < timeToBoundaryInX)
        timeToBoundary = timeToBoundaryInY;

    prevPosGuess = curPos - (vel * timeToBoundary);
}

void Simulator::advect(VecField& velNew, VecField& field) {
    glm::vec2 prevPos, curPos;
    glm::vec2 vel;
    // Temporary field
    VecField newField(field.width(), field.height());

    for(int y = 1; y < field.height() - 1; y++) {
        for(int x = 1; x < field.width() - 1; x++) {
            // Determine the velocity at the grid centers
            curPos = glm::vec2(x - 0.5, y - 0.5);
            vel = field.interpolate(curPos);

            // Backtrack with the interpolated velocity
            prevPos = curPos - (vel * Properties::Get()->DT);

            // Apply clipping at boundary if necessary
            ClipPos(prevPos, curPos, vel, field.width(), field.height());

            // Find the new grid centered velocities based on the previous position
            newField(x, y) = field.interpolate(prevPos);
        }
    }

    for(int y = 1; y < field.height() - 1; y++) {
        for(int x = 1; x < field.width() - 1; x++) {
            // Interpolate the new grid centered velocities to the grid edges
            if (x == 1)
                field(x, y).x = 0;
            else
                field(x, y).x = 0.5f * (newField(x, y).x + newField(x - 1, y).x);

            if (y == 1)
                field(x, y).y = 0;
            else
                field(x, y).y = 0.5f * (newField(x, y).y + newField(x, y - 1).y);

        }
    }

}

#if 0
void Simulator::projection(VecField& field) {
    auto pressure = ScalField(field.width(), field.height());
    auto divergence = ScalField(field.width(), field.height());

    // Calculate the divergence of the current velocity field
    for(int y = 1; y < field.height() - 1; y++) {
        for(int x = 1; x < field.width() - 1; x++) {
            divergence(x, y) = field(x + 1, y).x
                             - field(x, y).x
                             + field(x, y + 1).y
                             - field(x, y).y;
        }
    }


    // Initialize Laplacian matrix
    ImplicitMatrixLap laplacian(field.width());

    // Solve for pressure using conjugate gradient
    int steps = Properties::Get()->MAX_STEPS;
    double err = ConjGrad(field.size(),
                          &laplacian,
                          pressure.data(),
                          divergence.data(),
                          Properties::Get()->THRESHOLD,
                          &steps);
	//const float err = ConjugateGradient(pressure, divergence, Properties::Get()->MAX_STEPS);
    if (err > Properties::Get()->THRESHOLD) {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
        exit(1);
    }

    // Calculate the new velocity field as v - grad(p)
    for(int y = 1; y < field.height() - 1; y++) {
        for(int x = 1; x < field.width() - 1; x++) {
            // Checks for boundaries
            if (x == 1) {
                if (y == 1) {
                    field(x, y) = glm::vec2(0, 0);
                }
                else {
                    const float dyPressure = pressure(x, y) - pressure(x, y - 1);
                    field(x, y) = field(x, y) - glm::vec2(0, dyPressure);
                }
            }
            else {
                if (y == 1) {
                    field(x, y) = field(x, y)
                        - glm::vec2(pressure(x, y) - pressure(x - 1, y), 0);
                }
                else {
                    const float dxPressure = pressure(x, y) - pressure(x - 1, y);
                    const float dyPressure = pressure(x, y) - pressure(x, y - 1);
                    field(x, y) = field(x, y) - glm::vec2(dxPressure, dyPressure);
                }
            }
        }
    }
}
#else
void Simulator::applyGravity(VecField& field, ScalField& vof) {
	const float dt = Properties::Get()->DT;
	for (int y = 1; y < field.height() - 1; y++) {
		for (int x = 1; x < field.width() - 1; x++) {
			if (vof(x, y) > 0.01f)
				field(x, y).y -= Properties::Get()->GRAVITY_FORCE * dt;
		}
	}
}

void Simulator::advectVOF(VecField& field, ScalField& vof) {
	ScalField vofNew(vof.width(), vof.height());
	const float dt = Properties::Get()->DT;

	for (int y = 1; y < vof.height() - 1; y++) {
		for (int x = 1; x < vof.width() - 1; x++) {

			// MAC grid: field(x,y).x is the u-velocity on the LEFT face of cell (x,y)
			//           field(x,y).y is the v-velocity on the BOTTOM face of cell (x,y)
			// Right face of (x,y) = field(x+1, y).x
			// Top  face of (x,y) = field(x, y+1).y

			float u_left  = field(x,   y).x;   // left face
			float u_right = field(x+1, y).x;   // right face
			float v_bot   = field(x, y  ).y;   // bottom face
			float v_top   = field(x, y+1).y;   // top face

			// Donor-acceptor: flux = velocity * F of upstream cell
			// X fluxes (positive = flowing right)
			float fx_left  = u_left  > 0 ? u_left  * vof(x-1, y) : u_left  * vof(x,   y);
			float fx_right = u_right > 0 ? u_right * vof(x,   y) : u_right * vof(x+1, y);

			// Y fluxes (positive = flowing up)
			float fy_bot = v_bot > 0 ? v_bot * vof(x, y-1) : v_bot * vof(x,   y);
			float fy_top = v_top > 0 ? v_top * vof(x,   y) : v_top * vof(x, y+1);

			// Net outflow = right - left + top - bottom (all scaled by dt)
			vofNew(x, y) = vof(x, y) - dt * ((fx_right - fx_left) + (fy_top - fy_bot));
			vofNew(x, y) = glm::clamp(vofNew(x, y), 0.0f, 1.0f);
		}
	}

	vof = vofNew;
}

void Simulator::projection(VecField& field, ScalField& vof) {
    auto pressure   = ScalField(field.width(), field.height());
    auto divergence = ScalField(field.width(), field.height());

    const float SURFACE_THRESH = 0.01f;  // below this = empty/air
    const float FULL_THRESH    = 0.99f;  // above this = fully fluid

    // --- Divergence (only in fluid cells) ---
    for (int y = 1; y < field.height() - 1; y++) {
        for (int x = 1; x < field.width() - 1; x++) {
            if (vof(x, y) < SURFACE_THRESH) {
                divergence(x, y) = 0.0f;   // air cell — no constraint
                continue;
            }
            if (vof(x, y) < FULL_THRESH) {
                divergence(x, y) = 0.0f;   // surface cell — p=0 Dirichlet, skip divergence
                continue;
            }
            // Full fluid cell — standard divergence
            divergence(x, y) = field(x+1, y).x - field(x, y).x
                              + field(x, y+1).y - field(x, y).y;
        }
    }

    const float err = ConjugateGradient2(pressure, divergence, vof, Properties::Get()->MAX_STEPS);
    if (err > Properties::Get()->THRESHOLD) {
        fprintf(stderr, "projection did not converge! error: %f\n", err);
        exit(1);
    }

	for (int y = 0; y < pressure.height(); y++)
		for (int x = 0; x < pressure.width(); x++)
			if (vof(x, y) < FULL_THRESH)
				pressure(x, y) = 0.0f;

    // --- Velocity correction (only in fluid, zero in air) ---
    for (int y = 1; y < field.height() - 1; y++) {
        for (int x = 1; x < field.width() - 1; x++) {

            if (vof(x, y) < SURFACE_THRESH) {
                field(x, y) = glm::vec2(0.0f);  // air — kill velocity
                continue;
            }

            // Same boundary logic as before, unchanged
            if (x == 1) {
                if (y == 1) {
                    field(x, y) = glm::vec2(0, 0);
                } else {
                    field(x, y) -= glm::vec2(0, pressure(x,y) - pressure(x, y-1));
                }
            } else {
                if (y == 1) {
                    field(x, y) -= glm::vec2(pressure(x,y) - pressure(x-1, y), 0);
                } else {
                    field(x, y) -= glm::vec2(pressure(x,y) - pressure(x-1, y),
                                             pressure(x,y) - pressure(x, y-1));
                }
            }
        }
    }
}
#endif


void Simulator::VorticityConfinement() {
	VecField& velField = _grid->getVel();

	// The curl
	ScalField curl(velField.width(), velField.height());
	// The normalized gradient of the curl
	VecField gradCurl(velField.width(), velField.height());
	// The forces which will counteract numerical diffusion
	VecField forces(velField.width(), velField.height());

	// Calculate the magnitude of the curl of v at grid centers
	for(int y = 1; y < velField.height() - 1; y++) {
		for(int x = 1; x < velField.width() - 1; x++) {
			float bottom, right, top, left;

			// Check for boundaries
			if (x == 1)
				left = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[1];
			else
				left = velField.interpolate(glm::vec2(x - 1.5, y - 0.5))[1];

			if (x == velField.width() - 2)
				right = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[1];
			else
				right = velField.interpolate(glm::vec2(x + 0.5, y - 0.5))[1];

			if (y == 1)
				bottom = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[0];
			else
				bottom = velField.interpolate(glm::vec2(x - 0.5, y - 1.5))[0];

			if (y == velField.height() - 2)
				top = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[0];
			else
				top = velField.interpolate(glm::vec2(x - 0.5, y + 0.5))[0];

			curl(x, y) = (right - left - top + bottom) / 2;
		}
	}

	glm::vec2 grad_w;
	// Calculate the normalized gradient of the curls on the faces
	for(int y = 1; y < velField.height() - 1; y++) {
		for(int x = 1; x < velField.width() - 1; x++) {
			// Check for boundaries
			if (x == 1) {
				if (y == 1)
					grad_w = glm::vec2(0, 0);
				else
					grad_w = glm::vec2(0, curl(x, y) - curl(x, y - 1));
			}
			else if (y == 1) {
				grad_w = glm::vec2(curl(x, y) - curl(x - 1, y), 0);
			}
			else {
				grad_w = glm::vec2(curl(x, y) - curl(x - 1, y),
							  curl(x, y) - curl(x, y - 1));
			}

			if (glm::length(grad_w) > EPSILON)
				gradCurl(x, y) = grad_w / glm::length(grad_w);
			else
				gradCurl(x, y) = glm::vec2(0, 0);
		}
	}

	// Average the normals to the grid centers
	for(int y = 1; y < velField.height() - 1; y++) {
		for(int x = 1; x < velField.width() - 1; x++) {
			double gradCurlX = 0.5 * (gradCurl(x, y)[0]
									  + gradCurl(x + 1, y)[0]);
			double gradCurlY = 0.5 * (gradCurl(x, y)[1]
									  + gradCurl(x, y + 1)[1]);
			gradCurl(x, y) = glm::vec2(gradCurlX, gradCurlY);
		}
	}

	// Compute the needed forces at the centers of each cell
	for(int y = 1; y < velField.height() - 1; y++) {
		for(int x = 1; x < velField.width() - 1; x++) {
			double forceX = gradCurl(x, y)[1] * curl(x, y);
			double forceY = -gradCurl(x, y)[0] * curl(x, y);
			forces(x, y) = Properties::Get()->a * glm::vec2(forceX, forceY);
		}
	}

	// Average the forces to the faces of each cell
	for(int y = 1; y < velField.height() - 1; y++) {
		for(int x = 1; x < velField.width() - 1; x++) {
			if (x == 1) {
				if (y == 1) {
					forces(x, y) = glm::vec2(0, 0);
				}
				else {
					double forceY = 0.5 * (forces(x, y)[1] + forces(x, y - 1)[1]);
					forces(x, y) = glm::vec2(0, forceY);
				}
			}
			else if (y == 1) {
				double forceX = 0.5 * (forces(x, y)[0] + forces(x - 1, y)[0]);
				forces(x, y) = glm::vec2(forceX, 0);
			}
			else {
				double forceX = 0.5 * (forces(x, y)[0] + forces(x - 1, y)[0]);
				double forceY = 0.5 * (forces(x, y)[1] + forces(x, y - 1)[1]);
				forces(x, y) = glm::vec2(forceX, forceY);
			}
		}
	}

	// Apply forces to the field
	for (int i = 0; i < velField.size(); i++)
		velField[i] +=  Properties::Get()->DT * forces[i];
}