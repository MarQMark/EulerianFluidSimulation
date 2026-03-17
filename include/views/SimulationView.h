#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include "imgui/imgui.h"

#include "Viewable.h"


class SimulationView : public Viewable {
public:
    explicit SimulationView(ImTextureID textureID, int width, int height);
    ~SimulationView() = default;

    void render() override;

private:
    ImTextureID _texture_id;
    int _width;
    int _height;
};



#endif //SIMULATIONVIEW_H
