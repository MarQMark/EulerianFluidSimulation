//
// Created by ruut on 6/6/26.
//

#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/SkyboxNode.h"

#include "simulation/scene/NodeProperties.h"

SkyboxNode::SkyboxNode() {
    name = "Skybox";
    deletable = false;
    savable = false;
    gizmoEnabled = false;

    // --- Skybox Model ---
    skyboxModel = std::make_shared<Model>("Skybox", Mesh::createBox("Skybox Mesh"));
    skyboxTexture = std::make_shared<Texture2D>("assets/textures/skybox1.png");

    Material skyboxMaterial("Skybox", Shader::createSkybox());
    skyboxMaterial.setTexture("skybox", skyboxTexture);
    this->skyboxShader = skyboxMaterial.getShader();
    skyboxModel->addMaterial(skyboxMaterial);
}

NodeProperties SkyboxNode::buildProperties() {
    NodeProperties properties = SceneNode::buildProperties();

    properties.addSection("Skybox");
    properties.addProperty(std::make_unique<Texture2DProperty>(
        "Texture",
        [this] () { return skyboxTexture; },
        [this] (std::shared_ptr<Texture2D> tex)
        {
            skyboxTexture = tex;
            skyboxModel->getMaterial(0)->setTexture("skybox", skyboxTexture);
        }
    ));

    return properties;
}

void SkyboxNode::update(float deltaTime) { }

void SkyboxNode::draw(const glm::mat4& view, const glm::mat4& projection) const {
    glFrontFace(GL_CW);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    skyboxModel->draw(view, scene->getCamera()->getPerspectiveProjectionMatrix(), glm::mat4(1.0f));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
}

int SkyboxNode::getRenderPriority() const {
    return -1;
}

std::string SkyboxNode::getDefaultName() const {
    return "Skybox";
}
