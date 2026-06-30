#include "util/GuiHelper.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace {
    // Surfaces ───────────────────────────────────────────────────────────────────
    static constexpr ImVec4 kBG_WIN    {0.07f, 0.07f, 0.09f, 1.00f}; // #111115
    static constexpr ImVec4 kBG_CHILD  {0.10f, 0.10f, 0.13f, 1.00f}; // #1A1A21
    static constexpr ImVec4 kBG_POPUP  {0.12f, 0.12f, 0.16f, 1.00f}; // #1E1E29
    static constexpr ImVec4 kBG_TITLE  {0.09f, 0.11f, 0.19f, 1.00f}; // #17 1C30  (active title tint)

    // Input frames ───────────────────────────────────────────────────────────────
    static constexpr ImVec4 kFRAME     {0.14f, 0.14f, 0.18f, 1.00f};
    static constexpr ImVec4 kFRAME_H   {0.18f, 0.18f, 0.23f, 1.00f};
    static constexpr ImVec4 kFRAME_A   {0.16f, 0.16f, 0.21f, 1.00f};

    // Accent: cornflower blue ────────────────────────────────────────────────────
    static constexpr ImVec4 kACC       {0.31f, 0.56f, 1.00f, 1.00f};  // #4F8FFF
    static constexpr ImVec4 kACC_DIM   {0.20f, 0.38f, 0.74f, 1.00f};  // #3360BD
    static constexpr ImVec4 kACC_DARK  {0.10f, 0.20f, 0.45f, 1.00f};  // #193373

    // ── Headers — CollapsingHeader / TreeNode / Selectable ───────────────────────
    //    Visually distinct from buttons: flatter, slate-blue family
    static constexpr ImVec4 kHEADER    {0.16f, 0.18f, 0.28f, 1.00f}; // #282E47
    static constexpr ImVec4 kHEADER_H  {0.20f, 0.26f, 0.46f, 1.00f}; // #334275
    static constexpr ImVec4 kHEADER_A  {0.26f, 0.36f, 0.66f, 1.00f}; // #425CA8

    // ── Buttons — higher saturation so they clearly read as "clickable" ──────────
    static constexpr ImVec4 kBUTTON    {0.14f, 0.22f, 0.48f, 1.00f}; // #23387A
    static constexpr ImVec4 kBUTTON_H  {0.20f, 0.34f, 0.68f, 1.00f}; // #3357AD
    static constexpr ImVec4 kBUTTON_A  {0.28f, 0.48f, 0.88f, 1.00f}; // #477AE1

    // Data colours ───────────────────────────────────────────────────────────────
    static constexpr ImVec4 kTEAL      {0.19f, 0.80f, 0.60f, 1.00f};  // #30CC99  fluid
    static constexpr ImVec4 kBLUE      {0.31f, 0.56f, 1.00f, 1.00f};  // same as kACC  fps
    static constexpr ImVec4 kOK        {0.22f, 0.78f, 0.50f, 1.00f};  // #38C780  success
    static constexpr ImVec4 kWARN      {0.97f, 0.74f, 0.24f, 1.00f};  // #F7BD3D  warning
    static constexpr ImVec4 kERR       {0.92f, 0.36f, 0.36f, 1.00f};  // #EB5C5C  danger

    // Text ───────────────────────────────────────────────────────────────────────
    static constexpr ImVec4 kTEXT      {0.88f, 0.88f, 0.92f, 1.00f};
    static constexpr ImVec4 kDIM       {0.44f, 0.46f, 0.53f, 1.00f};
    static constexpr ImVec4 kBORDER    {0.19f, 0.20f, 0.25f, 1.00f};

    // Utility ────────────────────────────────────────────────────────────────────
    static ImU32 ToU32(const ImVec4 v) { return ImGui::ColorConvertFloat4ToU32(v); }
    static constexpr float kLabelFrac   = 0.44f; // label column: 44% of available width
}

