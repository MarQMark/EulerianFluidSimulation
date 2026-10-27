//
// Created by ruut on 6/30/26.
//

#include "views/SceneView.h"
#include <vector>
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "core/Editor.h"
#include "core/Input.h"
#include "simulation/data/IGrid.h"
#include "simulation/display/SimulationDisplay3D.h"
#include "util/Time.h"

SceneView::SceneView(Editor* editor, Renderer3D* renderer)
    : _editor(editor), _renderer(renderer) {
    refresh_scene_list();
}

void SceneView::render(void* renderData) {
    if (!_visible)
        return;

    auto* rData = static_cast<SimulationDisplay3D::RenderData3D*>(renderData);
    if (!rData || !rData->grid)
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

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && Input::Get()->isKeyPressed(Window::Key::Escape)) {
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
                    _editor->initBoundaries(rData->grid->boundary(), rData->grid->rho());
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
            _editor->initBoundaries(rData->grid->boundary(), rData->grid->rho());
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

    // --- Queries ---
    if (ImGui::Button("Add Query", ImVec2(totalWidth, 0))) {
        _editor->addQuery();
    }

    // --- Deletion Area ---
    if (ImGui::Button("Add Deletion Area", ImVec2(totalWidth, 0))) {
        _editor->addDeletionArea();
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
            _editor->getScene()->saveScene(filePathName, _sl_properties);

            refresh_scene_list();
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
            _editor->setSelected(nullptr);
            _editor->getScene()->loadScene(filePathName, _sl_properties);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Reset Scene", ImVec2(totalWidth, 0))) {
        _editor->setSelected(nullptr);
        _editor->getScene()->resetScene();
    }

    ImGui::Separator();

    float listSize = ImGui::GetContentRegionAvail().y - 25;
    if (ImGui::BeginListBox("##listbox", ImVec2(-FLT_MIN, listSize))) {
        for (int i = 0; i < _scene_list.size(); i++) {
            if (ImGui::Selectable(_scene_list[i].c_str(), i == _selected_scene)) {
                _selected_scene = i;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                _editor->setSelected(nullptr);
                _editor->getScene()->loadScene("assets//scenes//" + _scene_list[i] + ".json", _sl_properties);
            }
        }

        ImGui::EndListBox();
    }

    ImGui::Checkbox("Save/Load Properties", &_sl_properties);
    ImGui::End();
}

void SceneView::refresh_scene_list() {
    _scene_list.clear();

    namespace fs = std::filesystem;
    const fs::path dir = "assets//scenes//";
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            _scene_list.push_back(entry.path().stem().string());
        }
    }
}
