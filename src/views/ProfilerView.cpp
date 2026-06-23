#include "views/ProfilerView.h"

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  ProfilerView.cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <ranges>
#include <ratio>
#include <vector>

#include "imgui.h"
#include "util/GuiHelper.h"
#include "util/Profiler.h"

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// §1  LOCAL PALETTE
//     Visually distinct hues, all legible on a near-black background.
//     Order matches typical simulation pipelines (sharpen, advect, …)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
namespace {

static const ImVec4 kStageColors[] = {
    {0.19f, 0.80f, 0.60f, 1.0f},   // 0 teal   – Rho Sharpen
    {0.31f, 0.56f, 1.00f, 1.0f},   // 1 blue   – Rho Advect
    {0.97f, 0.74f, 0.24f, 1.0f},   // 2 amber  – Vel Advect
    {0.92f, 0.36f, 0.36f, 1.0f},   // 3 coral  – Add Force
    {0.76f, 0.44f, 1.00f, 1.0f},   // 4 violet – Project
    {0.30f, 0.84f, 0.96f, 1.0f},   // 5 cyan
    {1.00f, 0.55f, 0.30f, 1.0f},   // 6 orange
    {0.55f, 0.90f, 0.30f, 1.0f},   // 7 lime
};
static constexpr int kNumColors = (int)(sizeof(kStageColors) / sizeof(kStageColors[0]));

// Shared neutral tones
static const ImVec4 kACC  {0.31f, 0.56f, 1.00f, 1.0f};
static const ImVec4 kDIM  {0.44f, 0.46f, 0.54f, 1.0f};
static const ImVec4 kPEAK {0.28f, 0.30f, 0.38f, 1.0f};

static ImU32 U32(ImVec4 v)  { return ImGui::ColorConvertFloat4ToU32(v); }
static ImU32 U32A(ImVec4 v, float a) {
    return ImGui::ColorConvertFloat4ToU32({v.x, v.y, v.z, a});
}

static ImU32 COLOR(const int idx) {
    return U32(kStageColors[idx % 8]);
}
static ImU32 COLOR_A(const int idx, const float a) {
    return U32A(kStageColors[idx % 8], a);
}

} // anonymous namespace


// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// §3  MULTI-LINE PLOT
//
//     Draws one polyline per stage on a shared canvas using ImDrawList so
//     all lines share the same Y axis and can be compared directly.
//
//     Canvas layout:
//       • Dark bg rect
//       • Horizontal grid lines at 25 / 50 / 75 % of scale
//       • Per stage: anti-aliased polyline + filled dot at current sample
//       • Y-axis labels: full scale at top, midpoint at centre
//       • Thin border
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
void ProfilerView::draw_plot(const std::map<std::string, Profiler::Profile*>& profiles,
                                     float scaleMax, ImVec2 size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0      = ImGui::GetCursorScreenPos();
    ImVec2 p1      = {p0.x + size.x, p0.y + size.y};

    // ── Background ────────────────────────────────────────────────────────────
    dl->AddRectFilled(p0, p1, IM_COL32(9, 9, 12, 255), 6.0f);
    dl->PushClipRect(p0, p1, true);

    // ── Horizontal grid lines at 25%, 50%, 75% ───────────────────────────────
    static const float kGrid[] = {0.25f, 0.50f, 0.75f};
    for (float f : kGrid) {
        float y = p0.y + size.y * f;
        dl->AddLine({p0.x, y}, {p1.x, y}, IM_COL32(30, 32, 44, 255), 1.0f);
    }

    // ── One polyline per stage ────────────────────────────────────────────────
    std::vector<ImVec2> pts;
    pts.reserve(PROFILER_SAMPLES);

    for (int i = 0; const auto [name, profile]: profiles) {
        if (_disabled_profiles.contains(name) && _disabled_profiles[name]) {
            i++;
            continue;
        }

        pts.clear();
        for (int i2 = 0; i2 < PROFILER_SAMPLES; ++i2) {
            int   idx  = (profile->idx() + i2) % PROFILER_SAMPLES;
            float v    = profile->data()[idx];
            for (int iAvg = 0; iAvg < (_avg_samples - 1); iAvg++) {
                if (idx == 0)
                    idx = PROFILER_SAMPLES - 1;
                else
                    idx--;

                v += profile->data()[idx];
            }
            v /= (_avg_samples + 1);

            float x    = p0.x + (float)i2 / (float)(PROFILER_SAMPLES - 1) * size.x;
            float frac = (scaleMax > 0.0f) ? v / scaleMax : 0.0f;
            float y    = p0.y + size.y * (1.0f - frac);
            pts.push_back({x, y});
        }

        ImU32 lineCol = COLOR(i);

        // Faint area fill under the line
        std::vector<ImVec2> fill = pts;
        fill.push_back({p1.x, p1.y});
        fill.push_back({p0.x, p1.y});
        //dl->AddConvexPolyFilled(fill.data(), (int)fill.size(), U32A(kStageColors[i], 0.07f));

        // Line itself (1.5 px, anti-aliased by ImGui by default)
        dl->AddPolyline(pts.data(), PROFILER_SAMPLES, lineCol, ImDrawFlags_None, 1.5f);

        // Filled ring at current (rightmost) sample
        dl->AddCircleFilled(pts.back(), 3.5f, lineCol);
        dl->AddCircleFilled(pts.back(), 1.5f, IM_COL32(9, 9, 12, 255));

        i++;
    }

    dl->PopClipRect();

    // ── Frame border ──────────────────────────────────────────────────────────
    dl->AddRect(p0, p1, IM_COL32(36, 38, 52, 255), 6.0f, 0, 1.0f);

    // ── Y-axis labels ─────────────────────────────────────────────────────────
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%.2f ms", scaleMax);
    dl->AddText({p0.x + 5.0f, p0.y + 4.0f},
                IM_COL32(60, 62, 80, 255), buf);

    std::snprintf(buf, sizeof(buf), "%.2f", scaleMax * 0.5f);
    dl->AddText({p0.x + 5.0f, p0.y + size.y * 0.5f - 7.0f},
                IM_COL32(40, 42, 58, 255), buf);

    // Advance ImGui cursor past the canvas
    ImGui::Dummy(size);
}


