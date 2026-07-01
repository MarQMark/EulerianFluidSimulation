//
// Created by ruut on 5/31/26.
//

#include "simulation/scene/NodeProperties.h"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "glm/gtc/type_ptr.hpp"

void FloatProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    float v = get();
    if (ImGui::DragFloat(id.c_str(), &v, speed, min, max, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        set(v);
}

void BoolProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    bool v = get();
    if (ImGui::Checkbox(id.c_str(), &v))
        set(v);
};

void Vec3Property::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    glm::vec3 v = get();
    if (ImGui::DragFloat3(id.c_str(), glm::value_ptr(v), 0.01f))
        set(v);
}

void IVec3Property::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    glm::ivec3 v = get();
    if (ImGui::DragInt3(id.c_str(), glm::value_ptr(v), 0.01f))
        set(v);
}

void ColorProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    glm::vec3 v = get();
    if (ImGui::ColorEdit3(id.c_str(), glm::value_ptr(v)))
        set(v);
}

void Texture2DProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    auto currentTexture = get();

    float totalWidth = ImGui::GetContentRegionAvail().x;
    float aspect = (float)currentTexture->getHeight() / (float)currentTexture->getWidth();
    float height = totalWidth * aspect;

    ImGui::Image(
        (intptr_t)currentTexture->get(),
        ImVec2(totalWidth, height),
        ImVec2(0, 1),
        ImVec2(1,0 )
    );

    if (ImGuiFileDialog::Instance()->Display("LoadTexture")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            set(std::make_shared<Texture2D>(ImGuiFileDialog::Instance()->GetFilePathName()));
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Load Texture", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadTexture", "Choose location", ".png", config);
    }
}

void NodeProperties::addSection(const std::string& section) {
    sections.push_back({ section, static_cast<int>(properties.size()) });
}

Section NodeProperties::getSection(int index) {
    return sections[index];
}

void NodeProperties::addProperty(std::unique_ptr<NodeProperty> property) {
    properties.push_back(std::move(property));
}

NodeProperty* NodeProperties::getProperty(int index) const {
    return properties[index].get();
}

int NodeProperties::getPropertyCount() const {
    return properties.size();
}

int NodeProperties::getSectionCount() const {
    return sections.size();
}
