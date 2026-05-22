#include "views/FieldViewerView.h"

#include <algorithm>

#include "imgui.h"
#include "renderer/Renderer.h"

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
    //ImVec2 topCorner = ImGui::GetWindowPos();
    ImGui::SetCursorScreenPos(ImVec2(x_min, y_min));
    ImGui::Image(_txt->get(),
                 ImVec2(x_max - x_min, y_max - y_min),
                 ImVec2(0, 1),
                 ImVec2(1, 0));

    ImGui::SetCursorPos(topCorner);
    ImGui::Text("Max: %f", _max);

    ImGui::End();
}

void FieldViewerView::render_vec_field() {
    std::vector<float> data(_v_field->size() * 4);
    Renderer::drawField(data, *_v_field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

void FieldViewerView::render_scal_field() {
    std::vector<float> data(_s_field->size() * 4);
    Renderer::drawField(data, *_s_field, 2, _max);
    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}
