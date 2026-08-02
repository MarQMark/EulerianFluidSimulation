#include "views/Views.h"
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <ranges>
#include "util/GuiHelper.h"
#include <GLFW/glfw3.h>

#include "ImGuizmo.h"
#include "implot.h"
#include "core/Input.h"
#include "simulation/Properties.h"

Views::Views(const Window* window) {
    ImGui::CreateContext();
    ImPlot::CreateContext();
    Util::ApplyTheme();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

Views::~Views() {
    for (const auto view: _views | std::views::values)
        delete view;
}

void Views::update(void* renderData) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    window_flags |= ImGuiWindowFlags_MenuBar;

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);


    render_menubar();

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    Input::Get()->setInputBlocked(io.WantTextInput);

    for (const auto view: _views | std::views::values)
        view->render(renderData);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();

        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();

        glfwMakeContextCurrent(backup);
    }
}

void Views::addView(const std::string &name, Viewable *view) {
    if(_views.contains(name))
        std::cout << "[Warning] View with name " << name << " overrides existing view \n";

    _views[name] = view;
}

Viewable* Views::getView(const std::string &name) {
    if(!_views.contains(name)) {
        //std::cout << "[Warning] View with name " << name << " not in views map\n";
        return nullptr;
    }

    return _views[name];
}

void Views::replaceView(const std::string &name, Viewable *view) {
    if(_views.contains(name))
        delete _views[name];

    _views[name] = view;
}

void Views::deleteView(const std::string &name) {
    if(!_views.contains(name))
        return;

    delete _views[name];
    _views.erase(name);
}

void Views::render_menubar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) {}
            if (ImGui::MenuItem("Save")) {}

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                exit(0);
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Simulation")) {
            if (ImGui::BeginMenu("Change type")) {
                if (ImGui::MenuItem("CPU 2D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::CPU_2D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }
                if (ImGui::MenuItem("CPU 3D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::CPU_3D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }

                if (ImGui::MenuItem("OpenGL 2D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::OPENGL_2D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }
                if (ImGui::MenuItem("OpenGL 3D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::OPENGL_3D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }
#ifdef HAS_CUDA
                const bool cudaEnabled = PROPERTY_B(CUDA_ENABLED);
                if (!cudaEnabled) ImGui::BeginDisabled();
                if (ImGui::MenuItem("Cuda 2D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::CUDA_2D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }
                if (ImGui::MenuItem("Cuda 3D")) {
                    SET_PROPERTY(SIMULATION_TYPE, Properties::CUDA_3D, Properties::SimulatorType);
                    SET_PROPERTY_B(SIMULATION_REBUILD, true);
                }
                if (!cudaEnabled) ImGui::EndDisabled();
#endif

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("View")) {
            for (const auto& [name, view] : _views) {
                bool visible = view->isVisible();
                ImGui::MenuItem(name.c_str(), nullptr, &visible);
                view->setVisible(visible);
            }
            ImGui::EndMenu();
        }

        bool warn = false;
        std::string simTitle;
        switch (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType)) {
            case Properties::CPU_2D:
                warn = (PROPERTY_I(WIDTH_2D) * PROPERTY_I(HEIGHT_2D) > PROPERTY_I(WARN_N_CELLS));
                simTitle = "CPU 2D";
                break;
            case Properties::CPU_3D:
                warn = (PROPERTY_I(WIDTH_3D) * PROPERTY_I(HEIGHT_3D) * PROPERTY_I(DEPTH_3D) > PROPERTY_I(WARN_N_CELLS));
                simTitle = "CPU 3D";
                break;
            case Properties::OPENGL_2D:
                simTitle = "OpenGL 2D";
                break;
            case Properties::OPENGL_3D:
                simTitle = "OpenGL 3D";
                break;
#ifdef HAS_CUDA
            case Properties::CUDA_2D:
                simTitle = "Cuda 2D";
                break;
            case Properties::CUDA_3D:
                simTitle = "Cuda 3D";
                break;
#endif
            default:
                simTitle = "invalid";
        }
        const std::string simFrames = std::to_string(PROPERTY_I(SIMULATION_FRAMES));
        const float framesWidth = ImGui::CalcTextSize(simFrames.c_str()).x;

        const float textWidth = ImGui::CalcTextSize(simTitle.c_str()).x;
        const float availWidth = ImGui::GetWindowWidth();

        ImGui::SetCursorPosX(availWidth - (framesWidth + 10) - (textWidth + 15));
        ImGui::TextUnformatted(simFrames.c_str());
        ImGui::Separator();

        ImGui::SetCursorPosX(availWidth - (textWidth + 10));
        if (warn) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::TextUnformatted(simTitle.c_str());
        if (warn) ImGui::PopStyleColor();

        ImGui::EndMenuBar();
    }
}