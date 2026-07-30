//
// Created by ruut on 7/27/26.
//

#include "views/QueryDataView.h"
#include "imgui.h"
#include "implot.h"
#include "core/Editor.h"
#include "simulation/scene/nodes/QueryNode.h"

QueryDataView::QueryDataView(Editor* editor) : _editor(editor) { }

void QueryDataView::render(void* renderData) {
    if (!_visible)
        return;

    ImGui::Begin("Query Data");
    ImGui::PushItemWidth(-FLT_MIN);

    auto queryNodes = _editor->getScene()->getQueryNodes();

    if (queryNodes.size() == 0) {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No query nodes in scene.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No query nodes in scene.");
        ImGui::End();
        return;
    }

    if (_selectedNode >= queryNodes.size()) {
        _selectedNode = 0;
    }

    ImGui::Text("Query Node:");
    if (ImGui::BeginCombo("##Query Node", queryNodes[_selectedNode]->getName().c_str())) {
        for (int i = 0 ; i < queryNodes.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Selectable(queryNodes[i]->getName().c_str(), _selectedNode == i))
                _selectedNode = i;

            if (_selectedNode == i)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImGui::SeparatorText("");

    auto& history = queryNodes[_selectedNode]->getHistory();

    // Max Position
    if (ImGui::CollapsingHeader("Max Position")) {
        plotVectorMetric("##maxPosition", "Position", history, [] (const Metrics& m) { return m.maxPosition; });
    }
    // Min Position
    if (ImGui::CollapsingHeader("Min Position")) {
        plotVectorMetric("##minPosition", "Position", history, [] (const Metrics& m) { return m.minPosition; });
    }
    // Average Position
    if (ImGui::CollapsingHeader("Avg Position")) {
        plotVectorMetric("##avgPosition", "Position", history, [] (const Metrics& m) { return m.avgPosition; });
    }

    // Average Velocity
    if (ImGui::CollapsingHeader("Velocity")) {
        plotVectorMetric("##Velocity", "Velocity", history, [] (const Metrics& m) { return m.velocity; });
    }

    // Pressure
    if (ImGui::CollapsingHeader("Pressure")) {
        plotFloatMetric("##Pressure", "Pressure", history, [] (const Metrics& m) { return m.pressure; });
    }

    ImGui::End();
}

void QueryDataView::plotVectorMetric(const char* id, const char* label, const std::vector<Metrics>& history, VectorMetricGetter getter) {
    if (ImPlot::BeginPlot(id, ImVec2(-1, 200))) {
        ImPlot::SetupAxes(
            "Frame",
            label,
            ImPlotAxisFlags_AutoFit,
            ImPlotAxisFlags_AutoFit
        );

        MetricData mData {
            .history = &history,
            .getterVector = getter
        };

        ImPlot::PlotLineG(
            "X",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).x
                );
            },
            &mData,
            history.size()
        );

        ImPlot::PlotLineG(
            "Y",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).y
                );
            },
            &mData,
            history.size()
        );

        ImPlot::PlotLineG(
            "Z",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).z
                );
            },
            &mData,
            history.size()
        );

        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();

            double x = std::round(mouse.x);

            ImPlot::DragLineX(
                0,
                &x,
                ImVec4(1, 1, 1, 0.5f),
                1.0f
            );

            int index = static_cast<int>(x);

            if (index >= 0 && index < static_cast<int>(history.size())) {
                glm::vec3 value = getter(history[index]);

                ImPlot::Annotation(
                    x,
                    value.x,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "X: %.3f",
                    value.x
                );

                ImPlot::Annotation(
                    x,
                    value.y,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "Y: %.3f",
                    value.y
                );

                ImPlot::Annotation(
                    x,
                    value.z,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "Z: %.3f",
                    value.z
                );
            }
        }

        ImPlot::EndPlot();
    }
}

void QueryDataView::plotFloatMetric(const char* id, const char* label, const std::vector<Metrics>& history, FloatMetricGetter getter) {
    if (ImPlot::BeginPlot(id, ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(
            "Frame",
            label,
            ImPlotAxisFlags_AutoFit,
            ImPlotAxisFlags_AutoFit
        );

        MetricData mData {
            .history = &history,
            .getter = getter
        };

        ImPlot::PlotLineG(
            label,
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getter(metric)
                );
            },
            &mData,
            history.size()
        );

        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();

            // Vertical cursor line
            double x = mouse.x;
            ImPlot::DragLineX(
                0,
                &x,
                ImVec4(1, 1, 1, 0.5f),
                1.0f
            );

            // Convert x to index
            int index = static_cast<int>(std::round(x));

            if (index >= 0 && index < history.size()) {
                float value = getter(history[index]);

                ImPlot::Annotation(
                    x,
                    value,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "%.3f",
                    value
                );
            }
        }

        ImPlot::EndPlot();
    }
}
