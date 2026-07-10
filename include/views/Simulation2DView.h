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
    explicit Simulation2DView(ImTextureID textureID, int width, int height);

    void render(void* renderData) override;

private:
    bool _initialized = false;

    ImTextureID _texture_id;
    int _width;
    int _height;

    Mode _mode = FORCE;

    ImVec2 _mouse_pos = ImVec2(-1, -1);
};



#endif //SIMULATION2DVIEW_H