void Util::parse_val(float& dest, std::string& src, bool reset){
    if(src.empty())
        return;

    float tmp = dest;
    try {
        tmp = std::stof(src);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid input: " << src << " - " << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Overflow/Underflow error: " << e.what() << std::endl;
    }

    //if(Parameters::PARMS()->RESET_ON_CHANGE && tmp != *dest && reset)
    //    Parameters::PARMS()->RESET = true;

    dest = tmp;
}
void Util::parse_val(int& dest, std::string& src, bool reset){
    if(src.empty())
        return;

    int tmp = dest;
    try {
        tmp = std::stoi(src);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid input: " << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Overflow/Underflow error: " << e.what() << std::endl;
    }

    //if(Parameters::PARMS()->RESET_ON_CHANGE && tmp != *dest && reset)
    //    Parameters::PARMS()->RESET = true;

    dest = tmp;
}

void Util::parse_val(unsigned int& dest, std::string& src, bool reset){
    if(src.empty())
        return;

    unsigned int tmp = dest;
    try {
        tmp = std::stoul(src);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid input: " << e.what() << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Overflow/Underflow error: " << e.what() << std::endl;
    }

    //if(Parameters::PARMS()->RESET_ON_CHANGE && tmp != *dest && reset)
    //    Parameters::PARMS()->RESET = true;

    dest = tmp;
}

float Util::label_width() {
        return ImGui::GetContentRegionAvail().x * kLabelFrac;
}

