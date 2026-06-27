#include "views/SolverStepsView.h"

#include "imgui.h"
#include "simulation/Properties.h"
#include "util/GuiHelper.h"

void SolverStepsView::render(void* renderData) {
    if (!_visible) return;

    ImGui::Begin("Solver Steps");

    const int steps = PROPERTY_I(SOLVE_STEPS);
    _steps[_steps_offset] = static_cast<float>(steps);
    _steps_offset = (_steps_offset + 1) % MAX_STEPS;

    constexpr auto color = ImVec4{0.22f, 0.78f, 0.50f, 1.00f};

    Util::SectionHeader("SOLVER STEPS");

    ImGui::PushStyleColor(ImGuiCol_PlotLines,        color);
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
    ImGui::PlotLines("##SolverStepsPlot",
                     _steps, MAX_STEPS, _steps_offset,
                     nullptr, 0.0f,
                     PROPERTY_I(MAX_STEPS),
                     ImVec2(-FLT_MIN, 80));
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.44f, 0.46f, 0.53f, 1.00f});
    ImGui::TextUnformatted("Last solve: ");
    ImGui::PopStyleColor();

    ImGui::SameLine(115.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%u Steps", steps);
    ImGui::PopStyleColor();

    ImGui::End();
}
