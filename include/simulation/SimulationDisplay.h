#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H

#include <string>

#include "core/Editor.h"
#include "renderer/Renderer2D.h"
#include "renderer/Renderer3D.h"

class Viewable;
class Window;

class Grid;

class ISimulationDisplay {
protected:
    ISimulationDisplay() = default;
    std::map<std::string, Viewable*> _viewables;

public:
    virtual ~ISimulationDisplay() = default;

    virtual void update(void* renderData) = 0;

    virtual const std::map<std::string, Viewable*>& viewables() {
        return _viewables;
    };
};

class SimulationDisplay2D final : public ISimulationDisplay {
public:
    struct RenderData2D {
        Grid* grid;
    };
    SimulationDisplay2D(unsigned int width, unsigned int height);
    ~SimulationDisplay2D() override;

    void update(void* renderData) override;

private:
    unsigned int _width;
    unsigned int _height;
    Renderer2D* _renderer{};
};

class SimulationDisplay3D final : public ISimulationDisplay {
public:
    struct RenderData3D {
        // ScalField3D for fluid
    };

    SimulationDisplay3D(Window* window);
    ~SimulationDisplay3D() override;

    void update(void* renderData) override;

private:
    Renderer3D* _renderer{};
    Scene* _scene{};
    Editor* _editor{};
};


#endif //EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H