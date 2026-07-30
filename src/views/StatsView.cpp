#include "views/StatsView.h"

#include <algorithm>

#include "imgui.h"
#include "simulation/Properties.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/display/SimulationDisplay2D.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "util/GuiHelper.h"

void StatsView::render(void* renderData) {
    if (!renderData)
        return;
    if (!_visible)
        return;

    float current = 0;
    float compressed = 0;
    float floating = 0;
    float sum = 0;

    if (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType) <= Properties::SimulatorType::END_2D_SIMULATIONS) {
        const auto* data = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
        auto* grid = dynamic_cast<IGrid2D*>(data->grid);
        if (!grid)
            return;

        for (unsigned int i = 0; i < grid->rho().size(); i++) {
            if (grid->rho()[i] > PROPERTY_F(LIQUID_THRESHOLD))
                current += 1;
            else
                floating += grid->rho()[i];

            if (grid->rho()[i] > 1.f)
                compressed += grid->rho()[i] - 1.f;

            sum += grid->rho()[i];
        }
    }
    else {
        const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(renderData);
        auto* grid = dynamic_cast<IGrid3D*>(data->grid);
        if (!grid)
            return;

        for (unsigned int i = 0; i < grid->rho().size(); i++) {
            if (grid->rho()[i] > PROPERTY_F(LIQUID_THRESHOLD))
                current += 1;
            else
                floating += grid->rho()[i];

            if (grid->rho()[i] > 1.f)
                compressed += grid->rho()[i] - 1.f;

            sum += grid->rho()[i];
        }
    }

    const float total = PROPERTY_F(TOTAL_RHO);


    ImGui::Begin("Stats");

    // ── Record ───────────────────────────────────────────────────────────────
    _times[_times_offset] = current;
    _times_offset = (_times_offset + 1) % MAX_TIMES;

    // ── Header ───────────────────────────────────────────────────────────────
    Util::SectionHeader("PARTICLE DENSITY");

    // ── Plot (teal lines, darkened frame background) ──────────────────────────
    ImGui::PushStyleColor(ImGuiCol_PlotLines,        ImVec4{0.19f, 0.80f, 0.60f, 1.00f});
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4{0.40f, 0.95f, 0.74f, 1.00f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
    ImGui::PlotLines("##RhoPlot",
                     _times, MAX_TIMES, _times_offset,
                     nullptr, 0.0f, total, ImVec2(-FLT_MIN, 80));
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // ── Stat rows ─────────────────────────────────────────────────────────────
    Util::StatRow("In Fluid",    current,    total, ImVec4{0.22f, 0.78f, 0.50f, 1.00f}); // kOK   green
    Util::StatRow("Compressed",  compressed, total, ImVec4{0.31f, 0.56f, 1.00f, 1.00f}); // kACC  blue
    Util::StatRow("Floating",    floating,   total, ImVec4{0.97f, 0.74f, 0.24f, 1.00f}); // kWARN yellow

    // ── Total row (separator + full-brightness text) ──────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    {
        float pct = sum / total;
        ImVec4 totColor = (pct > 0.995f)
                        ? ImVec4{0.22f, 0.78f, 0.50f, 1.00f}   // all accounted for
                        : ImVec4{0.97f, 0.74f, 0.24f, 1.00f};  // some missing

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.60f, 0.62f, 0.68f, 1.00f});
        ImGui::Text("%-12s", "Total");
        ImGui::PopStyleColor();

        ImGui::SameLine(90.0f);
        ImGui::Text("%.0f / %.0f", sum, total);

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, totColor);
        ImGui::Text("%.1f%%", pct * 100.0f);
        ImGui::PopStyleColor();
    }

    ImGui::End();
}