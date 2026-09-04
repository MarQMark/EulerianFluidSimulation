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

    float max = 0;
    for (int i = 0; i < MAX_STEPS; i++)
        max = std::max(max, _steps[i]);

    ImGui::PushStyleColor(ImGuiCol_PlotLines,        color);
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
    ImGui::PlotLines("##SolverStepsPlot",
                     _steps, MAX_STEPS, _steps_offset,
                     nullptr, 0.0f,
                     max,
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

    const auto solverErrors = PROPERTY(SOLVE_ERRORS, std::vector<float>*);
    if (solverErrors && !solverErrors->empty()) {
        float max = 0;
        float min = FLT_MAX;
        int maxIt = 0, minIt = 0;
        for (int i = 0; i < solverErrors->size(); i++) {
            const auto err = (*solverErrors)[i];
            if (max < err) {
                max = err;
                maxIt = i;
            }
            if (min > err) {
                min = err;
                minIt = i;
            }
        }

        Util::SectionHeader("SOLVER ERROR");

        ImGui::PushStyleColor(ImGuiCol_PlotLines,        color);
        ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
        ImGui::PlotLines("##SolverErrorPlot",
                         solverErrors->data(), solverErrors->size(), 0,
                         nullptr, 0.0f,
                         max,
                         ImVec2(-FLT_MIN, 80));
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        ImGui::Text("Max [%d]: %f\nMin [%d]: %f", maxIt, max, minIt, min);
    }

    ImGui::End();
}
