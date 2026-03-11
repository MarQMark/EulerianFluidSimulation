#ifndef EULERIANFLUIDSIMULATION_VIEWS_H
#define EULERIANFLUIDSIMULATION_VIEWS_H
#include <map>
#include <string>

#include "Viewable.h"
#include "renderer/Window.h"


class Views {
public:
    explicit Views(const Window* window);
    ~Views();

    void render();

private:
    std::map<std::string, Viewable*> _views;
};


#endif //EULERIANFLUIDSIMULATION_VIEWS_H