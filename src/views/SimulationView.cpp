#include "views/SimulationView.h"

#include <algorithm>

#include "imgui/imgui.h"

SimulationView::SimulationView(const ImTextureID textureID, const int width, const int height)
    : _texture_id(textureID), _width(width), _height(height) {
    _forces.resize(_width * _height);
    for(auto& f : _forces)
        f = glm::vec2(0);
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

    if (ImGui::IsItemHovered())
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;

        constexpr ImU32 color = IM_COL32(255, 0, 0, 255);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddCircle(mouse, _size, color, 32, 2.0f);


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
            float radius_tex_x = _size * (_width  / (x_max - x_min));
            float radius_tex_y = _size * (_height / (y_max - y_min));

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
                        _forces[(_height - 1 - y) * _width + x] += force * 1.f;
                    }
                }
            }
        }
        else {
            for (auto& f : _forces)
                f = glm::vec2(0);
        }
    }

    /*constexpr float decay = -0.1;
    for(auto& f : _forces) {
        if(f.x > 0) f.x += decay;
        else if(f.x < 0) f.x -= decay;

        if(f.y > 0) f.y += decay;
        else if(f.y < 0) f.y -= decay;

        if(std::abs(f.x) < 0.2) f.x = 0;
        if(std::abs(f.y) < 0.2) f.y = 0;
    }*/

    ImGui::End();
}

std::vector<glm::vec2> & SimulationView::getForces() {
    return _forces;
}

