#ifndef EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H
#define EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H

#include <string>

#include "core/Editor.h"
#include "renderer/Renderer2D.h"
#include "renderer/Renderer3D.h"

class Viewable;
class Window;

class Grid;
class Simulation2DView;

class ISimulationDisplay {
protected:
    ISimulationDisplay() = default;

public:
    virtual ~ISimulationDisplay() = default;

    virtual void update(void* renderData) = 0;

    virtual Viewable* createViewable() = 0;
    virtual std::string_view viewableName() = 0;
};

class SimulationDisplay2D final : public ISimulationDisplay {
public:
    struct RenderData2D {
        Grid* grid;
    };
    SimulationDisplay2D(unsigned int width, unsigned int height);
    ~SimulationDisplay2D() override;

    void update(void* renderData) override;

    Viewable* createViewable() override;
    std::string_view viewableName() override;

private:
    Renderer2D* _renderer{};
    Simulation2DView* _simulation_view{};
};

class SimulationDisplay3D final : public ISimulationDisplay {
public:
    struct RenderData3D {
        // ScalField3D for fluid
    };

    SimulationDisplay3D(Window* window);
    ~SimulationDisplay3D() override;

    void update(void* renderData) override;

    Viewable* createViewable() override;
    std::string_view viewableName() override;

private:
    Renderer3D* _renderer{};
    Scene* _scene{};
    Editor* _editor{};
};


#endif //EULERIANFLUIDSIMULATION_SIMULATIONDISPLAY_H