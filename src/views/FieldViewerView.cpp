#include "views/FieldViewerView.h"

#include <algorithm>

#include "imgui.h"
#include "renderer/Renderer2D.h"

FieldViewerView::FieldViewerView(VecField *field, const std::string& name)
    : _name(name), _v_field(field) {
    _txt = new Texture2D(field->width(), field->height(), nullptr);
}

FieldViewerView::FieldViewerView(ScalField *field, const std::string& name)
    : _name(name), _s_field(field) {
    _txt = new Texture2D(field->width(), field->height(), nullptr);
}

FieldViewerView::~FieldViewerView() {
    delete _txt;
}

void DrawFieldGrid(ImDrawList* dl, ImVec2 origin, float cellSize, ScalField& field)
{
    for (int y = 0; y < 25; y++)
    {
        for (int x = 0; x < 25; x++)
        {
            float v = field(x, 24 - y);
            v = std::clamp(v, 0.0f, 1.0f);

            ImU32 col = IM_COL32(
                (int)(v * 255),
                (int)(v * 255),
                (int)(v * 255),
                255
            );

            if (x == 0 && 24 - y == 0) {
                col = IM_COL32(
                (int)(200),
                (int)(0),
                (int)(205),
                255
            );
            }

            ImVec2 p0(
                origin.x + x * cellSize,
                origin.y + y * cellSize
            );

            ImVec2 p1(
                p0.x + cellSize,
                p0.y + cellSize
            );

            dl->AddRectFilled(p0, p1, col);
        }
    }
}

void FieldViewerView::render() {
    if (!_v_field && !_s_field)
        return;

    _max = 0;

    if (_v_field)
        render_vec_field();
    if (_s_field)
        render_scal_field();

    ImGui::Begin(_name.c_str());

    if (_s_field && _s_field->size() == 25 * 25) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImVec2 origin = ImGui::GetCursorScreenPos();

        // pick integer cell size for perfect pixel look
        float cellSize = 50.0f;

        DrawFieldGrid(dl, origin, cellSize, *_s_field);

        // advance cursor so ImGui layout doesn't overlap
        ImGui::Dummy(ImVec2(25 * cellSize, 25 * cellSize));
        ImGui::End();
        return;
    }

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

    // ------------------------------------------------------------------
    // Hover detection
    // ------------------------------------------------------------------

    const float draw_w = x_max - x_min;
    const float draw_h = y_max - y_min;

    const int grid_w = field_width();
    const int grid_h = field_height();

    ImVec2 mouse = ImGui::GetMousePos();

    if(mouse.x >= x_min && mouse.x < x_max &&
       mouse.y >= y_min && mouse.y < y_max)
    {
        const float rel_x = (mouse.x - x_min) / draw_w;
        const float rel_y = (mouse.y - y_min) / draw_h;

        int cell_x = (int)(rel_x * grid_w);
        int cell_y = (int)((1.0f - rel_y) * grid_h); // image is vertically flipped

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
        if (_s_field) ImGui::Text("value: %.6f", (*_s_field)(cell_x, cell_y));
        if (_v_field) ImGui::Text("value: (%.6f, %.6f)", (*_v_field)(cell_x, cell_y).x, (*_v_field)(cell_x, cell_y).y );
        ImGui::EndTooltip();
    }

    ImGui::SetCursorPos(topCorner);
    ImGui::Text("Max: %f", _max);

    ImGui::End();
}

void FieldViewerView::render_vec_field() {
    std::vector<float> data(_v_field->size() * 4);
    Renderer2D::drawField(data, *_v_field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

void FieldViewerView::render_scal_field() {
    std::vector<float> data(_s_field->size() * 4);
    Renderer2D::drawField(data, *_s_field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

unsigned int FieldViewerView::field_width() const {
    if (_v_field) return _v_field->width();
    if (_s_field) return _s_field->width();
    return 0;
}

unsigned int FieldViewerView::field_height() const {
    if (_v_field) return _v_field->height();
    if (_s_field) return _s_field->height();
    return 0;
}
