#ifndef EULERIANFLUIDSIMULATION_VIEWS_H
#define EULERIANFLUIDSIMULATION_VIEWS_H
#include <iostream>
#include <map>
#include <string>

#include "Viewable.h"
#include "renderer/Window.h"


class Views {
public:
    explicit Views(const Window* window);
    ~Views();

    void update();

    void addView(const std::string& name, Viewable* view);
    Viewable* getView(const std::string& name);
    void replaceView(const std::string& name, Viewable* view);
    void deleteView(const std::string& name);

    template<class T = Viewable>
    T* getView(const std::string& name) {
        if(!_views.contains(name)) {
            std::cout << "[Warning] View with name " << name << " not in views map\n";
            return nullptr;
        }

        return  dynamic_cast<T*>(_views[name]);
    };

private:
    std::map<std::string, Viewable*> _views;
};


#endif //EULERIANFLUIDSIMULATION_VIEWS_H