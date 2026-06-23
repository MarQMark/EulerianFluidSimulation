#include "views/FPSView.h"

#include "imgui.h"
#include "simulation/Properties.h"
#include "util/GuiHelper.h"
#include "util/Time.h"

void FPSView::render() {
    if (!_visible)
        return;

    ImGui::Begin("Performance");

    _times[_times_offset] = static_cast<float>(Time::Get()->fps);
    _times_offset = (_times_offset + 1) % MAX_TIMES;

    // ── FPS colour: green ≥ 90 %, yellow ≥ 60 %, red below ──────────────────
    float  fpsFrac  = static_cast<float>(Time::Get()->fps)
                    / static_cast<float>(Time::Get()->targetFps);
    ImVec4 fpsColor = (fpsFrac >= 0.90f) ? ImVec4{0.22f, 0.78f, 0.50f, 1.00f}  // OK
    : (fpsFrac >= 0.60f) ? ImVec4{0.97f, 0.74f, 0.24f, 1.00f}  // WARN
    :                      ImVec4{0.92f, 0.36f, 0.36f, 1.00f}; // ERR

    // ── Header ───────────────────────────────────────────────────────────────
    Util::SectionHeader("PERFORMANCE");

    // ── Plot (colour tracks FPS health, darkened frame background) ───────────
    ImGui::PushStyleColor(ImGuiCol_PlotLines,        fpsColor);
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, fpsColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4{0.07f, 0.09f, 0.14f, 1.00f});
    ImGui::PlotLines("##FpsPlot",
                     _times, MAX_TIMES, _times_offset,
                     nullptr, 0.0f,
                     static_cast<float>(Time::Get()->targetFps),
                     ImVec2(-FLT_MIN, 80));
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // ── Current FPS readout ───────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.44f, 0.46f, 0.53f, 1.00f});
    ImGui::TextUnformatted("Current FPS");
    ImGui::PopStyleColor();

    ImGui::SameLine(115.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, fpsColor);
    ImGui::Text("%u fps", Time::Get()->fps);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Target FPS input ─────────────────────────────────────────────────────
    Util::AddTextField("Target FPS", Time::Get()->targetFps);

    ImGui::End();
}
