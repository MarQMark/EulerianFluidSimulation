//
// Created by ruut on 6/30/26.
//

#ifndef EULERIANFLUIDSIMULATION_SCENEVIEW_H
#define EULERIANFLUIDSIMULATION_SCENEVIEW_H

#include <string>
#include <vector>

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

    int _selected_scene = 0;
    std::vector<std::string> _scene_list{};
    void refresh_scene_list();
};

#endif //EULERIANFLUIDSIMULATION_SCENEVIEW_H
