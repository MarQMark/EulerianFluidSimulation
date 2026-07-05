//
// Created by ruut on 6/6/26.
//

#ifndef SPHLASH_SKYBOXNODE_H
#define SPHLASH_SKYBOXNODE_H

#include "assets/Model.h"
#include "assets/Texture2D.h"
#include "SceneNode.h"

class SkyboxNode : public SceneNode {
public:
    SkyboxNode();

    NodeProperties buildProperties() override;

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    int getRenderPriority() const override;
    std::string getDefaultName() const override;

private:
    std::shared_ptr<Model> skyboxModel;
    Shader* skyboxShader;
    std::shared_ptr<Texture2D> skyboxTexture;

    glm::mat4 skyboxPerspective;
};

#endif //SPHLASH_SKYBOXNODE_H
