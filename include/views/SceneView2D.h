#ifndef EULERIANFLUIDSIMULATION_SCENEVIEW2D_H
#define EULERIANFLUIDSIMULATION_SCENEVIEW2D_H

#include <string>
#include <vector>

#include "Viewable.h"
#include "simulation/data/Scene2D.h"

class SceneView2D : public Viewable {
public:
    explicit SceneView2D(Scene2D* scene);

    void render(void* renderData) override;

private:
    Scene2D* _scene;

    int _selected_scene = 0;
    std::vector<std::string> _scene_list{};
    void refresh_scene_list();

    bool _sl_properties = true;

    int _threshold = 100;
};

#endif //EULERIANFLUIDSIMULATION_SCENEVIEW2D_H