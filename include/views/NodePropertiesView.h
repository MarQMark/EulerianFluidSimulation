//
// Created by ruut on 6/30/26.
//

#ifndef EULERIANFLUIDSIMULATION_NODEPROPERTIESVIEW_H
#define EULERIANFLUIDSIMULATION_NODEPROPERTIESVIEW_H

#include "Viewable.h"
#include "simulation/scene/NodeProperties.h"

class ISelectable;
class Editor;

class NodePropertiesView : public Viewable {
public:
    NodePropertiesView(Editor* editor);

    void render(void* renderData) override;

private:
    Editor* _editor;
    ISelectable* _selected;
    NodeProperties _properties;
};

#endif //EULERIANFLUIDSIMULATION_NODEPROPERTIESVIEW_H
