#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY2D_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY2D_H

#include "ISimulationDisplay.h"
#include "renderer/Renderer2D.h"

class Grid2D;
class Viewable;
class Window;

class SimulationDisplay2D final : public ISimulationDisplay {
public:
    struct RenderData2D {
        IGrid2D* grid;
        unsigned int textureId;
    };

    SimulationDisplay2D(unsigned int width, unsigned int height);
    ~SimulationDisplay2D() override;

    void render(void* renderData) override;

private:
    unsigned int _width;
    unsigned int _height;
    Renderer2D* _renderer{};
};

#endif //EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H