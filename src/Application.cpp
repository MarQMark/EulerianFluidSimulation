#include "Application.h"

#include "core/InputManager.h"
#include "simulation/Properties.h"
#include "simulator/CPUSimulator2D.h"
#include "simulator/CPUSimulator3D.h"
#include "util/Time.h"
#include "views/DistributionView.h"
#include "views/FPSView.h"
#include "views/ProfilerView.h"
#include "views/PropertiesView.h"
#include "views/SolverStepsView.h"
#include "views/StatsView.h"

Application::Application() {
    _window = new Window(1280, 720, "Eulerian Fluid Simulation");
	Input.addInputWindow(_window);

    _views = new Views(_window);
    _views->addView("Properties", new PropertiesView());
    _views->addView("Performance", new FPSView());
    _views->addView("Stats", new StatsView());
    _views->addView("Distribution", new DistributionView());
    _views->addView("Profiler", new ProfilerView());
    _views->addView("SolverSteps", new SolverStepsView());

    //rebuild(CPU_2D);
	rebuild(CPU_3D);

    Time::Get()->targetFps = 165;
}

Application::~Application() {
    delete _simulator;
    delete _display;
    delete _views;
    delete _window;
}

void Application::update() {
    while (!_window->shouldClose()) {
    	Time::Get()->update();

    	if (!PROPERTY_B(PAUSED) || PROPERTY_I(SIMULATION_STEPS) > 0) {
    		_simulator->update();

    		if (PROPERTY_I(SIMULATION_STEPS) > 0)
    			ADD_PROPERTY_I(SIMULATION_STEPS, -1);
    	}

        _display->update(_simulator->getRenderData());
        _views->update();
    	_window->update();

        input_handler();

    	Time::Get()->limitFPS();
    	Input.reset();
    }
}

void Application::rebuild(const SimulatorType type) {
    if (_simulator) delete _simulator;
    if (_display) {
        _views->deleteView(std::string{_display->viewableName()});
        delete _display;
    }

    const int WIDTH = PROPERTY_I(WIDTH);
    const int HEIGHT = PROPERTY_I(HEIGHT);

    switch (type) {
        case OPENGL_2D:
            _display = new SimulationDisplay2D(WIDTH, HEIGHT);
            break;
        case CPU_3D:
    		_simulator = new CPUSimulator3D(WIDTH, HEIGHT, 1);
            _display = new SimulationDisplay3D(_window);
            break;
        case CPU_2D:
        default:
            _simulator = new CPUSimulator2D(WIDTH, HEIGHT);
            _display = new SimulationDisplay2D(WIDTH, HEIGHT);
            break;
    }

    _views->addView(std::string{_display->viewableName()}, _display->createViewable());
}

void Application::input_handler() {
    const bool searching = _views->getView<PropertiesView>("Properties")->searching();

    if(ImGui::IsKeyPressed(ImGuiKey_R) && !searching)
    	_should_rebuild = true;
    if (ImGui::IsKeyPressed(ImGuiKey_P) && !searching)
    	SET_PROPERTY_B(PAUSED, !PROPERTY_B(PAUSED));
    if (ImGui::IsKeyPressed(ImGuiKey_S) && !searching)
    	ADD_PROPERTY_I(SIMULATION_STEPS, 1);
    if (ImGui::IsKeyPressed(ImGuiKey_G) && !searching)
    	SET_PROPERTY_B(GRAVITY_ENABLED, !PROPERTY_B(GRAVITY_ENABLED));

    /*if (ImGui::IsKeyPressed(ImGuiKey_F1) && !searching) {
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
    }*/

    if (_should_rebuild) {
    	_should_rebuild = false;
    	rebuild(CPU_2D);
    }
}
