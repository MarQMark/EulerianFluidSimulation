#ifndef SIMULATIONVIEW_H
#define SIMULATIONVIEW_H

#include "imgui/imgui.h"

#include "Viewable.h"


class SimulationView : public Viewable {
public:
    explicit SimulationView(ImTextureID textureID);
    ~SimulationView() = default;

    void render() override;

private:
    ImTextureID _texture_id;
};



#endif //SIMULATIONVIEW_H
