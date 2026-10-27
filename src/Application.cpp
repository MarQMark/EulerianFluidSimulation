#include "Application.h"

#include <ranges>
#include "core/Input.h"
#include "simulation/Properties.h"
#include "simulation/simulator/CPUSimulator2D.h"
#include "simulation/simulator/CPUSimulator3D.h"
#include "simulation/simulator/OpenGLSimulator2D.h"
#include "simulation/simulator/OpenGLSimulator3D.h"
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
#include "simulation/simulator/CudaSimulator3D.h"
#endif

#ifdef HAS_FFMPEG
#include "views/RecordingView.h"
#endif //HAS_FFMPEG

Application::Application() {
    _window = new Window(1920, 1080, "Eulerian Fluid Simulation");
	Input::Get()->addInputWindow(_window);

    _views = new Views(_window);
    _views->addView("Properties", new PropertiesView());
    _views->addView("Performance", new FPSView());
    _views->addView("Profiler", new ProfilerView());
    _views->addView("SolverSteps", new SolverStepsView());
	_views->addView("Stats", new StatsView());
#ifdef HAS_FFMPEG
	_views->addView("Recording", new RecordingView());
#endif //HAS_FFMPEG

	rebuild();

    Time::Get()->targetFps = 165;

#ifdef HAS_CUDA
	int count = 0;
	const cudaError_t err = cudaGetDeviceCount(&count);

	if (err != cudaSuccess) {
		LOG_ERROR(cudaGetErrorString(err));
		SET_PROPERTY_B(CUDA_ENABLED, false);
	}
	else if (count < 1) {
		LOG_WARN("No Cuda capable devices found");
		SET_PROPERTY_B(CUDA_ENABLED, false);
	}
	else {
		LOG_INFO("Cuda Available");
		SET_PROPERTY_B(CUDA_ENABLED, true);
		SET_PROPERTY(SIMULATION_TYPE, Properties::SimulatorType::CUDA_3D, Properties::SimulatorType);
	}
#endif
}

Application::~Application() {
    delete _simulator;
    delete _display;
    delete _views;
    delete _window;
}

void Application::update() {
	Profiler* profiler = Profiler::Get();

    while (!_window->shouldClose()) {
    	profiler->start("Frame-time");

    	PROFILE("Process Events",
    		_window->processEvents();
    		Time::Get()->update();
    	)

    	PROFILE("Display Update",
    		_display->update(_simulator->getRenderData());
    	)

    	PROFILE("Simulation",
    		SET_PROPERTY_F(GRAVITY_FORCE, PROPERTY_F(GRAVITY_FORCE_PHYSICAL) / PROPERTY_F(CELL_SIZE));

    		if ((!PROPERTY_B(PAUSED) || PROPERTY_I(SIMULATION_STEPS) > 0) && PROPERTY_B(SIMULATION_INITIALIZED)) {
    			_simulator->update();

    			if (PROPERTY_I(SIMULATION_STEPS) > 0)
    				ADD_PROPERTY_I(SIMULATION_STEPS, -1);
    		}
    	)

    	PROFILE("Display Render",
			_display->render(_simulator->getRenderData());
    	)

    	PROFILE("Views Update",
			_views->update(_simulator->getRenderData());
    	)

    	PROFILE("Window Swap Buffers",
    		_window->swapBuffers();
		)

    	PROFILE("Loop End",
			input_handler();

    		Time::Get()->limitFPS();
    		Input::Get()->update();
    	)

    	if (PROPERTY_B(SIMULATION_REBUILD))
    		rebuild();
    	if (PROPERTY_B(SIMULATION_RESET))
			reset();

    	profiler->stop("Frame-time");
    }
}

void Application::rebuild() {
	SET_PROPERTY_B(SIMULATION_REBUILD, false);
	SET_PROPERTY_I(SIMULATION_FRAMES, 0);

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
    	case Properties::OPENGL_3D:
        {
        	_display = new SimulationDisplay3D(_window);

        	const int WIDTH	 = PROPERTY_I(WIDTH_3D);
        	const int HEIGHT = PROPERTY_I(HEIGHT_3D);
        	const int DEPTH	 = PROPERTY_I(DEPTH_3D);

        	_simulator = new OpenGLSimulator3D(WIDTH, HEIGHT, DEPTH);
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

        	_simulator = new CudaSimulator3D(WIDTH, HEIGHT, DEPTH);
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
	SET_PROPERTY_I(SIMULATION_FRAMES, 0);

	if (_simulator) delete _simulator;

	switch (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType)) {
		case Properties::OPENGL_2D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_2D);
			const int HEIGHT = PROPERTY_I(HEIGHT_2D);

			_simulator = new OpenGLSimulator2D(WIDTH, HEIGHT);
			break;
		}
		case Properties::OPENGL_3D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_3D);
			const int HEIGHT = PROPERTY_I(HEIGHT_3D);
			const int DEPTH	 = PROPERTY_I(DEPTH_3D);

			_simulator = new OpenGLSimulator3D(WIDTH, HEIGHT, DEPTH);
			break;
		}
		case Properties::CPU_3D:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_3D);
			const int HEIGHT = PROPERTY_I(HEIGHT_3D);
			const int DEPTH	 = PROPERTY_I(DEPTH_3D);

			_simulator = new CPUSimulator3D(WIDTH, HEIGHT, DEPTH);
			if (WIDTH * HEIGHT * DEPTH > PROPERTY_I(WARN_N_CELLS))
				LOG_WARN("CPU Simulation with high amount of cells")
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

			_simulator = new CudaSimulator3D(WIDTH, HEIGHT, DEPTH);
			break;
		}
#endif
		case Properties::CPU_2D:
		default:
		{
			const int WIDTH	 = PROPERTY_I(WIDTH_2D);
			const int HEIGHT = PROPERTY_I(HEIGHT_2D);

			_simulator = new CPUSimulator2D(WIDTH, HEIGHT);
			if (WIDTH * HEIGHT > PROPERTY_I(WARN_N_CELLS))
				LOG_WARN("CPU Simulation with high amount of cells")
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
	if (Input::Get()->isKeyDown(Window::Key::LeftControl) && Input::Get()->isKeyPressed(Window::Key::S))
		ADD_PROPERTY_I(SIMULATION_STEPS, 1);
	if (Input::Get()->isKeyPressed(Window::Key::F12) && _display) {
		for (const auto& [name, v] : _display->viewables()) {
			if (name.find("FieldViewer") != std::string::npos)
				v->setVisible(true);
		}
	}
}
