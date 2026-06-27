#include "views/Views.h"
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <ranges>
#include "util/GuiHelper.h"
#include <GLFW/glfw3.h>

#include "simulation/Properties.h"

Views::Views(const Window* window) {
    ImGui::CreateContext();
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

void DrawTaskbar()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float height = 32.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Taskbar", nullptr, flags);

    if (ImGui::Button("Open")) {
        // ...
    }

    ImGui::SameLine();

    if (ImGui::Button("Save")) {
        // ...
    }

    ImGui::SameLine();

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
}

void Views::update(void* renderData) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

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

        ImGui::EndMenuBar();
    }
}