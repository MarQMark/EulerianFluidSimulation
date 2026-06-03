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

    if (ImGui::CollapsingHeader("Grid")) {
        Util::AddTextField("Width", Properties::Get()->WIDTH);
        Util::AddTextField("Height", Properties::Get()->HEIGHT);

        ImGui::Text("Fill Shape:");
        const char* shapes[] = {
            "Full", "Half", "Square", "Empty"
        };
        if (ImGui::BeginCombo("##Fill_Shape_combo", shapes[Properties::Get()->INIT_SHAPE])) {
            for (int i = 0; i < IM_ARRAYSIZE(shapes); i++)
            {
                bool selected = (Properties::Get()->INIT_SHAPE == i);

                if (ImGui::Selectable(shapes[i], selected))
                    Properties::Get()->INIT_SHAPE = (Properties::InitShape)i;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
    }

    if (ImGui::CollapsingHeader("Interaction")) {
        Util::AddTextField("Brush Size", Properties::Get()->BRUSH_SIZE);
    }

    if (ImGui::CollapsingHeader("Simulation")) {
        Util::AddTextField("Max Steps", Properties::Get()->MAX_STEPS);
        Util::AddTextField("Threshold", Properties::Get()->THRESHOLD);
        ImGui::Separator();

        Util::AddTextField("Viscosity", Properties::Get()->VISCOSITY);
        ImGui::Separator();

        Util::AddTextField("Lambda", Properties::Get()->LAMBDA);
        Util::AddTextField("Eta", Properties::Get()->ETA);
        ImGui::Separator();

        Util::AddTextField("DT", Properties::Get()->DT);
        ImGui::Checkbox("Gravity", &Properties::Get()->GRAVITY);
        Util::AddTextField("Gravity F", Properties::Get()->GRAVITY_FORCE);

        ImGui::Checkbox("Push", &Properties::Get()->PUSH);
        Util::AddTextField("Push F", Properties::Get()->PUSH_FORCE);
    }

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::Text("Fill Shape:");
        const char* shapes[] = {
            "Velocity", "Density", "Divergence", "Pressure", "Fluid", "Gamma", "Beta"
        };
        if (ImGui::BeginCombo("##Draw_mode_combo", shapes[Properties::Get()->DRAW_MODE])) {
            for (int i = 0; i < IM_ARRAYSIZE(shapes); i++)
            {
                bool selected = (Properties::Get()->DRAW_MODE == i);

                if (ImGui::Selectable(shapes[i], selected))
                    Properties::Get()->DRAW_MODE = (Properties::DrawMode)i;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::Text("Max value: %f", Properties::Get()->MAX_VALUE);

        if (ImGui::Button(Properties::Get()->PAUSED ? "Resume" : "Pause ")) {
            Properties::Get()->PAUSED = !Properties::Get()->PAUSED;
        }

        ImGui::Checkbox("Velocity Arrows", &Properties::Get()->DRAW_VEL_ARROWS);

        ImGui::Separator();
        ImGui::Checkbox("Re-Calculate Divergence", &Properties::Get()->RE_CALC_DIV);
    }

    //ImGui::PopItemWidth();
    ImGui::End();
}