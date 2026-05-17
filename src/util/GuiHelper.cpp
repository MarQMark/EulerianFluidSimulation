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
    std::stringstream ss;

    ss << val;
    std::string buf1(ss.str());
    ss.str(std::string());

    ImGui::Text(name.c_str());
    ImGui::InputText(std::string("##" + name).c_str(), &buf1, ImGuiInputTextFlags_CharsDecimal);

    parse_val(val, buf1);
}

void Util::AddTextField(const std::string& name, float& val) {
    std::stringstream ss;

    ss << val;
    std::string buf1(ss.str());
    ss.str(std::string());

    ImGui::Text(name.c_str());
    ImGui::InputText(std::string("##" + name).c_str(), &buf1, ImGuiInputTextFlags_CharsDecimal);

    parse_val(val, buf1);
}

void Util::AddTextField(const std::string& name, unsigned int& val) {
    std::stringstream ss;

    ss << val;
    std::string buf1(ss.str());
    ss.str(std::string());

    ImGui::Text(name.c_str());
    ImGui::InputText(std::string("##" + name).c_str(), &buf1, ImGuiInputTextFlags_CharsDecimal);

    parse_val(val, buf1);
}
