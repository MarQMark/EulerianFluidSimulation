//
// Created by ruut on 6/30/26.
//

#include "views/NodePropertiesView.h"

#include "imgui.h"
#include "core/Editor.h"

NodePropertiesView::NodePropertiesView(Editor* editor)
    : _editor(editor)
{ }

void NodePropertiesView::render(void* renderData) {
    if (!_visible)
        return;

    if (_editor->getSelected() != _selected) {
        _selected = _editor->getSelected();

        if (_selected)
            _properties = _selected->buildProperties();
    }

    ImGui::Begin("Properties");
    ImGui::PushItemWidth(-FLT_MIN);

    if (_selected) {
        int sectionIndex = 0;
        for (int p = 0 ; p < _properties.getPropertyCount() ; p++) {
            while (sectionIndex < _properties.getSectionCount() && _properties.getSection(sectionIndex).index == p) {
                ImGui::SeparatorText(_properties.getSection(sectionIndex).name.c_str());
                sectionIndex++;
            }

            _properties.getProperty(p)->draw();
        }
    } else {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No object selected.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
