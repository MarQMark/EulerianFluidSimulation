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

    for (const auto p: _properties)
        delete p;
}

Properties::Category & Properties::add_category(const std::string &name, const bool expanded) {
    auto* cat = new Category(name, expanded);
    _categories.push_back(cat);
    return *cat;
}

Properties::SubCategory & Properties::add_sub_category(Category &cat, const std::string &name) {
    auto* sub = new SubCategory(name);
    cat.sub_categories.push_back(sub);
    return *sub;
}

void Properties::add_property(const unsigned int token, IProperty *p) {
    if (_properties.size() <= token) {
        LOG_WARN("Adding uninitialized Property: " + p->name)
        _properties.push_back(p);
        return;
    }
    if(_properties[token]) {
        LOG_ERROR("Duplicate Property (" + std::to_string(token) + "): " + p->name);
        std::abort();
    }
    _properties[token] = p;
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
    const char* shapeNames[] = {"Full", "Half", "Square", "Empty"};
    constexpr InitShape shapes[] = {FULL, HALF, SQUARE, EMPTY};
    Util::AddCombo(name, value, shapes, shapeNames, IM_ARRAYSIZE(shapes), description);
}
template<>
void Properties::Property<Properties::DrawMode>::draw(){
    const char* modeNames[] = {"Velocity", "Density", "Divergence", "Pressure", "Fluid", "Gamma", "Beta"};
    constexpr DrawMode modes[] = { VELOCITY, DENSITY, DIVERGENCE, PRESSURE, FLUID, GAMMA, BETA};
    Util::AddCombo(name, value, modes, modeNames, IM_ARRAYSIZE(modes), description);
}
template<>
void Properties::Property<Properties::SolverType>::draw(){
    std::vector<const char*> solverNames;
    auto* solvers = PROPERTY(AVLB_SOLVERS, std::vector<Properties::SolverType>*);
    for (const auto s : *solvers) {
        switch (s) {
            case EIGEN_CG:
                solverNames.push_back("Eigen CG");
                break;
            case EIGEN_BiCGSTAB:
                solverNames.push_back("Eigen BiCGSTAB");
                break;
            case SELF_CG:
                solverNames.push_back("Self CG");
                break;
            case JACOBI:
                solverNames.push_back("Jacobi");
                break;
            default:;
        }
    }

    if (solverNames.empty())
        ImGui::Text("No solver available");
    else
        Util::AddCombo(name, value, solvers->data(),solverNames.data(), solverNames.size(), description);
}
template<>
void Properties::Property<Properties::SimulatorType>::draw(){}

template<>
void Properties::Property<std::vector<Properties::SolverType>*>::draw() {}