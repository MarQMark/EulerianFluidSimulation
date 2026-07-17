#include "Application.h"

#include <ranges>
#include "core/Input.h"
#include "simulation/Properties.h"
#include "simulation/simulator/CPUSimulator2D.h"
#include "simulation/simulator/CPUSimulator3D.h"
#include "simulation/simulator/OpenGLSimulator2D.h"
#include "util/Time.h"
#include "views/DistributionView.h"
#include "views/FPSView.h"
#include "views/ProfilerView.h"
#include "views/PropertiesView.h"
#include "views/SolverStepsView.h"
#include "views/StatsView.h"

#ifdef HAS_CUDA
#include <cuda_runtime.h>
#include "simulation/simulator/CudaSimulator2D.h"
#endif


Application::Application() {
    _window = new Window(1280, 720, "Eulerian Fluid Simulation");
	Input::Get()->addInputWindow(_window);

    _views = new Views(_window);
    _views->addView("Properties", new PropertiesView());
    _views->addView("Performance", new FPSView());
    _views->addView("Profiler", new ProfilerView());
    _views->addView("SolverSteps", new SolverStepsView());

	rebuild();

    Time::Get()->targetFps = 165;

#ifdef HAS_CUDA
	int count = 0;
	const cudaError_t err = cudaGetDeviceCount(&count);

	if (err != cudaSuccess)
		std::cout << cudaGetErrorString(err) << '\n';
	else
		std::cout << "Cuda Available" << '\n';
#endif
}

Application::~Application() {
    delete _simulator;
    delete _display;
    delete _views;
    delete _window;
}

void Application::update() {
    while (!_window->shouldClose()) {
    	_window->processEvents();
    	Time::Get()->update();

    	_display->update(_simulator->getRenderData());

    	if ((!PROPERTY_B(PAUSED) || PROPERTY_I(SIMULATION_STEPS) > 0) && PROPERTY_B(SIMULATION_INITIALIZED)) {
    		_simulator->update();

    		if (PROPERTY_I(SIMULATION_STEPS) > 0)
    			ADD_PROPERTY_I(SIMULATION_STEPS, -1);
    	}

        _display->render(_simulator->getRenderData());
        _views->update(_simulator->getRenderData());
    	_window->swapBuffers();

        input_handler();

    	Time::Get()->limitFPS();
    	Input::Get()->update();

    	if (PROPERTY_B(SIMULATION_REBUILD))
    		rebuild();
    	if (PROPERTY_B(SIMULATION_RESET))
			reset();
    }
}

void Application::rebuild() {
	SET_PROPERTY_B(SIMULATION_REBUILD, false);

    if (_simulator) delete _simulator;
    if (_display) {
    	for (const auto& name: _display->viewables() | std::views::keys)
			_views->deleteView(name);
        delete _display;
    }

    switch (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType)) {
        case Properties::OPENGL_2D:
        {
    		const int WIDTH	 = PROPERTY_I(WIDTH_2D);
    		const int HEIGHT = PROPERTY_I(HEIGHT_2D);

    		_simulator = new OpenGLSimulator2D(WIDTH, HEIGHT);
            _display = new SimulationDisplay2D(WIDTH, HEIGHT);
            break;
        }
        case Properties::CPU_3D:
        {
            _display = new SimulationDisplay3D(_window);

    		const int WIDTH	 = PROPERTY_I(WIDTH_3D);
    		const int HEIGHT = PROPERTY_I(HEIGHT_3D);
    		const int DEPTH	 = PROPERTY_I(DEPTH_3D);

    		_simulator = new CPUSimulator3D(WIDTH, HEIGHT, DEPTH);
            break;
        }
#ifdef HAS_CUDA
    	case Properties::CUDA_2D:
        {
        	const int WIDTH	 = PROPERTY_I(WIDTH_2D);
        	const int HEIGHT = PROPERTY_I(HEIGHT_2D);

        	_simulator = new CudaSimulator2D(WIDTH, HEIGHT);
        	_display = new SimulationDisplay2D(WIDTH, HEIGHT);
        	break;
        }
    	case Properties::CUDA_3D:
        {
        	const int WIDTH	 = PROPERTY_I(WIDTH_3D);
        	const int HEIGHT = PROPERTY_I(HEIGHT_3D);
        	const int DEPTH	 = PROPERTY_I(DEPTH_3D);

        	//_simulator = new CudaSimulator3D(WIDTH, HEIGHT, DEPTH);
        	_display = new SimulationDisplay3D(_window);
        	break;
        }
#endif
        case Properties::CPU_2D:
        default:
        {
    		const int WIDTH	 = PROPERTY_I(WIDTH_2D);
    		const int HEIGHT = PROPERTY_I(HEIGHT_2D);

            _simulator = new CPUSimulator2D(WIDTH, HEIGHT);
            _display = new SimulationDisplay2D(WIDTH, HEIGHT);
            break;
        }
    }

	for (const auto& [name, v] : _display->viewables())
		_views->addView(name, v);

	SET_PROPERTY_B(SIMULATION_INITIALIZED, true);
}