void Util::ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    // ── Geometry ─────────────────────────────────────────────────────────────
    s.WindowRounding     = 5.0f;    s.ChildRounding      = 4.0f;
    s.FrameRounding      = 5.0f;    s.PopupRounding      = 4.0f;
    s.ScrollbarRounding  = 6.0f;    s.GrabRounding       = 4.0f;
    s.TabRounding        = 4.0f;

    s.WindowBorderSize   = 1.0f;    s.ChildBorderSize    = 1.0f;
    s.FrameBorderSize    = 0.0f;    s.PopupBorderSize    = 1.0f;

    s.WindowPadding      = {8.0f, 4.0f};
    s.FramePadding       = { 8.0f,  3.0f};
    s.ItemSpacing        = { 5.0f,  4.0f};
    s.ItemInnerSpacing   = { 6.0f,  4.0f};
    s.IndentSpacing      = 14.0f;
    s.ScrollbarSize      = 5.0f;
    s.GrabMinSize        =  8.0f;

    // ── Colours ───────────────────────────────────────────────────────────────
    ImVec4* c = s.Colors;

    // ── Text ──────────────────────────────────────────────────────────────────
    c[ImGuiCol_Text]                  = kTEXT;
    c[ImGuiCol_TextDisabled]          = kDIM;
    c[ImGuiCol_TextSelectedBg]        = {kACC.x, kACC.y, kACC.z, 0.25f};

    // ── Surfaces ──────────────────────────────────────────────────────────────
    c[ImGuiCol_WindowBg]              = kBG_WIN;
    c[ImGuiCol_ChildBg]               = kBG_CHILD;
    c[ImGuiCol_PopupBg]               = kBG_POPUP;
    c[ImGuiCol_Border]                = kBORDER;
    c[ImGuiCol_BorderShadow]          = {0, 0, 0, 0};

    // ── Input frames ──────────────────────────────────────────────────────────
    c[ImGuiCol_FrameBg]               = kFRAME;
    c[ImGuiCol_FrameBgHovered]        = kFRAME_H;
    c[ImGuiCol_FrameBgActive]         = kFRAME_A;

    // ── Title bars ────────────────────────────────────────────────────────────
    c[ImGuiCol_TitleBg]               = kBG_WIN;
    c[ImGuiCol_TitleBgActive]         = kBG_TITLE;  // subtle blue tint when focused
    c[ImGuiCol_TitleBgCollapsed]      = kBG_WIN;
    c[ImGuiCol_MenuBarBg]             = kBG_CHILD;

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    c[ImGuiCol_ScrollbarBg]           = {kBG_WIN.x, kBG_WIN.y, kBG_WIN.z, 0.60f};
    c[ImGuiCol_ScrollbarGrab]         = kACC_DARK;
    c[ImGuiCol_ScrollbarGrabHovered]  = kACC_DIM;
    c[ImGuiCol_ScrollbarGrabActive]   = kACC;

    // ── Checkboxes & sliders ──────────────────────────────────────────────────
    c[ImGuiCol_CheckMark]             = kACC;
    c[ImGuiCol_SliderGrab]            = kACC_DIM;
    c[ImGuiCol_SliderGrabActive]      = kACC;

    // ── Buttons ───────────────────────────────────────────────────────────────
    c[ImGuiCol_Button]                = kBUTTON;
    c[ImGuiCol_ButtonHovered]         = kBUTTON_H;
    c[ImGuiCol_ButtonActive]          = kBUTTON_A;

    // ── Headers — CollapsingHeader / TreeNode / Selectable ────────────────────
    c[ImGuiCol_Header]                = kHEADER;
    c[ImGuiCol_HeaderHovered]         = kHEADER_H;
    c[ImGuiCol_HeaderActive]          = kHEADER_A;

    // ── Separators ────────────────────────────────────────────────────────────
    c[ImGuiCol_Separator]             = kBORDER;
    c[ImGuiCol_SeparatorHovered]      = kACC_DIM;
    c[ImGuiCol_SeparatorActive]       = kACC;

    // ── Resize grip ───────────────────────────────────────────────────────────
    c[ImGuiCol_ResizeGrip]            = {kACC.x, kACC.y, kACC.z, 0.12f};
    c[ImGuiCol_ResizeGripHovered]     = {kACC.x, kACC.y, kACC.z, 0.45f};
    c[ImGuiCol_ResizeGripActive]      = kACC;

    // ── Tabs ──────────────────────────────────────────────────────────────────
    c[ImGuiCol_Tab]                   = {0.10f, 0.12f, 0.19f, 1.0f};
    c[ImGuiCol_TabHovered]            = kHEADER_H;
    c[ImGuiCol_TabActive]             = kHEADER;
    c[ImGuiCol_TabUnfocused]          = kBG_WIN;
    c[ImGuiCol_TabUnfocusedActive]    = kBG_CHILD;

    // ── Docking ───────────────────────────────────────────────────────────────
    c[ImGuiCol_DockingPreview]        = {kACC.x, kACC.y, kACC.z, 0.28f};
    c[ImGuiCol_DockingEmptyBg]        = kBG_CHILD;

    // ── Plots (safe defaults; overridden per-call in panel code) ──────────────
    c[ImGuiCol_PlotLines]             = kACC;
    c[ImGuiCol_PlotLinesHovered]      = kACC;
    c[ImGuiCol_PlotHistogram]         = kACC_DIM;
    c[ImGuiCol_PlotHistogramHovered]  = kACC;

    // ── Tables ────────────────────────────────────────────────────────────────
    c[ImGuiCol_TableHeaderBg]         = kBG_CHILD;
    c[ImGuiCol_TableBorderStrong]     = kBORDER;
    c[ImGuiCol_TableBorderLight]      = {kBORDER.x, kBORDER.y, kBORDER.z, 0.40f};
    c[ImGuiCol_TableRowBg]            = {0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt]         = {1, 1, 1, 0.025f};

    // ── Navigation / drag-drop / modal ────────────────────────────────────────
    c[ImGuiCol_DragDropTarget]        = kACC;
    c[ImGuiCol_NavHighlight]          = kACC;
    c[ImGuiCol_NavWindowingHighlight] = {1.0f, 1.0f, 1.0f, 0.70f};
    c[ImGuiCol_NavWindowingDimBg]     = {0.8f, 0.8f, 0.8f, 0.15f};
    c[ImGuiCol_ModalWindowDimBg]      = {0.06f, 0.06f, 0.09f, 0.65f};
}

void Util::SectionHeader(const char* title) {
    ImDrawList* dl    = ImGui::GetWindowDrawList();
    ImVec2      pos   = ImGui::GetCursorScreenPos();
    float       avail = ImGui::GetContentRegionAvail().x;
    float       lineY = pos.y + ImGui::GetTextLineHeight() * 0.5f;

    // Leading diamond accent
    dl->AddCircleFilled({pos.x + 5.0f, lineY}, 4.0f, ToU32(kACC));

    // Title text (offset past diamond)
    ImGui::SetCursorScreenPos({pos.x + 16.0f, pos.y});
    ImGui::PushStyleColor(ImGuiCol_Text, kACC);
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    // Trailing hairline rule
    float textRight = pos.x + 16.0f + ImGui::CalcTextSize(title).x + 8.0f;
    dl->AddLine({textRight, lineY}, {pos.x + avail, lineY}, ToU32(kBORDER), 1.0f);
}


