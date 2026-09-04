#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY2D_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY2D_H

#include "ISimulationDisplay.h"
#include "renderer/Renderer2D.h"

class Scene2D;
class Grid2D;
class Viewable;
class Window;

class SimulationDisplay2D final : public ISimulationDisplay {
public:
    struct RenderData2D {
        const int dim = 2;
        IGrid2D* grid;
        unsigned int textureId;
    };

    SimulationDisplay2D(unsigned int width, unsigned int height);
    ~SimulationDisplay2D() override;

    void render(void* renderData) override;
    void reset(void* simulationData) override;

private:
    unsigned int _width;
    unsigned int _height;
    Renderer2D* _renderer{};
    Scene2D* _scene{};
};

#endif //EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H