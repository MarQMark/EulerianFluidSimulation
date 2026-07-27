#ifndef SIMULATION2DVIEW_H
#define SIMULATION2DVIEW_H

#include "imgui/imgui.h"
#include "Viewable.h"

class Simulation2DView : public Viewable {
private:
    enum Mode{
        FORCE = 0,
        BOUNDARY,
        FLUID,
        END
    };
public:
    explicit Simulation2DView(unsigned int width, unsigned int height);

    void render(void* renderData) override;

private:
    bool _initialized = false;

    unsigned int _width;
    unsigned int _height;

    Mode _mode = FORCE;

    ImVec2 _mouse_pos = ImVec2(-1, -1);
};



#endif //SIMULATION2DVIEW_H
