#include "views/Simulation3DView.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "core/Editor.h"
#include "core/Input.h"
#include "glm/gtc/type_ptr.hpp"
#include "renderer/Renderer3D.h"
#include "simulation/Properties.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "util/Time.h"

Simulation3DView::Simulation3DView(Editor* editor, Renderer3D* renderer)
    : editor(editor), renderer(renderer)
{ }

void Simulation3DView::render(void* renderData) {
    if (!_visible)
        return;

    const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(renderData);
    if (!renderData || data->dim != 3 || PROPERTY(SIMULATION_TYPE, Properties::SimulatorType) <= Properties::SimulatorType::END_2D_SIMULATIONS)
        return;

    ImGui::Begin("Simulation 3D View");

    ImVec2 size = ImGui::GetContentRegionAvail();

    const float renderScale = PROPERTY_F(VP_SCALE);
    const bool renderLock = PROPERTY_B(VP_LOCK);

    float bottomCursorPosY;
    if (renderLock) {
        constexpr float view_padding = 10;
        float x_min, x_max, y_min, y_max;
        const float win_x = ImGui::GetWindowPos().x;
        const float win_y = ImGui::GetWindowPos().y + 10;
        const float win_w = ImGui::GetWindowWidth();
        const float win_h = ImGui::GetWindowHeight() - 10;
        const float txtAspect = (float)PROPERTY_I(VP_WIDTH) / (float)PROPERTY_I(VP_HEIGHT);
        const float winAspect = win_w / win_h;

        if(winAspect > txtAspect){
            y_min = win_y + view_padding;
            y_max = win_y + win_h - view_padding;
            x_min = win_x + (win_w - (y_max - y_min) * txtAspect) / 2.f;
            x_max = win_x + (win_w + (y_max - y_min) * txtAspect) / 2.f;
        }
        else {
            x_min = win_x + view_padding;
            x_max = win_x + win_w - view_padding;
            y_min = win_y + (win_h - (x_max - x_min) / txtAspect) / 2.f;
            y_max = win_y + (win_h + (x_max - x_min) / txtAspect) / 2.f;
        }

        ImVec2 topCorner = ImGui::GetCursorPos();
        //ImVec2 topCorner = ImGui::GetWindowPos();

        ImGui::SetCursorScreenPos(ImVec2(x_min, y_min));
        ImGui::Image(renderer->getTextureId(),
                     ImVec2(x_max - x_min, y_max - y_min),
                     ImVec2(0, 1),
                     ImVec2(1, 0));

        bottomCursorPosY = ImGui::GetCursorPosY() - 15;
    }
    else {
        ImGui::Image(
            (void *)static_cast<intptr_t>(renderer->getTextureId()),
            ImVec2(viewportSize.x / renderScale , viewportSize.y / renderScale),
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        bottomCursorPosY = viewportSize.y / renderScale;
    }

    if (auto camera = editor->getScene()->getCamera())
        camera->setHover(ImGui::IsWindowHovered());

    // Check if resized
    if (renderLock) {
        size = { PROPERTY_I(VP_WIDTH) * renderScale, PROPERTY_I(VP_HEIGHT) * renderScale };
    }
    if (viewportSize.x != (size.x * renderScale) || viewportSize.y != (size.y * renderScale)) {
        viewportSize = { size.x * renderScale, size.y * renderScale };

        renderer->setSize(viewportSize.x, viewportSize.y);
        editor->updateRenderingSize(viewportSize.x, viewportSize.y);
        editor->getScene()->getCamera()->setAspectRatio(viewportSize.x / viewportSize.y);

        ImGui::End();
        return;
    }

    ImVec2 viewportPos = ImGui::GetItemRectMin();
    ImVec2 viewportSize = ImGui::GetItemRectSize();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    if (auto selected = editor->getSelected(); selected && selected->showGizmo()) {
        static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;

        if (Input::Get()->isKeyPressed(Window::Key::F))
            operation = ImGuizmo::TRANSLATE;

        if (Input::Get()->isKeyPressed(Window::Key::G))
            operation = ImGuizmo::ROTATE;

        if (Input::Get()->isKeyPressed(Window::Key::H))
            operation = ImGuizmo::SCALE;

        if (Input::Get()->isKeyPressed(Window::Key::J))
            mode = ImGuizmo::MODE::WORLD;

        if (Input::Get()->isKeyPressed(Window::Key::K))
            mode = ImGuizmo::MODE::LOCAL;

        ImGui::SetNextWindowPos({ viewportPos.x + 5, viewportPos.y + 5 });
        ImGui::BeginChild("##gizmoOverlay", ImVec2(0, 0));

        auto highlight = [](bool active) {
            return active ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f)   // yellow = active
                          : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // grey = inactive
        };

        ImGui::TextColored(highlight(operation == ImGuizmo::TRANSLATE), "[F] Translate");
        ImGui::SameLine();
        ImGui::TextColored(highlight(operation == ImGuizmo::ROTATE),    "[G] Rotate");
        ImGui::SameLine();
        ImGui::TextColored(highlight(operation == ImGuizmo::SCALE),     "[H] Scale");

        ImGui::Spacing();

        ImGui::TextColored(highlight(mode == ImGuizmo::WORLD), "[J] World");
        ImGui::SameLine();
        ImGui::TextColored(highlight(mode == ImGuizmo::LOCAL), "[K] Local");

        ImGui::EndChild();

        if (auto transform = dynamic_cast<Transform*>(selected)) {
            auto matrix = transform->getTransform();

            if (ImGuizmo::Manipulate(
                glm::value_ptr(editor->getScene()->getCamera()->getViewMatrix()),
                glm::value_ptr(editor->getScene()->getCamera()->getProjectionMatrix()),
                operation,
                mode,
                glm::value_ptr(matrix)
            )) {
                transform->setTransform(matrix);
            }
        }
    }

    if (PROPERTY_B(PAUSED) && (int)(Time::Get()->tt / 500) % 2 == 0) {
        ImGui::SetCursorPos({ 20, bottomCursorPosY - 5 });
        ImGui::Text("PAUSED");
    }

    ImGui::End();
}
