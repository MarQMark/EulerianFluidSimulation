#ifndef EULERIANFLUIDSIMULATION_SIMULATION3DVIEW_H
#define EULERIANFLUIDSIMULATION_SIMULATION3DVIEW_H

#include "Viewable.h"
#include "glm/glm.hpp"

class Renderer3D;
class CameraNode;
class Editor;

class Simulation3DView : public Viewable {
public:
    explicit Simulation3DView(Editor* editor, Renderer3D* renderer);

    void render(void* renderData) override;

private:
    Editor* editor;
    Renderer3D* renderer;

    glm::vec2 viewportSize = { 0, 0 };
    glm::vec2 mousePosition = { 0, 0 };
    bool dragging = false;
};

#endif //EULERIANFLUIDSIMULATION_SIMULATION3DVIEW_H
