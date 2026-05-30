#include "views/StatsView.h"

#include "imgui.h"
#include "simulation/Properties.h"

void StatsView::render() {
    if (!_visible)
        return;

    ImGui::Begin("Stats");

    if (!_grid) {
        ImGui::End();
        return;
    }

    float current = 0;
    float compressed = 0;
    float floating = 0;
    for (unsigned int i = 0; i < _grid->rho().size(); i++) {
        if (_grid->rho()[i] > Properties::Get()->LIQUID_THRESHOLD)
            current += 1;
        else
            floating += _grid->rho()[i];

        if (_grid->rho()[i] > 1.f)
            compressed += _grid->rho()[i] - 1.f;

        //val += _grid->rho()[i];
    }

    const float total = Properties::Get()->TOTAL_RHO;

    _times[_times_offset] = current;
    _times_offset = (_times_offset + 1) % MAX_TIMES;

    ImGui::PlotLines(
        "Rho",
        _times,
        MAX_TIMES,
        _times_offset,
        nullptr,
        0.0f,
        total,
        ImVec2(0, 80)
    );

    ImGui::Text("In Fluid: %.0f/ %.0f (%.2f)", current, total, (current/total) * 100.f);
    ImGui::Text("Compressed: %.0f/ %.0f (%.2f)", compressed, total, (compressed/total) * 100.f);
    ImGui::Text("Floating: %.0f/ %.0f (%.2f)", floating, total, (floating/total) * 100.f);

    ImGui::End();
}

void StatsView::recreate(Grid *grid) {
    _grid = grid;
}
