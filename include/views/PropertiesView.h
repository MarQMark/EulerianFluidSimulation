#ifndef EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H
#define EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H

#include "Viewable.h"

class PropertiesView : public Viewable {
public:
    PropertiesView() = default;
    ~PropertiesView() = default;

    auto render() -> void override;

private:
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIESVIEW_H