#include "views/FieldViewerView3D.h"

#include <algorithm>

#include "imgui.h"
#include "renderer/Renderer3D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "simulation/data/Grid3D.h"

FieldViewerView3D::FieldViewerView3D(const Properties::DrawMode type, std::string name)
    : _type(type), _name(std::move(name)){
    _visible = false;
}

FieldViewerView3D::~FieldViewerView3D() {
    delete _txt;
}

void FieldViewerView3D::render(void *renderData) {
    if (!_visible) return;
    if (!renderData) return;
    const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(renderData);
    auto* grid = data->grid;
    if (!grid)
        return;

    _max = 0;

    VecField3D* vField{};
    ScalField3D* sField{};
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
        case Properties::BOUNDARY:
            sField = &grid->boundary();
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
    const float win_h = ImGui::GetWindowHeight() - 10 - 50;
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

    const char* names[] = { "X", "Y", "Z" };
    int currentDim = _dim;
    if (ImGui::Combo("Slice Axis", &currentDim, names, IM_ARRAYSIZE(names))) {
        if (currentDim != _dim) {
            _dim = currentDim;
            _changed_dim = true;
        }
    }
    ImGui::SliderInt("Level", &_level, 0, _sizes[_dim] - 1);

    // Hover detection
    const float draw_w = x_max - x_min;
    const float draw_h = y_max - y_min;

    const int grid_w = _txt->getWidth();
    const int grid_h = _txt->getHeight();

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
        if (_dim == 0) {
            ImGui::Text("y: %d", cell_x);
            ImGui::Text("z: %d", cell_y);
        }
        else if (_dim == 1) {
            ImGui::Text("x: %d", cell_y);
            ImGui::Text("z: %d", cell_x);
        }
        else if (_dim == 2) {
            ImGui::Text("x: %d", cell_x);
            ImGui::Text("y: %d", cell_y);
        }

        if (sField) {
            float v = 0.f;
            if (_dim == 0) v = (*sField)(_level, cell_x, cell_y);
            else if (_dim == 1) v = (*sField)(cell_y, _level, cell_x);
            else if (_dim == 2) v = (*sField)(cell_x, cell_y, _level);
            ImGui::Text("value: %.6f", v);
        }
        if (vField) {
            auto v = glm::vec3(0.f);
            if (_dim == 0) v = (*vField)(_level, cell_x, cell_y);
            else if (_dim == 1) v = (*vField)(cell_y, _level, cell_x);
            else if (_dim == 2) v = (*vField)(cell_x, cell_y, _level);
            ImGui::Text("value: (%.6f, %.6f, %.6f)", v.x, v.y, v.z);
        }
        ImGui::EndTooltip();
    }

    ImGui::SetCursorPos(topCorner);
    ImGui::Text("Max: %f", _max);

    ImGui::End();
}

void FieldViewerView3D::render_vec_field(VecField3D &field) {
    _sizes[0] = field.width();
    _sizes[1] = field.height();
    _sizes[2] = field.depth();
    const unsigned int usedSizes[2] = {_sizes[(_dim + 1) % 3], _sizes[(_dim + 2) % 3]};
    if (_used_sizes[0] != usedSizes[0] || _used_sizes[1] != usedSizes[1] || _changed_dim) {
        _used_sizes[0] = usedSizes[0];
        _used_sizes[1] = usedSizes[1];
        delete _txt;
        _txt = new Texture2D(_used_sizes[0], _used_sizes[1], nullptr);
        _changed_dim = false;
    }
    if (!_txt) {
        _txt = new Texture2D(_used_sizes[0], _used_sizes[1], nullptr);
    }

    std::vector<float> data(field.size() * 4);
    Renderer3D::drawField(data, field, _dim, _level, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

void FieldViewerView3D::render_scal_field(ScalField3D &field) {
    _sizes[0] = field.width();
    _sizes[1] = field.height();
    _sizes[2] = field.depth();
    const unsigned int usedSizes[2] = {_sizes[(_dim + 1) % 3], _sizes[(_dim + 2) % 3]};
    if (_used_sizes[0] != usedSizes[0] || _used_sizes[1] != usedSizes[1] || _changed_dim) {
        _used_sizes[0] = usedSizes[0];
        _used_sizes[1] = usedSizes[1];
        delete _txt;
        _txt = new Texture2D(_used_sizes[0], _used_sizes[1], nullptr);
        _changed_dim = false;
    }
    if (!_txt) {
        _txt = new Texture2D(_used_sizes[0], _used_sizes[1], nullptr);
    }

    std::vector<float> data(field.size() * 4);
    Renderer3D::drawField(data, field, _dim, _level,2,_max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}
