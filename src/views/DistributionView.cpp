#include "views/DistributionView.h"

#include <algorithm>

#include "imgui.h"
#include "simulation/Properties.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "util/GuiHelper.h"

void DistributionView::render(void* renderData) {
    if (!renderData)
        return;
    if (!_visible)
        return;
    const auto* data = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
    auto* grid = dynamic_cast<IGrid2D*>(data->grid);
    if (!grid)
        return;

    ImGui::Begin("Rho Distribution");

    if (_y_level == -1)
        _y_level = grid->height() / 2;

    auto& rho = grid->rho();
    _y_level = std::clamp(_y_level, 0, (int)rho.height() - 1);

    Util::SectionHeader("DISTRIBUTION");

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