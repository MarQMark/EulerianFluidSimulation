//
// Created by ruut on 6/30/26.
//

#include "views/SceneView.h"
#include <vector>
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "core/Editor.h"

SceneView::SceneView(Editor* editor, Renderer3D* renderer)
    : _editor(editor), _renderer(renderer)
{ }

void SceneView::render(void* renderData) {
    if (!_visible)
        return;

    ImGui::Begin("Scene");
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Scene ---
    ImGui::SeparatorText("Scene Graph");

    std::vector<SceneNode*> sceneNodes = _editor->getScene()->getSceneNodes();

    if (ImGui::BeginTable("##sceneNodes", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        for (int i = 0 ; i < sceneNodes.size() ; i++) {
            if (!sceneNodes[i]->isSceneGraphVisible())
                continue;

            ImGui::TableNextRow();
            ImGui::PushID(i);

            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            bool isSelected = (sceneNodes[i] == _editor->getSelected());

            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.60f, 0.28f, 0.28f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.35f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.80f, 0.42f, 0.42f, 1.00f));
            }

            if (ImGui::Button(sceneNodes[i]->getName().c_str(), ImVec2(-FLT_MIN, 19))) {
                if (isSelected) {
                    _editor->setSelected(nullptr);
                } else {
                    _editor->setSelected(sceneNodes[i]);
                }
            }

            if (isSelected)
                ImGui::PopStyleColor(3);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (isSelected) {
                    _editor->setSelected(nullptr);
                }
            }

            ImGui::TableNextColumn();

            if (sceneNodes[i]->isSceneGraphHideable()) {
                auto visibilityLabel = sceneNodes[i]->isVisible() ? "Hide" : "Show";
                if (ImGui::Button(visibilityLabel, ImVec2(0, 19))) {
                    sceneNodes[i]->setVisible(!sceneNodes[i]->isVisible());
                }
            }

            ImGui::TableNextColumn();
            if (sceneNodes[i]->isDeletable()) {
                if (ImGui::Button(" - ", ImVec2(0, 19))) {
                    if (isSelected) {
                        _editor->setSelected(nullptr);
                    }

                    _editor->removeSceneNode(sceneNodes[i]);
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("");

    // --- Particle Spawns ---
    if (ImGui::Button("Add Fluid Spawn Cube", ImVec2(totalWidth, 0))) {
        _editor->addParticleSpawn("");
    }

    if (ImGuiFileDialog::Instance()->Display("LoadParticleSpawnModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            _editor->addParticleSpawn(ImGuiFileDialog::Instance()->GetFilePathName());
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Add Fluid Spawn Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadParticleSpawnModel", "Choose Model", ".obj", config);
    }

    // --- Add (Load) Model ---
    if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            _editor->addBoundary(ImGuiFileDialog::Instance()->GetFilePathName());
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Add Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadModel", "Choose Model", ".obj", config);
    }

    // --- Force Fields ---
    if (ImGui::Button("Add Force Field", ImVec2(totalWidth, 0))) {
        _editor->addForceField();
    }

    ImGui::SeparatorText("");

    if (ImGui::Button("Save Scene", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveScene", "Choose location", ".json", config);
    }

    if (ImGuiFileDialog::Instance()->Display("SaveScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            _editor->getScene()->saveScene(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Load Scene", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadScene", "Choose location", ".json", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            _editor->getScene()->loadScene(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Reset Scene", ImVec2(totalWidth, 0))) {
        _editor->setSelected(nullptr);
        _editor->getScene()->resetScene();
    }

    ImGui::End();
}
