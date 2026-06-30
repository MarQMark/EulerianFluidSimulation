#ifndef SIMULATION2DVIEW_H
#define SIMULATION2DVIEW_H

#include <vector>
#include <glm/vec2.hpp>

#include "imgui/imgui.h"

#include "Viewable.h"
#include "simulation/Grid.h"
#include "simulation/ScalField2D.h"
#include "simulation/VecField2D.h"


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

    void setVelField(VecField2D* velField);

private:
    bool _initialized = false;

    ImTextureID _texture_id;
    int _width;
    int _height;

    Mode _mode = FORCE;

    ImVec2 _mouse_pos = ImVec2(-1, -1);

    VecField2D* _vel_field{};
};



#endif //SIMULATION2DVIEW_H
