#ifndef EULERIANFLUIDSIMULATION_GUIHELPER_H
#define EULERIANFLUIDSIMULATION_GUIHELPER_H
#include <string>

#include "imgui.h"

class Util {
public:
    static void AddTextField(const std::string& name, int& val);
    static void AddTextField(const std::string& name, float& val);
    static void AddTextField(const std::string& name, unsigned int& val);

    template<typename T>
    static void AddCombo(const std::string& name, T& value, const char* options[], int len);

private:
    static void parse_val(float& dest, std::string& src, bool reset = true);
    static void parse_val(int& dest, std::string& src, bool reset = true);
    static void parse_val(unsigned int& dest, std::string& src, bool reset = true);
};

template<typename T>
void Util::AddCombo(const std::string &name, T &value, const char** options, int len) {
    ImGui::Text("%s", name.c_str());
    if (ImGui::BeginCombo(("##Combo" + name).c_str(), options[value])) {
        for (int i = 0; i < len; i++)
        {
            const bool selected = (value == i);

            if (ImGui::Selectable(options[i], selected))
                value = (T)i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

#endif //EULERIANFLUIDSIMULATION_GUIHELPER_H
