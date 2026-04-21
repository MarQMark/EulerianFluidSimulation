#include "views/SimulationView.h"

#include <algorithm>

#include "glm/geometric.hpp"
#include "imgui/imgui.h"
#include "simulation/Properties.h"
#include "simulation/VecField.h"

SimulationView::SimulationView(const ImTextureID textureID, const int width, const int height)
    : _texture_id(textureID), _width(width), _height(height) {
    _forces = new VecField(_width,  _height);
}

SimulationView::~SimulationView() {
    delete _forces;
}

#define TOP_SPACING 10;
void SimulationView::render() {
    if (!_visible)
        return;

    ImGui::Begin("View");

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

    ImGui::SetCursorScreenPos(ImVec2(x_min, y_min));
    ImGui::Image(_texture_id,
                 ImVec2(x_max - x_min, y_max - y_min),
                 ImVec2(0, 1),
                 ImVec2(1, 0));


    _forces->clear();
    if (ImGui::IsItemHovered())
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;

        constexpr ImU32 color = IM_COL32(255, 0, 0, 255);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddCircle(mouse, Properties::Get()->BRUSH_SIZE, color, 32, 2.0f);


        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
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
            float radius_tex_x = Properties::Get()->BRUSH_SIZE * (_width  / (x_max - x_min));
            float radius_tex_y = Properties::Get()->BRUSH_SIZE * (_height / (y_max - y_min));

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
                    float dx = x - cx;
                    float dy = y - cy;

                    if (dx*dx + dy*dy <= r2)
                    {
                        (*_forces)(x, (_height - 1 - y)) += force * glm::vec2(1.f, -1);
                    }
                }
            }
        }
    }

    if (_vel_field) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        glm::vec2 gridSpacing((x_max - x_min) / _vel_field->width(), (y_max - y_min) / _vel_field->height());

        float max = 0;
        for(auto v : *_vel_field) {
            max = std::max(max, glm::length(v));
        }

        for (int y = 0; y < _vel_field->height(); y++) {
            for (int x = 0; x < _vel_field->width(); x++) {
                glm::vec2 vel = (*_vel_field)(x, y);

                const int y_flip = _vel_field->width() - y - 1;
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

    ImGui::End();
}

VecField& SimulationView::getForces() const {
    return *_forces;
}

void SimulationView::setVelField(VecField *velField) {
    _vel_field = velField;
}

