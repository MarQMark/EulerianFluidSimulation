#ifndef SIMULATION2DVIEW_H
#define SIMULATION2DVIEW_H

#include <vector>
#include <glm/vec2.hpp>

#include "imgui/imgui.h"

#include "Viewable.h"
#include "simulation/Grid.h"
#include "simulation/ScalField.h"
#include "simulation/VecField.h"


class Simulation2DView : public Viewable {
private:
    enum Mode{
        FORCE = 0,
        BOUNDARY,
        FLUID,
        END
    };
public:
    explicit Simulation2DView() = default;

    void render() override;

    void recreate(ImTextureID textureID, const Grid* grid);

    void setVelField(VecField* velField);

private:
    bool _initialized = false;

    ImTextureID _texture_id;
    int _width;
    int _height;

    Mode _mode = FORCE;

    ImVec2 _mouse_pos = ImVec2(-1, -1);
    VecField* _forces{};

    VecField* _vel_field{};
    ScalField* _boundary{};
    ScalField* _rho{};
};



#endif //SIMULATION2DVIEW_H
