#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include <vector>
#include <glm/vec2.hpp>

#include "imgui/imgui.h"

#include "Viewable.h"


class SimulationView : public Viewable {
public:
    explicit SimulationView(ImTextureID textureID, int width, int height);
    ~SimulationView() = default;

    void render() override;

    std::vector<glm::vec2>& getForces();

private:
    ImTextureID _texture_id;
    int _width;
    int _height;

    float _size = 70.f;
    ImVec2 _mouse_pos = ImVec2(-1, -1);
    std::vector<glm::vec2> _forces;
};



#endif //SIMULATIONVIEW_H
