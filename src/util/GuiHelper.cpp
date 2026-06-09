#include "util/GuiHelper.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"


void Util::parse_val(float& dest, std::string& src, bool reset){
    if(src.empty())
        return;


    float tmp = dest;
    try {
        tmp = std::stof(src);
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

void Util::AddTextField(const std::string& name, int& val) {
    ImGui::Columns(2, nullptr, false);
    ImGui::TextUnformatted(name.c_str());
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputInt(std::string("##" + name).c_str(), &val);
    ImGui::Columns(1);
}
void Util::AddTextField(const std::string& name, float& val) {
    ImGui::Columns(2, nullptr, false);
    ImGui::TextUnformatted(name.c_str());
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputFloat(std::string("##" + name).c_str(), &val, 0, 0, "%.6f", ImGuiInputTextFlags_CharsScientific);
    ImGui::Columns(1);
}
void Util::AddTextField(const std::string& name, unsigned int& val) {
    ImGui::Columns(2, nullptr, false);
    ImGui::TextUnformatted(name.c_str());
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputScalar((std::string("##" + name).c_str() + name).c_str(),
                       ImGuiDataType_U32,
                       &val);
    ImGui::Columns(1);
}