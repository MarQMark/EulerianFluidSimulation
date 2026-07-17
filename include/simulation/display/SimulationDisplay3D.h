//
// Created by ruut on 7/1/26.
//

#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY3D_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY3D_H

#include "ISimulationDisplay.h"

class Editor;
class Scene;
class Renderer3D;
class Window;
class IGrid3D;

class SimulationDisplay3D final : public ISimulationDisplay {
public:
    struct RenderData3D {
        IGrid3D* grid;
    };

    SimulationDisplay3D(Window* window);
    ~SimulationDisplay3D() override;

    void update(void* simulationData) override;
    void render(void* renderData) override;
    void reset(void* simulationData) override;

private:
    Renderer3D* _renderer{};
    Scene* _scene{};
    Editor* _editor{};
};

#endif //EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY3D_H
