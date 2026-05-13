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
void parse_val(int& dest, std::string& src, bool reset = true){
    if(src.empty())
        return;


    int tmp = dest;
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
void add_field(const std::string& name, int& val) {
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
    ImGui::PushItemWidth(-FLT_MIN);

    add_field("Brush Size", Properties::Get()->BRUSH_SIZE);

    ImGui::Separator();

    add_field("a", Properties::Get()->a);
    add_field("Max Steps", Properties::Get()->MAX_STEPS);
    add_field("Threshold", Properties::Get()->THRESHOLD);

    ImGui::Separator();

    add_field("DT", Properties::Get()->DT);
    add_field("Width", Properties::Get()->WIDTH);
    add_field("Height", Properties::Get()->HEIGHT);

    ImGui::Checkbox("Gravity", &Properties::Get()->GRAVITY);
    add_field("Gravity F", Properties::Get()->GRAVITY_FORCE);

    ImGui::Checkbox("Push", &Properties::Get()->PUSH);
    add_field("Push F", Properties::Get()->PUSH_FORCE);

    ImGui::PopItemWidth();
    ImGui::End();
}
