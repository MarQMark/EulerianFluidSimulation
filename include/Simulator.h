#ifndef EULERIANFLUIDSIMULATION_SIMULATOR_H
#define EULERIANFLUIDSIMULATION_SIMULATOR_H

#include "renderer/Window.h"
#include "views/Views.h"


class Simulator {
public:
    Simulator();
    ~Simulator();

    void update();

private:
    Window* _window{};
    Views* _views{};
};


#endif //EULERIANFLUIDSIMULATION_SIMULATOR_H