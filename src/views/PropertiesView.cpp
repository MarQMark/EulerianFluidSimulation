#include "views/PropertiesView.h"

#include <sstream>
#include <iostream>
#include <ostream>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "simulation/Properties.h"
#include "util/GuiHelper.h"

void PropertiesView::render(void* renderData) {
    if (!_visible)
        return;

    ImGui::Begin("Properties");

    if(ImGui::IsKeyPressed(ImGuiKey_Slash)) {
        _focus_search = true;
        _show_search = true;
        _search_shown = false;
        ImGui::SetWindowFocus("Properties");
    }

    if (_show_search)
        render_search();
    else
        render_categories();

    ImGui::End();
}

bool PropertiesView::searching() const {
    return _focused;
}

void PropertiesView::render_categories() {
    _focused = false;

    for (const auto& cat : Properties::Get()->getCategories()) {
        if (cat->empty())
            continue;
        if (ImGui::CollapsingHeader(cat->name.c_str())) {
            for (const auto& sub : cat->sub_categories) {
                if (sub->empty())
                    continue;
                Util::SectionHeader(sub->name.c_str());
                for (const auto& p : *sub)
                    p->draw();
            }
            if (!cat->sub_categories.empty())
                Util::SectionHeader("Ungrouped");
            for (const auto& p : *cat)
                p->draw();
        }
    }
}

void PropertiesView::render_search() {
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (_focus_search) {
        ImGui::SetKeyboardFocusHere();
        _focus_search = false;
    }
    ImGui::InputText("##Search", &_query, ImGuiInputTextFlags_AutoSelectAll);

    // This is buggy; ImGui sometimes loses IsItemActiveAsInputText randomly
    // It has likely something to do with how tooltips are new windows and make it momentarily lose active id
    _focused = ImGui::IsItemActiveAsInputText();
    if (!_focused && _query.empty() && _search_shown)
        _show_search = false;

    if (_focused)
        _search_shown = true;
    else if (!_search_shown)
        _focus_search = true;

    ImGui::Separator();

    std::vector<std::pair<std::string, Properties::IProperty*>> results;
    for (const auto& cat : Properties::Get()->getCategories()) {
        for (const auto& sub : cat->sub_categories) {
            for (const auto& p : *sub) {
                if (score_match(p->name, _query) >= _valid_threshold ||
                    score_match(p->description, _query) >= _valid_threshold) {
                    results.emplace_back(cat->name, p);
                }
            }
        }
        for (const auto& p : *cat) {
            if (score_match(p->name, _query) >= _valid_threshold ||
                score_match(p->description, _query) >= _valid_threshold) {
                results.emplace_back(cat->name, p);
            }
        }
    }

    for (int i = 0; i < results.size(); i++) {
        if (i == 0 || results[i - 1].first != results[i].first)
            Util::SectionHeader(results[i].first.c_str());
        results[i].second->draw();
    }
}

int PropertiesView::score_match(const std::string &s, const std::string &q) {
    std::istringstream ss(s);
    std::string word;
    int score = 0;

    if (s == q) return 100;
    if (s.rfind(q, 0) == 0) score += 50;

    while (ss >> word)
    {
        if (word == q)
            score += 40;
        else if (word.find(q) != std::string::npos)
            score += 10;
    }

    if (s.find(q) != std::string::npos)
        score += 20;

    return score;
}
