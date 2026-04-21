#include "Simulator.h"

#include <algorithm>
#include <iostream>
#include <views/SimulationView.h>

#include "simulation/Properties.h"
#include "simulation/ScalField.h"
#include "solver/LinearSolver.hpp"
#include "views/PropertiesView.h"

#define RES_X 72
#define RES_Y 72

#define DT 01.67f

Simulator::Simulator() {
    _window = new Window(1280, 720, "StableFluids");
    _renderer = new Renderer(RES_X, RES_Y);
    _views = new Views(_window);

    _views->addView("Simulation", new SimulationView(_renderer->getTexture2D()->get(), RES_X, RES_Y));
    _views->addView("Properties", new PropertiesView());

    _grid = new Grid(RES_X, RES_Y);
}

Simulator::~Simulator() {
    delete _grid;
    delete _views;
    delete _renderer;
    delete _window;
}

void Simulator::update() {
    while (!_window->shouldClose()) {
        simulate();
        _views->render();
        _renderer->render(_grid);
        _window->update();

        if(ImGui::IsKeyPressed(ImGuiKey_R)) {
            delete _grid;
            _grid = new Grid(RES_X, RES_Y);
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
    add_force(_grid->getVel());
	VorticityConfinement();
    projection(_grid->getVel());

    dynamic_cast<SimulationView*>(_views->getView("Simulation"))->setVelField(&_grid->getVel());

    //_grid->swapPres();
    //_grid->swapVel();
}

glm::vec2 bilinearInterpol(glm::vec2 pos, const std::vector<glm::vec2> &field) {
    pos += glm::vec2(.5);
    const int x = (int)pos.x;
    const int y = (int)pos.y;

    int ix1 = x;
    int ix2 = x + 1;
    if(pos.x - (float)x < 0.5f) {
        ix1 = x - 1;
        ix2 = x;
    }
    int iy1 = y;
    int iy2 = y + 1;
    if(pos.y - (float)y < 0.5f) {
        iy1 = y - 1;
        iy2 = y;
    }

    ix1 = std::clamp(ix1, 0, RES_X - 1);
    ix2 = std::clamp(ix2, 0, RES_X - 1);
    iy1 = std::clamp(iy1, 0, RES_Y - 1);
    iy2 = std::clamp(iy2, 0, RES_Y - 1);

    glm::vec2 v[4] = {
        field[iy1 * RES_X + ix1],
        field[iy2 * RES_X + ix1],
        field[iy1 * RES_X + ix2],
        field[iy2 * RES_X + ix2],
    };

    float x1 = (float)ix1 + 0.5f;
    float x2 = (float)ix2 + 0.5f;
    float y1 = (float)iy1 + 0.5f;
    float y2 = (float)iy2 + 0.5f;

    const float dx2x1 = x2 - x1;
    glm::vec2 fxy1 = ((x2 - pos.x)/dx2x1) * v[0] + ((pos.x - x1)/dx2x1) * v[2];
    glm::vec2 fxy2 = ((x2 - pos.x)/dx2x1) * v[1] + ((pos.x - x1)/dx2x1) * v[3];
    if(dx2x1 < 1e-9) {
        fxy1 = v[1];
        fxy2 = v[3];
    }

    const float dy2y1 = y2 - y1;
    glm::vec2 fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
    if(dy2y1 < 1e-9)
        fxy = fxy1;

    return fxy;
}
glm::vec2 bilinearInterpol2(const glm::vec2 pos, const std::vector<glm::vec2> &field)
{
    int x0 = (int)std::floor(pos.x);
    int y0 = (int)std::floor(pos.y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    x0 = std::clamp(x0, 0, RES_X - 1);
    y0 = std::clamp(y0, 0, RES_Y - 1);
    x1 = std::clamp(x1, 0, RES_X - 1);
    y1 = std::clamp(y1, 0, RES_Y - 1);

    float sx = pos.x - x0;
    float sy = pos.y - y0;

    auto at = [&](int x, int y) -> const glm::vec2& {
        return field[y * RES_X + x];
    };

    const glm::vec2 &v00 = at(x0, y0);
    const glm::vec2 &v10 = at(x1, y0);
    const glm::vec2 &v01 = at(x0, y1);
    const glm::vec2 &v11 = at(x1, y1);

    glm::vec2 v0 = (1.0f - sx) * v00 + sx * v10;
    glm::vec2 v1 = (1.0f - sx) * v01 + sx * v11;

    glm::vec2 ret = (1.0f - sy) * v0 + sy * v1;

    return ret;
}

void Simulator::add_force(VecField &vel) {
    for (int y = 2; y < RES_Y - 2; y++)
        for (int x = 2; x < RES_X - 2; x++) {
            vel(x, y) += dynamic_cast<SimulationView*>(_views->getView("Simulation"))->getForces()(x, y) * DT;

            if (x >= 30 && x < 42 && y > 50 && y < 60)
                vel(x, y) += glm::vec2(0, -.1);
        }
}

void Simulator::advect2(VecField& velNew, VecField& vel) {
    glm::vec2 rdx(RES_X, RES_Y);
    for (int y = 1; y < RES_Y - 1; y++) {
        for (int x = 1; x < RES_X - 1; x++)
        {
            const glm::vec2 pos = glm::vec2(x, y) - DT * rdx * vel(x, y);
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
    {
        return (curPos[axis] - gridSize) / vel[axis];
    }
    else // Out of low bounds
    {
        return curPos[axis] / vel[axis];
    }
}

#define EPSILON 1.0e-10
void ClipPos(glm::vec2 &prevPosGuess, const glm::vec2 &curPos, const glm::vec2 &vel, int gridSize)
{
    if (prevPosGuess[0] > 0 && prevPosGuess[0] <= gridSize
        && prevPosGuess[1] > 0 && prevPosGuess[1] <= gridSize)
    {
        // The guess position is inbounds so no need to clip
        return;
    }

    // Check for components of velocity being zero since we will divide by them
    bool xVelIsZero = fabs(vel[0]) < EPSILON;
    bool yVelIsZero = fabs(vel[1]) < EPSILON;

    if (xVelIsZero && yVelIsZero)
    {
        // If the velocity has zero magnitude, we can't clip it
        return;
    }

    // Initialize to max to handle cases where the velocity in that axis is 0
    double timeToBoundaryInX = std::numeric_limits<double>::max();
    double timeToBoundaryInY = timeToBoundaryInX;

    if (!xVelIsZero)
    {
        timeToBoundaryInX = CalcTimeToBoundaryAlongAxis(0, vel, curPos, gridSize);
    }

    if (!yVelIsZero)
    {
        timeToBoundaryInY = CalcTimeToBoundaryAlongAxis(1, vel, curPos, gridSize);
    }

    // Correct for the direction that goes out of bounds first
    float timeToBoundary = timeToBoundaryInX;
    if (timeToBoundaryInY < timeToBoundaryInX)
    {
        timeToBoundary = timeToBoundaryInY;
    }

    prevPosGuess = curPos - (vel * timeToBoundary);
}

void Simulator::advect(VecField& velNew, VecField& field) {
    glm::vec2 prevPos, curPos;
    glm::vec2 vel;
    // Temporary field
    VecField newField(field.width(), field.height());

    for(int y = 1; y < field.height() - 1; y++)
    {
        for(int x = 1; x < field.width() - 1; x++)
        {
            // Determine the velocity at the grid centers
            curPos = glm::vec2(x - 0.5, y - 0.5);
            vel = field.interpolate(curPos);

            // Backtrack with the interpolated velocity
            prevPos = curPos - (vel * DT);

            // Apply clipping at boundary if necessary
            ClipPos(prevPos, curPos, vel, field.height());

            // Find the new grid centered velocities based on the previous position
            newField(x, y) = field.interpolate(prevPos);
        }
    }

    //return;
    for(int y = 1; y < field.height() - 1; y++)
    {
        for(int x = 1; x < field.width() - 1; x++)
        {
            // Interpolate the new grid centered velocities to the grid edges
            if (x == 1)
                field(x, y)[0] = 0;
            else
                field(x, y)[0] = 0.5 * (newField(x, y)[0] + newField(x - 1, y)[0]);

            if (y == 1)
                field(x, y)[1] = 0;
            else
                field(x, y)[1] = 0.5 * (newField(x, y)[1] + newField(x, y - 1)[1]);

        }
    }

}


#define MAX_STEPS 100000
#define EPSILON 1.0e-5

void Simulator::projection(VecField& field) {
    //auto pressure = ScalField(field.width(), field.height());
    //auto divergence = ScalField(field.width(), field.height());

    auto* pres = new ScalField(field.width(), field.height());
    auto* div = new ScalField(field.width(), field.height());
    auto pressure = *pres;
    auto divergence = *div;


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
    //glm::mat<field.width(), field.height()> laplacian;

    // Solve for pressure using conjugate gradient
    int steps = MAX_STEPS;
    double err = ConjGrad(field.size(),
                          &laplacian,
                          pressure.data(),
                          divergence.data(),
                          EPSILON,
                          &steps);
    if (err > EPSILON)
    {
        fprintf(stderr, "solution for projection did not converge! error: %f\n", err);
        exit(1);
    }

    // Calculate the new velocity field as v - grad(p)
    for(int y = 1; y < field.height() - 1; y++)
    {
        for(int x = 1; x < field.width() - 1; x++)
        {
            // Checks for boundaries
            if (x == 1)
            {
                if (y == 1)
                {
                    field(x, y) = glm::vec2(0, 0);
                }
                else
                {
                    double yPressureDelta = pressure(x, y)
                                          - pressure(x, y - 1);
                    field(x, y) = field(x, y) - glm::vec2(0, yPressureDelta);
                }
            }
            else
            {
                if (y == 1)
                {
                    field(x, y) = field(x, y)
                        - glm::vec2(pressure(x, y)
                        - pressure(x - 1, y), 0);
                }
                else
                {
                    double xPressureDelta = pressure(x, y)
                                          - pressure(x - 1, y);
                    double yPressureDelta = pressure(x, y)
                                          - pressure(x, y - 1);
                    field(x, y) = field(x, y) - glm::vec2(xPressureDelta, yPressureDelta);
                }
            }
        }
    }

    delete pres;
    delete div;
}

void Simulator::VorticityConfinement()
{
	VecField& velField = _grid->getVel();

	// The curl
	ScalField curl(velField.width(), velField.height());
	// The normalized gradient of the curl
	VecField gradCurl(velField.width(), velField.height());
	// The forces which will counteract numerical diffusion
	VecField forces(velField.width(), velField.height());

	// Calculate the magnitude of the curl of v at grid centers
	for(int y = 1; y < velField.height() - 1; y++)
	{
		for(int x = 1; x < velField.width() - 1; x++)
		{
			double bottom, right, top, left;

			// Check for boundaries
			if (x == 1)
			{
				left = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[1];
			}
			else
			{
				left = velField.interpolate(glm::vec2(x - 1.5, y - 0.5))[1];
			}

			if (x == velField.width() - 2)
			{
				right = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[1];
			}
			else
			{
				right = velField.interpolate(glm::vec2(x + 0.5, y - 0.5))[1];
			}

			if (y == 1)
			{
				bottom = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[0];
			}
			else
			{
				bottom = velField.interpolate(glm::vec2(x - 0.5, y - 1.5))[0];
			}

			if (y == velField.height() - 2)
			{
				top = velField.interpolate(glm::vec2(x - 0.5, y - 0.5))[0];
			}
			else
			{
				top = velField.interpolate(glm::vec2(x - 0.5, y + 0.5))[0];
			}

			curl(x, y) = (right - left - top + bottom) / 2;
		}
	}

	glm::vec2 grad_w;
	// Calculate the normalized gradient of the curls on the faces
	for(int y = 1; y < velField.height() - 1; y++)
	{
		for(int x = 1; x < velField.width() - 1; x++)
		{
			// Check for boundaries
			if (x == 1)
			{
				if (y == 1)
				{
					grad_w = glm::vec2(0, 0);
				}
				else
				{
					grad_w = glm::vec2(0, curl(x, y) - curl(x, y - 1));
				}
			}
			else if (y == 1)
			{
				grad_w = glm::vec2(curl(x, y) - curl(x - 1, y), 0);
			}
			else
			{
				grad_w = glm::vec2(curl(x, y) - curl(x - 1, y),
							  curl(x, y) - curl(x, y - 1));
			}

			if (glm::length(grad_w) > EPSILON)
			{
				gradCurl(x, y) = grad_w / glm::length(grad_w);
			}
			else
			{
				gradCurl(x, y) = glm::vec2(0, 0);
			}
		}
	}

	// Average the normals to the grid centers
	for(int y = 1; y < velField.height() - 1; y++)
	{
		for(int x = 1; x < velField.width() - 1; x++)
		{
			double gradCurlX = 0.5 * (gradCurl(x, y)[0]
									  + gradCurl(x + 1, y)[0]);
			double gradCurlY = 0.5 * (gradCurl(x, y)[1]
									  + gradCurl(x, y + 1)[1]);
			gradCurl(x, y) = glm::vec2(gradCurlX, gradCurlY);
		}
	}

	// Compute the needed forces at the centers of each cell
	for(int y = 1; y < velField.height() - 1; y++)
	{
		for(int x = 1; x < velField.width() - 1; x++)
		{
			double forceX = gradCurl(x, y)[1] * curl(x, y);
			double forceY = -gradCurl(x, y)[0] * curl(x, y);
			forces(x, y) = Properties::Get()->a * glm::vec2(forceX, forceY);
		}
	}

	// Average the forces to the faces of each cell
	for(int y = 1; y < velField.height() - 1; y++)
	{
		for(int x = 1; x < velField.width() - 1; x++)
		{
			if (x == 1)
			{
				if (y == 1)
				{
					forces(x, y) = glm::vec2(0, 0);
				}
				else
				{
					double forceY = 0.5 * (forces(x, y)[1] + forces(x, y - 1)[1]);
					forces(x, y) = glm::vec2(0, forceY);
				}
			}
			else if (y == 1)
			{
				double forceX = 0.5 * (forces(x, y)[0] + forces(x - 1, y)[0]);
				forces(x, y) = glm::vec2(forceX, 0);
			}
			else
			{
				double forceX = 0.5 * (forces(x, y)[0] + forces(x - 1, y)[0]);
				double forceY = 0.5 * (forces(x, y)[1] + forces(x, y - 1)[1]);
				forces(x, y) = glm::vec2(forceX, forceY);
			}
		}
	}

	// Apply forces to the field
	for (int i = 0; i < velField.size(); i++)
	{
		velField[i] += DT * forces[i];
	}
}