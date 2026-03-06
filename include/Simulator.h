#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H
#include "renderer/Window.h"


class Simulator {
public:
    Simulator();
    ~Simulator();

    void update();

private:
    Window* _window{};
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H