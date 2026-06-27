#ifndef EULERIANFLUIDSIMULATION_APPLICATION_H
#define EULERIANFLUIDSIMULATION_APPLICATION_H

#include "simulation/SimulationDisplay.h"
#include "simulator/ISimulator.h"
#include "views/Views.h"

class Application {
public:
    Application();
    ~Application();

    void update();

private:
    Window* _window{};
    ISimulator* _simulator{};
    ISimulationDisplay* _display{};
    Views* _views{};

    enum SimulatorType {
        CPU_2D,
        OPENGL_2D,
        CPU_3D,
    };
    void rebuild(SimulatorType type);
    bool _should_rebuild = false;

    void input_handler();
};


#endif //EULERIANFLUIDSIMULATION_APPLICATION_H