#include "simulation/Properties.h"

#include <iostream>
#include <ostream>

#include "imgui.h"
#include "util/GuiHelper.h"

Properties* Properties::s_instance = nullptr;

#define ADD(cat, name, desc, val) {auto* p = new Property(name, desc, val); \
                                   cat.add(p); \
                                   add_property(p); }

Properties::Properties() {
    auto& interaction = add_category("Interaction");
    ADD(interaction, "Brush Size", "Radius of Brush in screen pixels", 50.f);

    auto& simulation = add_category("Simulation2");
    auto& simulation_rho = add_sub_category(simulation, "Density");
    ADD(simulation_rho, "Lambda", "Control how much compression is penalized", .5f);
    ADD(simulation, "Gravity", "", 0.002f);
    ADD(simulation, "Gravity Enabled", "", false);
}

Properties::~Properties() {
    for (const auto cat : _categories)
        delete cat;
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
    Util::AddTextField(name, value);
}
template<>
void Properties::Property<float>::draw(){
    Util::AddTextField(name, value);
}
template<>
void Properties::Property<unsigned int>::draw(){
    Util::AddTextField(name, value);
}
template<>
void Properties::Property<bool>::draw(){
    ImGui::Checkbox(name.c_str(), &value);
}
