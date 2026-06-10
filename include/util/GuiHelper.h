#ifndef EULERIANFLUIDSIMULATION_GUIHELPER_H
#define EULERIANFLUIDSIMULATION_GUIHELPER_H
#include <string>

#include "imgui.h"

class Util {

public:
    static void AddTextField(const std::string& name, int& val, const std::string& description = "");
    static void AddTextField(const std::string& name, float& val, const std::string& description = "");
    static void AddTextField(const std::string& name, unsigned int& val, const std::string& description = "");
    static void AddBasicTextField(const std::string& name, std::string& text, const std::string& description = "");

    template<typename T>
    static void AddCombo(const std::string& name, T& value, const char* options[], int len, const std::string& description = "");

    static void ApplyTheme();
    static void SectionHeader(const char *title);
    static void StatRow(const char *label, float value, float total, ImVec4 color);
    static void AddTooltip(const std::string& description);
private:
    static void parse_val(float& dest, std::string& src, bool reset = true);
    static void parse_val(int& dest, std::string& src, bool reset = true);
    static void parse_val(unsigned int& dest, std::string& src, bool reset = true);

    static float label_width();
};

template<typename T>
void Util::AddCombo(const std::string& name, T& value, const char** options, const int len, const std::string& description) {
    ImGui::PushStyleColor(ImGuiCol_Text, {0.44f, 0.46f, 0.53f, 1.00f});
    ImGui::TextUnformatted(name.c_str());
    Util::AddTooltip(description);
    ImGui::PopStyleColor();
    ImGui::SameLine(label_width());
    ImGui::SetNextItemWidth(-FLT_MIN);

    if (ImGui::BeginCombo(("##Combo" + name).c_str(), options[static_cast<int>(value)])) {
        for (int i = 0; i < len; ++i) {
            bool selected = (value == static_cast<T>(i));
            if (ImGui::Selectable(options[i], selected))
                value = static_cast<T>(i);
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    Util::AddTooltip(description);
}


#endif //EULERIANFLUIDSIMULATION_GUIHELPER_H
