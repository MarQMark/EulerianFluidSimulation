//
// Created by ruut on 6/28/26.
//

#include "simulation/scene/nodes/FluidNode.h"
#include "renderer/MarchingCubes.h"
#include "simulation/data/Grid3D.h"
#include "simulation/Properties.h"
#include "simulation/data/OpenGLGrid3D.h"
#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/BoundingBoxNode.h"

FluidNode::FluidNode() {
    name = "Fluid";
    savable = false;
    gizmoEnabled = false;
    setDeletable(false);

    // --- Marching Cubes Model ---
    auto mcMesh = std::make_unique<Mesh>("FluidMesh", 1, 1);
    _marchingCubes = new MarchingCubes(mcMesh.get());
    _marchingCubesModel = std::make_shared<Model>("Fluid", std::move(mcMesh));

    // --- Cubes Model ---
    auto scMesh = std::make_unique<Mesh>("FluidMesh", 1, 1);
    _simpleCubes = new SimpleCubes(scMesh.get());
    _simpleCubesModel = std::make_shared<Model>("Fluid", std::move(scMesh));

    Material fluidMaterial("Fluid", Shader::createDefaultShaded());
    fluidMaterial.setVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
    _marchingCubesModel->addMaterial(fluidMaterial);
    _simpleCubesModel->addMaterial(fluidMaterial);

    updateMaterials();
}

FluidNode::~FluidNode() {
    delete _marchingCubes;
    delete _simpleCubes;
}

void FluidNode::setRenderData(const SimulationDisplay3D::RenderData3D* data) {
    _renderData = data;
}

void FluidNode::update(float deltaTime) {
    if (!_renderData)
        return;

    const StorageBuffer* gpuRho = nullptr;
    if (PROPERTY(SIMULATION_TYPE, Properties::SimulatorType) == Properties::SimulatorType::OPENGL_3D) {
        if (const auto* grid = dynamic_cast<OpenGLGrid3D*>(_renderData->grid))
            gpuRho = &grid->gpuRho();
    }

    if (_settings.mode == MARCHING_CUBES) {
        _marchingCubes->updateMesh(
            _renderData->grid->rho(),
            scene->getSimulationDomain()->getWorldBoundingBox(),
            gpuRho
        );
    }

    if (_settings.mode == CUBES) {
        _simpleCubes->update(
            _renderData->grid->rho(),
            scene->getSimulationDomain()->getWorldBoundingBox(),
            gpuRho
        );
    }
}

void FluidNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    if (!_renderData)
        return;

    if (_settings.mode == MARCHING_CUBES) {
        _marchingCubesModel->draw(view, projection, glm::mat4(1.0f));
    }

    if (_settings.mode == CUBES) {
        _simpleCubesModel->draw(view, projection, glm::mat4(1.0f));
    }
}

NodeProperties FluidNode::buildProperties() {
    NodeProperties properties = SceneNode::buildProperties();

    properties.addSection("Rendering");
    properties.addProperty(std::make_unique<ColorProperty>(
        "Color",
        [this] () { return _settings.color; },
        [this] (const glm::vec3 c) { _settings.color = c; updateMaterials(); }
    ));

    properties.addProperty(std::make_unique<EnumProperty<RenderMode>>(
        "Render Mode",
        [this] () { return _settings.mode; },
        [this] (const RenderMode c) { _settings.mode = c; },
        std::vector<std::pair<RenderMode, std::string>>{ { MARCHING_CUBES, "Marching Cubes" }, { CUBES, "Cubes" } }
    ));

    return properties;
}

std::string FluidNode::getTypeName() const {
    return "FluidNode";
}

std::string FluidNode::getDefaultName() const {
    return "Fluid";
}

void FluidNode::updateMaterials() const {
    _marchingCubesModel->getMaterial(0)->setVec3("color", _settings.color);
    _simpleCubesModel->getMaterial(0)->setVec3("color", _settings.color);
}
