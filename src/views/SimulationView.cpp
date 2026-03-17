#include "views/SimulationView.h"
#include "imgui/imgui.h"

SimulationView::SimulationView(const ImTextureID textureID, const int width, const int height)
    : _texture_id(textureID), _width(width), _height(height) {
}

#define TOP_SPACING 10;
void SimulationView::render() {
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

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddImage(_texture_id,
                       ImVec2(x_min, y_min),
                       ImVec2(x_max, y_max),
                       ImVec2(0, 1),
                       ImVec2(1, 0));

    ImGui::End();
}

