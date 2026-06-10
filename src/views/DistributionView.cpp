#include "views/DistributionView.h"

#include <algorithm>

#include "imgui.h"
#include "simulation/Properties.h"
#include "util/GuiHelper.h"

void DistributionView::render() {
    if (!_visible)
        return;

    ImGui::Begin("Rho Distribution");
    if (!_grid) {
        ImGui::End();
        return;
    }

    auto& rho = _grid->rho();
    _y_level = std::clamp(_y_level, 0, (int)rho.height() - 1);

    Util::SectionHeader("PERFORMANCE");

    ImGui::PushStyleColor(ImGuiCol_PlotLines,        ImVec4{0.22f, 0.78f, 0.50f, 1.00f});
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4{0.22f, 0.78f, 0.50f, 1.00f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
    ImGui::PlotLines("##FpsPlot",
                     &rho.data()[_y_level * rho.width()],
                     rho.width(), 0,
                     nullptr, 0.0f,
                     1.1f,
                     ImVec2(-FLT_MIN, 80));
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::SliderInt("Y Level", &_y_level, 0, rho.height() - 1);

    ImGui::End();
}

void DistributionView::recreate(Grid *grid) {
    _grid = grid;
    _y_level = _grid->height() / 2;
}