void ProfilerView::render() {
    ImGui::Begin("Profiler");

    Util::SectionHeader("PIPELINE PROFILER");

    const auto& profiles = Profiler::Get()->profiles();

    if (profiles.empty()) {
        ImGui::TextDisabled("(no profiler data yet)");
        ImGui::End();
        return;
    }

    // ── Dynamic scale: max across all history, 20 % headroom, floor 1 ms ─────
    float scaleMax = 0.0f;
    float total = 0;
    float avg[profiles.size()];
    for (int i = 0; auto [name, profile] : profiles) {
        if (_disabled_profiles.contains(name) && _disabled_profiles[name]);
        else scaleMax = std::max(scaleMax, profile->max());

        avg[i] = profile->avg(_avg_samples);
        total += avg[i];
        i++;
    }
    scaleMax = std::max(scaleMax * 1.2f, 1.0f);

    // ── Multi-line graph ──────────────────────────────────────────────────────
    float plotW = ImGui::GetContentRegionAvail().x;
    //drawMultiLinePlot(_stages, scaleMax, {plotW, 100.0f});
    draw_plot(profiles, scaleMax, {plotW, 100.0f});
    ImGui::Spacing();

    constexpr ImGuiTableFlags kTbl =
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_BordersInnerH  |
        ImGuiTableFlags_NoHostExtendX;

    if (!ImGui::BeginTable("##prof", 4, kTbl)) {
        ImGui::End();
        return;
    }

    ImGui::TableSetupColumn("##name",  ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##cur",   ImGuiTableColumnFlags_WidthFixed, 68.0f);
    ImGui::TableSetupColumn("##peak",  ImGuiTableColumnFlags_WidthFixed, 72.0f);
    ImGui::TableSetupColumn("##pct",   ImGuiTableColumnFlags_WidthFixed, 46.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ── One row per stage ─────────────────────────────────────────────────────
    for (int i = 0; const auto [name, profile]: profiles) {

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        {
            ImVec2 cell_start = ImGui::GetCursorScreenPos();
            const float h = ImGui::GetTextLineHeight();
            ImGui::InvisibleButton(
                ("##cell" + std::to_string(i)).c_str(),
                ImVec2(ImGui::GetContentRegionAvail().x, h));

            const bool clicked = ImGui::IsItemClicked();

            ImGui::SetCursorScreenPos(cell_start);
            auto color = COLOR(i);
            if (_disabled_profiles.contains(name) && _disabled_profiles[name])
                color = U32({0.7f, 0.70f, 0.70f, 1.0f});
            dl->AddCircleFilled(
                {cell_start.x + 5.0f, cell_start.y + h * 0.5f},
                3.5f,
                color);

            ImGui::SetCursorScreenPos({cell_start.x + 13.0f, cell_start.y});
            ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
            ImGui::TextUnformatted(name.c_str());
            ImGui::PopStyleColor();

            if (clicked) {
                if (!_disabled_profiles.contains(name))
                    _disabled_profiles[name] = true;
                else
                    _disabled_profiles[name] = !_disabled_profiles[name];
            }
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f ms", avg[i]);

        ImGui::TableSetColumnIndex(2);
        ImGui::PushStyleColor(ImGuiCol_Text, kPEAK);
        ImGui::Text("%.3f ms", profile->max());
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(3);
        {
            ImVec2 b0 = ImGui::GetCursorScreenPos();
            const float bW = ImGui::GetContentRegionAvail().x;
            const float bH = ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y;

            dl->AddRectFilled(b0, {b0.x + bW, b0.y + bH},
                              IM_COL32(18, 18, 26, 255), 3.0f);
            dl->AddRectFilled(
                b0, {b0.x + bW * (avg[i] / total), b0.y + bH},
                COLOR_A(i, .5f), 3.0f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR(i));
        ImGui::Text("%.1f%%", (avg[i] / total) * 100.0f);
        ImGui::PopStyleColor();

        i++;
    }

    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::TextUnformatted("Total");
    ImGui::PopStyleColor();

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f ms", total);

    ImGui::TableSetColumnIndex(2);
    ImGui::PushStyleColor(ImGuiCol_Text, kPEAK);
    ImGui::Text("~%.0f fps", 1000.0f / std::max(total, 0.001f));
    ImGui::PopStyleColor();

    ImGui::TableSetColumnIndex(3);
    if (ImGui::Button("Reset"))
        Profiler::Get()->resetMax();

    ImGui::EndTable();

    ImGui::SliderInt("Avg Samples", &_avg_samples, 1, 30);

    ImGui::End();
}