void Util::StatRow(const char* label, float value, float total, ImVec4 color) {
    float pct = value / total;

    // Label (dimmed)
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::Text("%-12s", label);
    ImGui::PopStyleColor();

    // Value
    ImGui::SameLine(90.0f);
    ImGui::Text("%.0f / %.0f", value, total);

    // Percentage (accent colour)
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%.1f%%", pct * 100.0f);
    ImGui::PopStyleColor();

    // Mini progress bar drawn into the DrawList
    const float  barH  = 3.0f;
    const float  gapY  = 2.0f;
    ImVec2       bPos  = ImGui::GetCursorScreenPos();
    float        bW    = ImGui::GetContentRegionAvail().x;
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    // Track
    dl->AddRectFilled(
        {bPos.x,           bPos.y},
        {bPos.x + bW,      bPos.y + barH},
        IM_COL32(28, 28, 36, 255), 2.0f);

    // Fill
    if (pct > 0.0f)
        dl->AddRectFilled(
            {bPos.x,               bPos.y},
            {bPos.x + bW * pct,    bPos.y + barH},
            ToU32(color), 2.0f);

    // Advance cursor past the bar
    ImGui::Dummy({bW, barH + gapY});
}

void Util::AddTooltip(const std::string& description) {
    if (!description.empty() && ImGui::IsItemHovered()) {
        const ImVec2 mouse = ImGui::GetMousePos();
        constexpr float padding = 8.0f;
        ImVec2 tooltipSize = ImGui::CalcTextSize(description.c_str());
        tooltipSize.x += ImGui::GetStyle().WindowPadding.x * 2.0f;
        tooltipSize.y += ImGui::GetStyle().WindowPadding.y * 2.0f;

        const ImGuiViewport* vp = ImGui::GetMainViewport();

        const float minX = vp->Pos.x;
        const float minY = vp->Pos.y;
        const float maxX = vp->Pos.x + vp->Size.x;
        const float maxY = vp->Pos.y + vp->Size.y;

        ImVec2 pos(mouse.x + 16.0f, mouse.y + 16.0f);

        if (pos.x + tooltipSize.x > maxX)
            pos.x = maxX - tooltipSize.x - padding;

        if (pos.y + tooltipSize.y > maxY)
            pos.y = maxY - tooltipSize.y - padding;

        if (pos.x < minX)
            pos.x = minX + padding;

        if (pos.y < minY)
            pos.y = minY + padding;

        ImGui::SetNextWindowPos(pos);

        ImGui::BeginTooltip();
        ImGui::TextUnformatted(description.c_str());
        ImGui::EndTooltip();
    }
}

void Util::AddTextField(const std::string& name, int& val, const std::string& description) {
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::TextUnformatted(name.c_str());
    AddTooltip(description);
    ImGui::PopStyleColor();
    ImGui::SameLine(label_width());
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputInt(("##" + name).c_str(), &val);
    AddTooltip(description);
}

void Util::AddTextField(const std::string& name, float& val, const std::string& description) {
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::TextUnformatted(name.c_str());
    AddTooltip(description);
    ImGui::PopStyleColor();
    ImGui::SameLine(label_width());
    ImGui::SetNextItemWidth(-FLT_MIN);
    //ImGui::InputFloat(("##" + name).c_str(), &val,
    //                  0.0f, 0.0f, FloatFmt(val), ImGuiInputTextFlags_CharsScientific);
    std::stringstream ss;
    ss << val;
    std::string buf(ss.str());
    ImGui::InputText(name.c_str(), &buf);
    parse_val(val, buf);
    AddTooltip(description);
}

void Util::AddTextField(const std::string& name, unsigned int& val, const std::string& description) {
    ImGui::PushStyleColor(ImGuiCol_Text, kDIM);
    ImGui::TextUnformatted(name.c_str());
    AddTooltip(description);
    ImGui::PopStyleColor();
    ImGui::SameLine(label_width());
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputScalar(("##" + name).c_str(), ImGuiDataType_U32, &val);
    AddTooltip(description);
}

void Util::AddBasicTextField(const std::string &name, std::string &text, const std::string &description) {
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText(("##" + name).c_str(), &text);
    AddTooltip(description);
}
