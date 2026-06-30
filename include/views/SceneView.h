//
// Created by ruut on 6/30/26.
//

#ifndef EULERIANFLUIDSIMULATION_SCENEVIEW_H
#define EULERIANFLUIDSIMULATION_SCENEVIEW_H

#include "Viewable.h"

class Renderer3D;
class Editor;

class SceneView : public Viewable {
public:
    SceneView(Editor* editor, Renderer3D* renderer);

    void render(void* renderData) override;

private:
    Editor* _editor;
    Renderer3D* _renderer;
};

#endif //EULERIANFLUIDSIMULATION_SCENEVIEW_H
