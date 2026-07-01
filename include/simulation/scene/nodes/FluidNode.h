//
// Created by ruut on 6/28/26.
//

#ifndef EULERIANFLUIDSIMULATION_FLUIDNODE_H
#define EULERIANFLUIDSIMULATION_FLUIDNODE_H

#include "SceneNode.h"
#include "assets/Model.h"
#include "renderer/MarchingCubes.h"
#include "simulation/SimulationDisplay3D.h"

class FluidNode : public SceneNode {
public:
    FluidNode();
    ~FluidNode();

    void setRenderData(const SimulationDisplay3D::RenderData3D* data);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;
    NodeProperties buildProperties() override;

private:
    std::shared_ptr<Model> _fluidModel;
    const SimulationDisplay3D::RenderData3D* _renderData;

    MarchingCubes* _cubes;
};

#endif //EULERIANFLUIDSIMULATION_FLUIDNODE_H
