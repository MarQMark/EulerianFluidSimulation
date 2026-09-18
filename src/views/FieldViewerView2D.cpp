#include "views/FieldViewerView2D.h"

#include <algorithm>
#include <utility>

#include "imgui.h"
#include "renderer/Renderer2D.h"
#include "simulation/display/SimulationDisplay2D.h"

FieldViewerView2D::FieldViewerView2D(const Properties::DrawMode type, std::string name)
    : _type(type), _name(std::move(name)){
    _visible = false;
}

FieldViewerView2D::~FieldViewerView2D() {
    delete _txt;
}

void FieldViewerView2D::render(void* renderData) {
    if (!_visible) return;
    if (!renderData) return;
    const auto* data = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
    auto* grid = data->grid;
    if (!grid)
        return;

    if (!_txt)
        _txt = new Texture2D(grid->width(), grid->height(), nullptr);

    _max = 0;

    VecField2D* vField{};
    ScalField2D* sField{};
    switch (_type) {
        case Properties::VELOCITY:
            vField = &grid->vel();
            break;
        case Properties::FLUID:
        case Properties::DENSITY:
            sField = &grid->rho();
            break;
        case Properties::DIVERGENCE:
            sField = &grid->divergence();
            break;
        case Properties::PRESSURE:
            sField = &grid->pressure();
            break;
        case Properties::GAMMA:
            sField = &grid->gamma();
            break;
        case Properties::BETA:
            sField = &grid->beta();
            break;
        default:;
    }

    if (vField) render_vec_field(*vField);
    if (sField) render_scal_field(*sField);

    ImGui::Begin(_name.c_str());

    constexpr float view_padding = 10;
    float x_min, x_max, y_min, y_max;
    const float win_x = ImGui::GetWindowPos().x;
    const float win_y = ImGui::GetWindowPos().y + 10;
    const float win_w = ImGui::GetWindowWidth();
    const float win_h = ImGui::GetWindowHeight() - 10;
    const float txtAspect = (float)_txt->getWidth() / (float)_txt->getHeight();
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
    ImGui::SetCursorScreenPos(ImVec2(x_min, y_min));
    ImGui::Image(_txt->get(),
                 ImVec2(x_max - x_min, y_max - y_min),
                 ImVec2(0, 1),
                 ImVec2(1, 0));


    // Hover detection
    const float draw_w = x_max - x_min;
    const float draw_h = y_max - y_min;

    const int grid_w = grid->width();
    const int grid_h = grid->height();

    ImVec2 mouse = ImGui::GetMousePos();

    if(mouse.x >= x_min && mouse.x < x_max &&
       mouse.y >= y_min && mouse.y < y_max)
    {
        const float rel_x = (mouse.x - x_min) / draw_w;
        const float rel_y = (mouse.y - y_min) / draw_h;

        int cell_x = (int)(rel_x * grid_w);
        int cell_y = (int)((1.0f - rel_y) * grid_h);

        cell_x = std::clamp(cell_x, 0, grid_w - 1);
        cell_y = std::clamp(cell_y, 0, grid_h - 1);

        const float cell_w = draw_w / grid_w;
        const float cell_h = draw_h / grid_h;

        ImVec2 p0(
            x_min + cell_x * cell_w,
            y_min + (grid_h - 1 - cell_y) * cell_h
        );

        ImVec2 p1(
            p0.x + cell_w,
            p0.y + cell_h
        );

        ImGui::GetWindowDrawList()->AddRect(
            p0,
            p1,
            IM_COL32(255, 0, 0, 255),
            0.0f,
            0,
            2.0f
        );

        ImGui::BeginTooltip();
        ImGui::Text("x: %d", cell_x);
        ImGui::Text("y: %d", cell_y);
        if (sField) ImGui::Text("value: %.6f", (*sField)(cell_x, cell_y));
        if (vField) ImGui::Text("value: (%.6f, %.6f)", (*vField)(cell_x, cell_y).x, (*vField)(cell_x, cell_y).y );
        ImGui::EndTooltip();
    }

    ImGui::SetCursorPos(topCorner);
    ImGui::Text("Max: %f", _max);

    ImGui::End();
}

void FieldViewerView2D::render_vec_field(VecField2D& field) {
    std::vector<float> data(field.size() * 4);
    Renderer2D::drawField(data, field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

void FieldViewerView2D::render_scal_field(ScalField2D& field) {
    std::vector<float> data(field.size() * 4);
    Renderer2D::drawField(data, field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}