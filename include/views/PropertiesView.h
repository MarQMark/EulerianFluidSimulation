#ifndef EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H
#define EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H

#include <string>

#include "Viewable.h"

class PropertiesView : public Viewable {
public:
    PropertiesView() = default;
    ~PropertiesView() = default;

    void render() override;

    bool searching() const;

private:
    bool _show_search = false;
    bool _focus_search = false;
    bool _focused = false;
    std::string _query = "";
    const int _valid_threshold = 30;

    void render_categories();
    void render_search();

    int score_match(const std::string& s, const std::string& q);
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H