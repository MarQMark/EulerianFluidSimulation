//
// Created by ruut on 6/19/26.
//

#ifndef EULERIANFLUIDSIMULATION_RENDERER3D_H
#define EULERIANFLUIDSIMULATION_RENDERER3D_H

#include "glad/gl.h"
#include <glm/glm.hpp>

class SceneNode;
class Scene;
class Framebuffer;

class Renderer3D {
public:
    Renderer3D();
    ~Renderer3D();

    void render(const Scene* scene) const;

    // Framebuffer texture
    GLuint getTextureId() const;
    void setSize(int width, int height) const;

private:
    void renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const;

    Framebuffer* framebuffer;
};

#endif //EULERIANFLUIDSIMULATION_RENDERER3D_H
