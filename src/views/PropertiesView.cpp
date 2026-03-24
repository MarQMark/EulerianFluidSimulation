#include "views/PropertiesView.h"

#include <sstream>
#include <iostream>
#include <ostream>

#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "simulation/Properties.h"

void parse_val(float& dest, std::string& src, bool reset = true){
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

void add_field(const std::string& name, float& val) {
    std::stringstream ss;

    ss << val;
    std::string buf1(ss.str());
    ss.str(std::string());

    ImGui::Text(name.c_str());
    ImGui::InputText(std::string("##" + name).c_str(), &buf1, ImGuiInputTextFlags_CharsDecimal);

    parse_val(val, buf1);
}

void PropertiesView::render() {
    ImGui::Begin("Properties");

    add_field("Brush Size", Properties::Get()->BRUSH_SIZE);

    ImGui::Separator();

    add_field("a", Properties::Get()->a);

    ImGui::End();
}
