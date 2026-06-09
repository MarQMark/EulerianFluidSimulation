#include "views/PropertiesView.h"

#include <sstream>
#include <iostream>
#include <ostream>

#include "imgui.h"
#include "imgui_internal.h"
#include "simulation/Properties.h"
#include "util/GuiHelper.h"


void PropertiesView::render() {
    ImGui::Begin("Properties");
    // ImGui::PushItemWidth(-FLT_MIN);

    for (const auto& cat : Properties::Get()->getCategories()) {
        if (ImGui::CollapsingHeader(cat->name.c_str())) {
            for (const auto& sub : cat->sub_categories) {
                ImGui::SeparatorText(sub->name.c_str());
                for (const auto& p : sub->properties)
                    p->draw();
            }
            if (!cat->sub_categories.empty())
                ImGui::SeparatorText("Ungrouped");
            for (const auto& p : cat->properties)
                p->draw();
        }
    }

    //ImGui::PopItemWidth();
    ImGui::End();
}