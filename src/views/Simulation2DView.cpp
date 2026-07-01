#include "views/Simulation2DView.h"

#include <algorithm>

#include "glm/geometric.hpp"
#include "imgui/imgui.h"
#include "simulation/Grid2D.h"
#include "simulation/Properties.h"
#include "simulation/SimulationDisplay2D.h"
#include "simulation/VecField2D.h"
#include "util/Time.h"

#define TOP_SPACING 10;

Simulation2DView::Simulation2DView(const ImTextureID textureID, const int width, const int height)
        : _texture_id(textureID), _width(width), _height(height) {
}

void Simulation2DView::render(void* renderData) {
    if (!renderData)
        return;
    if (!_visible)
        return;
    const auto* data = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
    const auto* grid = data->grid;
    auto& forces = grid->forces();
    auto& boundary = grid->boundary();
    auto& rho = grid->rho();

    ImGui::Begin("Simulation 2D View");

    if(ImGui::IsKeyPressed(ImGuiKey_M)) {
        _mode = (Mode)((_mode + 1) % END);
    }

    constexpr float view_padding = 10;
    float x_min, x_max, y_min, y_max;
    const float win_x = ImGui::GetWindowPos().x;
    const float win_y = ImGui::GetWindowPos().y + TOP_SPACING;
    const float win_w = ImGui::GetWindowWidth();
    const float win_h = ImGui::GetWindowHeight() - TOP_SPACING;
    const float txtAspect = (float)_width / (float)_height;
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
    ImGui::Image(_texture_id,
                 ImVec2(x_max - x_min, y_max - y_min),
                 ImVec2(0, 1),
                 ImVec2(1, 0));


    forces.clear();
    if (ImGui::IsItemHovered()) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;

        const float multi = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 7.f : 2.f;
        ADD_PROPERTY_F(BRUSH_SIZE, ImGui::GetIO().MouseWheel * multi);

        constexpr ImU32 color = IM_COL32(255, 0, 0, 255);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddCircle(mouse, PROPERTY_F(BRUSH_SIZE), color, 32, 2.0f);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            const bool left = ImGui::IsMouseDown(ImGuiMouseButton_Left);

            ImVec2 mouse = ImGui::GetIO().MousePos;
            ImVec2 delta = ImGui::GetIO().MouseDelta; // per-frame drag delta (pixels)

            // Convert circle center to texture space
            float u = (mouse.x - x_min) / (x_max - x_min);
            float v = (mouse.y - y_min) / (y_max - y_min);

            // Clamp (optional, for safety)
            u = std::clamp(u, 0.0f, 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);

            int cx = (int)(u * (_width  - 1));
            int cy = (int)(v * (_height - 1));

            // Convert radius to texture space
            float radius_tex_x = PROPERTY_F(BRUSH_SIZE) * (_width  / (x_max - x_min));
            float radius_tex_y = PROPERTY_F(BRUSH_SIZE) * (_height / (y_max - y_min));

            float r2 = radius_tex_x * radius_tex_x; // use x scale (aspect already preserved)

            // Scale mouse delta into texture space
            glm::vec2 force(
                delta.x * (_width  / (x_max - x_min)),
                delta.y * (_height / (y_max - y_min))
            );

            // Iterate bounding box of the circle
            int xmin = std::max(0, (int)(cx - radius_tex_x));
            int xmax = std::min((int)_width  - 1, (int)(cx + radius_tex_x));
            int ymin = std::max(0, (int)(cy - radius_tex_y));
            int ymax = std::min((int)_height - 1, (int)(cy + radius_tex_y));

            for (int y = ymin; y <= ymax; ++y)
            {
                for (int x = xmin; x <= xmax; ++x)
                {
                    const float dx = x - cx;
                    const float dy = y - cy;

                    const int flipY = _height - 1 - y;
                    if (x == 0 || flipY == 0 || x == _width - 1 || flipY == _height - 1)
                        continue;

                    if (dx*dx + dy*dy <= r2) {
                        switch (_mode) {
                            case FORCE:
                                forces(x, flipY) += force * glm::vec2(1.f, -1);
                                break;
                            case BOUNDARY:
                                if (left) boundary(x, flipY) = 1;
                                else      boundary(x, flipY) = 0;
                                break;
                            case FLUID:
                                if (left) rho(x, flipY) = 1;
                                else      rho(x, flipY) = 0;
                                break;
                            case END:
                                break;
                        }
                    }
                }
            }
        }
    }

    if (_vel_field && PROPERTY_B(DRAW_VEL_ARROWS)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        glm::vec2 gridSpacing((x_max - x_min) / _vel_field->width(), (y_max - y_min) / _vel_field->height());

        float max = 0;
        for(auto v : *_vel_field) {
            max = std::max(max, glm::length(v));
        }

        for (int y = 0; y < _vel_field->height(); y++) {
            for (int x = 0; x < _vel_field->width(); x++) {
                glm::vec2 vel = (*_vel_field)(x, y);

                const int y_flip = _vel_field->height() - y - 1;
                vel = vel / max;
                draw_list->AddLine(
                    ImVec2(
                        x_min + (x + .5f) * gridSpacing.x,
                        y_min + (y_flip + .5f) * gridSpacing.y),
                    ImVec2(
                        x_min + (x + .5f) * gridSpacing.x + vel.x * 10,
                        y_min + (y_flip + .5f) * gridSpacing.y + vel.y * 10),
                    IM_COL32(0, 255, 0, 255),
                    1.5f
                );
            }
        }
    }

    //ImGui::SetCursorScreenPos(ImVec2(topCorner));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 20);
    switch (_mode) {
        case FORCE:
            ImGui::Text("Mode: Force");
            break;
        case BOUNDARY:
            ImGui::Text("Mode: Boundary");
            break;
        case FLUID:
            ImGui::Text("Mode: Fluid");
            break;
        case END:
        default:
            ImGui::Text("Mode: Invalid");
            break;
    }

    if (PROPERTY_B(PAUSED) && (int)(Time::Get()->tt / 500) % 2 == 0) {
        ImGui::SetCursorPos(topCorner);
        ImGui::Text("PAUSED");
    }

    ImGui::End();
}

void Simulation2DView::setVelField(VecField2D *velField) {
    _vel_field = velField;
}

