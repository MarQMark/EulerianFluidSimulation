#include "views/SceneView2D.h"

#include <filesystem>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "simulation/display/SimulationDisplay2D.h"


SceneView2D::SceneView2D(Scene2D *scene)  : _scene(scene) {
    refresh_scene_list();
}

void SceneView2D::render(void *renderData) {
    if (!_visible)
        return;

    auto* rData = static_cast<SimulationDisplay2D::RenderData2D*>(renderData);
    if (!rData || !rData->grid)
        return;

    ImGui::Begin("Scene2D");
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    if (ImGui::Button("Save Scene", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveScene", "Choose location", ".scene2D", config);
    }

    if (ImGuiFileDialog::Instance()->Display("SaveScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            _scene->saveScene(rData->grid, filePathName, _sl_properties);

            refresh_scene_list();
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Load Scene", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadScene", "Choose location", ".scene2D", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            _scene->loadScene(rData->grid, filePathName, _sl_properties);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    bool loadedScene = _scene->loadedScene();
    if (!loadedScene) ImGui::BeginDisabled();
    if (ImGui::Button("Reset Scene", ImVec2(totalWidth, 0))) {
        _scene->resetScene(rData->grid);
    }
    if (ImGui::Button("Clear Scene", ImVec2(totalWidth, 0))) {
        _scene->clearScene();
    }
    if (!loadedScene) ImGui::EndDisabled();

    ImGui::Separator();

    if (ImGui::Button("Load Image", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadImage", "Choose location", ".png", config);
    }
    if (ImGuiFileDialog::Instance()->Display("LoadImage")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            _scene->loadImage(rData->grid, filePathName, _threshold);
        }

        ImGuiFileDialog::Instance()->Close();
    }
    ImGui::SliderInt("Value", &_threshold, -255, 255);

    ImGui::Separator();

    float listSize = ImGui::GetContentRegionAvail().y - 25;
    if (ImGui::BeginListBox("##listbox", ImVec2(-FLT_MIN, listSize))) {
        for (int i = 0; i < _scene_list.size(); i++) {
            if (ImGui::Selectable(_scene_list[i].c_str(), i == _selected_scene)) {
                _selected_scene = i;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                _scene->loadScene(rData->grid, "assets//scenes//" + _scene_list[i] + ".scene2D", _sl_properties);
            }
        }

        ImGui::EndListBox();
    }

    ImGui::Checkbox("Save/Load Properties", &_sl_properties);
    ImGui::End();
}

void SceneView2D::refresh_scene_list() {
    _scene_list.clear();

    namespace fs = std::filesystem;
    const fs::path dir = "assets//scenes//";
    if (!fs::exists(dir) || !fs::is_directory(dir))
        return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".scene2D") {
            _scene_list.push_back(entry.path().stem().string());
        }
    }
}

