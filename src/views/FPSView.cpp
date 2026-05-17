#include "views/FPSView.h"

#include "imgui.h"
#include "util/GuiHelper.h"
#include "util/Time.h"

void FPSView::render() {
    if (!_visible)
        return;

    ImGui::Begin("Performance");

    _times[_times_offset] = Time::Get()->fps;
    _times_offset = (_times_offset + 1) % MAX_TIMES;

    ImGui::PlotLines(
        "FPS",
        _times,
        MAX_TIMES,
        _times_offset,
        nullptr,
        0.0f,
        Time::Get()->targetFps,
        ImVec2(0, 80)
    );

    ImGui::Text("Current FPS: %u", Time::Get()->fps);
    ImGui::Separator();
    Util::AddTextField("Target FPS", Time::Get()->targetFps);

    ImGui::End();
}
