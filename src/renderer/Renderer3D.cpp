#include "renderer/Renderer3D.h"
#include "renderer/Framebuffer.h"
#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/SceneNode.h"

Renderer3D::Renderer3D() {
    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->bind();
    framebuffer->addColorAttachment(GL_LINEAR);
    framebuffer->addDepthTexture();
}

Renderer3D::~Renderer3D() {
    delete framebuffer;
}

void Renderer3D::render(const Scene* scene) const {
    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera transform
    auto camera = scene->getCamera();
    auto view = camera->getViewMatrix();
    auto projection = camera->getProjectionMatrix();

    auto nodes = scene->getSceneNodes();

    std::ranges::sort(nodes, [](const SceneNode* a, const SceneNode* b) {
        return a->getRenderPriority() < b->getRenderPriority();
    });

    for (const auto& node : nodes)
        renderNode(node, view, projection);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer3D::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

void Renderer3D::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

void Renderer3D::renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const {
    if (!node->isVisible())
        return;

    node->draw(view, projection);

    for (const auto& child : node->getChildren())
        renderNode(child, view, projection);
}
