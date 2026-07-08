//
// Created by ruut on 6/28/26.
//

#ifndef EULERIANFLUIDSIMULATION_FLUIDNODE_H
#define EULERIANFLUIDSIMULATION_FLUIDNODE_H

#include "SceneNode.h"
#include "assets/Model.h"
#include "renderer/MarchingCubes.h"
#include "renderer/SimpleCubes.h"
#include "simulation/SimulationDisplay3D.h"

enum RenderMode {
    MARCHING_CUBES,
    CUBES
};

struct RenderingSettings {
    RenderMode mode = RenderMode::MARCHING_CUBES;

    glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.6f);
};

class FluidNode : public SceneNode {
public:
    FluidNode();
    ~FluidNode();

    void setRenderData(const SimulationDisplay3D::RenderData3D* data);

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;
    NodeProperties buildProperties() override;
    std::string getTypeName() const override;
    std::string getDefaultName() const override;

private:
    void updateMaterials() const;

    const SimulationDisplay3D::RenderData3D* _renderData;

    std::shared_ptr<Model> _marchingCubesModel;
    std::shared_ptr<Model> _simpleCubesModel;

    MarchingCubes* _marchingCubes;
    SimpleCubes* _simpleCubes;

    RenderingSettings _settings;
};

#endif //EULERIANFLUIDSIMULATION_FLUIDNODE_H