void Application::reset() {
	SET_PROPERTY_B(SIMULATION_RESET, false);

	if (_simulator) delete _simulator;

	switch (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType)) {
		case Properties::OPENGL_2D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_2D);
			const int HEIGHT = PROPERTY_I(HEIGHT_2D);

			_simulator = new OpenGLSimulator2D(WIDTH, HEIGHT);
			break;
		}
		case Properties::CPU_3D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_3D);
			const int HEIGHT = PROPERTY_I(HEIGHT_3D);
			const int DEPTH	 = PROPERTY_I(DEPTH_3D);

			_simulator = new CPUSimulator3D(WIDTH, HEIGHT, DEPTH);
			break;
		}
#ifdef HAS_CUDA
		case Properties::CUDA_2D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_2D);
			const int HEIGHT = PROPERTY_I(HEIGHT_2D);

			_simulator = new CudaSimulator2D(WIDTH, HEIGHT);
			break;
		}
		case Properties::CUDA_3D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_3D);
			const int HEIGHT = PROPERTY_I(HEIGHT_3D);
			const int DEPTH	 = PROPERTY_I(DEPTH_3D);

			//_simulator = new CudaSimulator3D(WIDTH, HEIGHT, DEPTH);
			break;
		}
#endif
		case Properties::CPU_2D:
		default:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_2D);
			const int HEIGHT = PROPERTY_I(HEIGHT_2D);

			_simulator = new CPUSimulator2D(WIDTH, HEIGHT);
			break;
		}
	}

	_display->reset(_simulator->getRenderData());

	SET_PROPERTY_B(SIMULATION_INITIALIZED, true);
}

void Application::input_handler() {
	if (Input::Get()->isKeyDown(Window::Key::LeftControl) && Input::Get()->isKeyPressed(Window::Key::R))
		SET_PROPERTY_B(SIMULATION_REBUILD, true);
    if(Input::Get()->isKeyPressed(Window::Key::R))
    	SET_PROPERTY_B(SIMULATION_RESET, true);
    if (Input::Get()->isKeyPressed(Window::Key::P) || Input::Get()->isKeyPressed(Window::Key::Space))
    	SET_PROPERTY_B(PAUSED, !PROPERTY_B(PAUSED));
    if (Input::Get()->isKeyPressed(Window::Key::G))
    	SET_PROPERTY_B(GRAVITY_ENABLED, !PROPERTY_B(GRAVITY_ENABLED));
	if (Input::Get()->isKeyDown(Window::Key::LeftControl) && Input::Get()->isKeyPressed(Window::Key::S))
		ADD_PROPERTY_I(SIMULATION_STEPS, 1);
	if (Input::Get()->isKeyPressed(Window::Key::F12) && _display) {
		for (const auto& [name, v] : _display->viewables()) {
			if (name.find("FieldViewer") != std::string::npos)
				v->setVisible(true);
		}
	}
}
