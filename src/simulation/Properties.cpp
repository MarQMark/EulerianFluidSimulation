#include "simulation/Properties.h"

#include <iostream>
#include <ostream>
#include <ranges>

#include "imgui.h"
#include "util/GuiHelper.h"

Properties* Properties::s_instance = nullptr;

Properties::~Properties() {
    for (const auto cat : _categories)
        delete cat;

    for (const auto p: _properties | std::views::values)
        delete p;
}

Properties::Category & Properties::add_category(const std::string &name) {
    auto* cat = new Category(name);
    _categories.push_back(cat);
    return *cat;
}

Properties::SubCategory & Properties::add_sub_category(Category &cat, const std::string &name) {
    auto* sub = new SubCategory(name);
    cat.sub_categories.push_back(sub);
    return *sub;
}

void Properties::add_property(IProperty *p) {
    if (_properties.contains(p->name)) {
        std::cerr << "Duplicate Property name: " << p->name << std::endl;
        std::abort();
    }
    _properties[p->name] = p;
}

template<typename T>
void Properties::Property<T>::draw() {
    ImGui::Text("%s: unsupported type", name.c_str());
}
template<>
void Properties::Property<int>::draw(){
    Util::AddTextField(name, value, description);
}
template<>
void Properties::Property<float>::draw(){
    Util::AddTextField(name, value, description);
}
template<>
void Properties::Property<unsigned int>::draw(){
    Util::AddTextField(name, value, description);
}
template<>
void Properties::Property<bool>::draw(){
    ImGui::Checkbox(name.c_str(), &value);
    Util::AddTooltip(description);
}
template<>
void Properties::Property<Properties::InitShape>::draw(){
    const char* shapes[] = {"Full", "Half", "Square", "Empty"};
    Util::AddCombo(name, value, shapes, IM_ARRAYSIZE(shapes), description);
}
template<>
void Properties::Property<Properties::DrawMode>::draw(){
    const char* shapes[] = {"Velocity", "Density", "Divergence", "Pressure", "Fluid", "Gamma", "Beta"};
    Util::AddCombo(name, value, shapes, IM_ARRAYSIZE(shapes), description);
}
template<>
void Properties::Property<Properties::SolverType>::draw(){
    const char* shapes[] = {"Eigen CG", "Eigen BiCGSTAB", "Self CG", "Jacobi"};
    Util::AddCombo(name, value, shapes, IM_ARRAYSIZE(shapes), description);
}
template<>
void Properties::Property<Properties::SimulatorType>::draw(){}