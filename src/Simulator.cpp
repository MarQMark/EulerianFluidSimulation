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

	//for (int y = 40; y < 50; y++)
	//	for (int x = 40; x < 50; x++)
	//		_grid->getPres()(x, y) = 1;
//
	_grid->getPres() = 1.f;
	//_grid->getPres().set(1.f);

	for (int y = 0; y < _grid->getVel().height() / 2; y++)
		for (int x = 0; x < _grid->getVel().width(); x++)
			_grid->getPres()(x, y) = 1;
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

	extrapolateVelocity(_grid->getVel(), _grid->getPres());

	// 2. Advect rho
	ScalField rhoNew(_grid->getPres().width(), _grid->getPres().height());
	advectScalar(rhoNew, _grid->getPres(), _grid->getVel());
	_grid->getPres() = rhoNew;

	// 3. Sharpen rho
	sharpenDensity(_grid->getPres());

    advect(_grid->getNewVel(), _grid->getVel());
    //_grid->swapVel();
	//VorticityConfinement();

    add_force(_grid->getVel(), _grid->getPres());
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

void Simulator::add_force(VecField &vel, ScalField &rho) {
    for (int y = 2; y < vel.height() - 2; y++)
        for (int x = 2; x < vel.width() - 2; x++) {

        	bool fluidHere  = rho(x, y)   > 0.5f;
        	bool fluidBelow = rho(x, y-1) > 0.5f;
        	bool fluidLeft  = rho(x-1, y) > 0.5f;

        	// Y-face: between (x,y) and (x,y-1)
        	if (!(fluidHere || fluidBelow))   // 'or' is more stable than 'and'
        		continue;

            vel(x, y) += dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getForces()(x, y) *  Properties::Get()->DT;


        	if (Properties::Get()->PUSH)
				if (x >= 30 && x < 42 && y > 50 && y < 60)
					vel(x, y) += glm::vec2(0, -Properties::Get()->PUSH_FORCE);
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
void Simulator::applyGravity(VecField& field, ScalField& rho) {
	const float dt = Properties::Get()->DT;
	for (int y = 1; y < field.height() - 1; y++) {
		for (int x = 1; x < field.width() - 1; x++) {
			if (rho(x,y) > 0.5f)
				field(x, y).y -= Properties::Get()->GRAVITY_FORCE * dt;
		}
	}
}

void Simulator::extrapolateVelocity(VecField& vel, const ScalField& rho) {
	// We need velocity in air cells near the surface so that
	// backtracing from those cells finds valid fluid density.
	// Simple approach: BFS outward from fluid cells for N layers.

	const int LAYERS = 4;  // how many cells deep into air to fill
	const int w = vel.width();
	const int h = vel.height();

	// Mark which cells have valid velocity: fluid cells do, air cells don't
	ScalField valid(w, h);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			valid(x, y) = (rho(x, y) > 0.5f) ? 1.0f : 0.0f;

	ScalField validNew(w, h);
	VecField  velNew(w, h);

	for (int layer = 0; layer < LAYERS; layer++) {
		validNew = valid;
		velNew   = vel;

		for (int y = 1; y < h - 1; y++) {
			for (int x = 1; x < w - 1; x++) {
				if (valid(x, y) > 0.5f) continue;  // already valid

				// Average all valid neighbors
				glm::vec2 sum(0.0f);
				int count = 0;

				auto check = [&](int nx, int ny) {
					if (valid(nx, ny) > 0.5f) {
						sum += vel(nx, ny);
						count++;
					}
				};

				check(x - 1, y);
				check(x + 1, y);
				check(x, y - 1);
				check(x, y + 1);

				if (count > 0) {
					velNew(x, y)   = sum / (float)count;
					validNew(x, y) = 1.0f;
				}
			}
		}

		valid = validNew;
		vel   = velNew;
	}
}

void Simulator::advectScalar(ScalField& rhoNew, const ScalField& rho, VecField& vel) {
	const float dt = Properties::Get()->DT;

	for (int y = 1; y < rho.height() - 1; y++) {
		for (int x = 1; x < rho.width() - 1; x++) {
			// Cell center position
			glm::vec2 pos(x - 0.5f, y - 0.5f);

			// Interpolate velocity at cell center from staggered field
			// vel(x,y).x is LEFT face, vel(x,y).y is BOTTOM face
			glm::vec2 u;
			u.x = 0.5f * (vel(x, y).x + vel(x + 1, y).x);
			u.y = 0.5f * (vel(x, y).y + vel(x, y + 1).y);

			// Backtrack
			glm::vec2 prev = pos - u * dt;

			// Clamp to interior
			prev.x = glm::clamp(prev.x, 0.5f, rho.width()  - 1.5f);
			prev.y = glm::clamp(prev.y, 0.5f, rho.height() - 1.5f);

			//rhoNew(x, y) = rho.interpolate(prev);
			rhoNew(x, y) = interpolateScalar(rho, prev);
		}
	}
}

// Bilinear interpolation for scalar field (cell-centered, so offset by -0.5)
float Simulator::interpolateScalar(const ScalField& f, glm::vec2 pos) {
	// pos is in cell-center coords, shift to index space
	float px = pos.x + 0.5f;
	float py = pos.y + 0.5f;

	int x0 = (int)px;  int y0 = (int)py;
	int x1 = x0 + 1;   int y1 = y0 + 1;

	x0 = glm::clamp(x0, 0, (int)f.width()  - 1);
	x1 = glm::clamp(x1, 0, (int)f.width()  - 1);
	y0 = glm::clamp(y0, 0, (int)f.height() - 1);
	y1 = glm::clamp(y1, 0, (int)f.height() - 1);

	float tx = px - x0;
	float ty = py - y0;

	return (1-tx)*(1-ty)*f(x0,y0) + tx*(1-ty)*f(x1,y0)
		 + (1-tx)*ty   *f(x0,y1) + tx*ty    *f(x1,y1);
}

void Simulator::sharpenDensity(ScalField& rho) {
    const float dt  = Properties::Get()->DT;
    const float dT  = 3.0f * dt;   // fictitious timestep, paper uses 3x
    const float tau = 0.4f;        // max density diff between neighbors

    ScalField delta(rho.width(), rho.height());

    for (int y = 1; y < rho.height() - 1; y++) {
        for (int x = 1; x < rho.width() - 1; x++) {
            float r  = rho(x, y);
            float rR = rho(x+1, y), rL = rho(x-1, y);
            float rU = rho(x, y+1), rD = rho(x, y-1);

            // Upwind gradient magnitude (paper eq. 6-13)
            // δx+ = -(ρ_i - ρ_{i-1}) * dT
            // δx- = -(ρ_{i+1} - ρ_i) * dT
            float dxP = -(r  - rL) * dT;
            float dxM = -(rR - r)  * dT;
            float dyP = -(r  - rD) * dT;
            float dyM = -(rU - r)  * dT;

            // Max gradient magnitude for increase vs decrease (eq. 8-13)
            float gradPos = sqrtf(
                std::max({powf(std::max(dxP,0.f),2.f), powf(std::min(dxM,0.f),2.f)}) +
                std::max({powf(std::max(dyP,0.f),2.f), powf(std::min(dyM,0.f),2.f)})
            );
            float gradNeg = sqrtf(
                std::max({powf(std::min(dxP,0.f),2.f), powf(std::max(dxM,0.f),2.f)}) +
                std::max({powf(std::min(dyP,0.f),2.f), powf(std::max(dyM,0.f),2.f)})
            );

            // Sharpening weight w (eq. 14)
            // Drives ρ away from 0.5: positive above, negative below
            float maxNeighborDiff = std::max({fabsf(r-rR), fabsf(r-rL),
                                              fabsf(r-rU), fabsf(r-rD)});
            float w = powf(r - 0.5f, 3.0f)
                    * (1.0f - std::min(1.0f, maxNeighborDiff / tau));

            // Density correction (eq. 15)
            float dr = w * (w >= 0 ? gradPos : gradNeg);

            // Local mass conservation (eq. 17):
            // Don't modify cells already on the fluid side (ρ > 0.5)
            // Don't let ρ go negative
            if (r > 0.5f)
                dr = 0.0f;
            else if (r + dr < 0.0f)
                dr = -r;

            delta(x, y) = dr;
        }
    }

    // Apply correction + scatter lost mass back toward the 0.5 iso-contour
    for (int y = 1; y < rho.height() - 1; y++) {
        for (int x = 1; x < rho.width() - 1; x++) {
            rho(x, y) += delta(x, y);

            // Scatter -delta (recovered mass) toward gradient direction
            // Simple version: distribute to steepest neighbor toward surface
            float dr = -delta(x, y);
            if (fabsf(dr) < 1e-6f) continue;

            // Follow density gradient toward 0.5 iso-contour (paper Algorithm 2)
            // In 2D, just deposit to the neighbor with rho closest to 0.5
            float best = 1e9f;
            int bx = x, by = y;
            for (auto [nx, ny] : {std::pair{x+1,y},{x-1,y},{x,y+1},{x,y-1}}) {
                float d = fabsf(rho(nx, ny) - 0.5f);
                if (d < best) { best = d; bx = nx; by = ny; }
            }
            rho(bx, by) += dr;
            rho(bx, by)  = glm::clamp(rho(bx, by), 0.0f, 1.0f);
        }
    }

    // Final clamp
    for (int y = 0; y < rho.height(); y++)
        for (int x = 0; x < rho.width(); x++)
            rho(x, y) = glm::clamp(rho(x, y), 0.0f, 1.0f);
}

void Simulator::projection(VecField& field, ScalField& rho) {
    auto pressure   = ScalField(field.width(), field.height());
    auto divergence = ScalField(field.width(), field.height());

	const float lambda = 0.5f;  // paper uses 0.5
	const float eta    = 1.0f;  // paper uses 1.0

	for (int y = 1; y < field.height() - 1; y++) {
		for (int x = 1; x < field.width() - 1; x++) {
			if (rho(x, y) <= 0.5f) {
				divergence(x, y) = 0.0f;
				continue;
			}

			divergence(x, y) = field(x+1,y).x - field(x,y).x
							  + field(x,y+1).y - field(x,y).y;

			// Artificial divergence to correct rho > 1 (paper sec 3.7)
			// Pushes excess density out of overfull cells
			if (rho(x, y) > 1.0f) {
				float excess = rho(x, y) - 1.0f;
				divergence(x, y) += std::min(lambda * excess, eta);
			}
		}
	}

    const float err = ConjugateGradient2(pressure, divergence, rho, Properties::Get()->MAX_STEPS);
    if (err > Properties::Get()->THRESHOLD) {
        fprintf(stderr, "projection did not converge! error: %f\n", err);
        exit(1);
    }

    for (int y = 1; y < field.height() - 1; y++) {
        for (int x = 1; x < field.width() - 1; x++) {

        	if (rho(x, y) <= 0.5f) {
        		field(x, y) = glm::vec2(0.0f);
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