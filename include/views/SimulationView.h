#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include <vector>
#include <glm/vec2.hpp>

#include "imgui/imgui.h"

#include "Viewable.h"
#include "simulation/VecField.h"


class SimulationView : public Viewable {
public:
    explicit SimulationView(ImTextureID textureID, int width, int height);
    ~SimulationView();

    void render() override;

    VecField& getForces() const;

private:
    ImTextureID _texture_id;
    int _width;
    int _height;

    ImVec2 _mouse_pos = ImVec2(-1, -1);
    VecField* _forces{};
};



#endif //SIMULATIONVIEW_H
