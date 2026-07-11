#include "renderer/Renderer3D.h"
#include "renderer/Framebuffer.h"
#include "simulation/scene/Scene.h"
#include "simulation/scene/nodes/SceneNode.h"
#include "simulation/data/VecField3D.h"
#include "simulation/data/ScalField3D.h"

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

void Renderer3D::drawField(std::vector<float> &data, VecField3D &field, const int dim, const int level, const int chanel, float &max) {
    for(const auto v : field)
        max = std::max(max, glm::length(v));

    const unsigned int sizes[3] = {field.width(), field.height(), field.depth()};
    const unsigned int used_sizes[2] = {sizes[(dim + 1) % 3], sizes[(dim + 2) % 3]};
    for (unsigned int i = 0; i < used_sizes[0]; i++) {
        for (unsigned int j = 0; j < used_sizes[1]; j++) {
            const unsigned int idx = j * used_sizes[0] + i;

            data[idx * 4 + 0] = 0;
            data[idx * 4 + 1] = 0;
            data[idx * 4 + 2] = 0;
            data[idx * 4 + 3] = 1.f;

            if (dim == 0)
                data[idx * 4 + chanel] = glm::length(field(level, i, j)) / max;
            else if (dim == 1)
                data[idx * 4 + chanel] = glm::length(field(j, level, i)) / max;
            else if (dim == 2)
                data[idx * 4 + chanel] = glm::length(field(i, j, level)) / max;
        }
    }
}

void Renderer3D::drawField(std::vector<float> &data, ScalField3D &field, const int dim, const int level, const int chanel, float &max) {
    const unsigned int sizes[3] = {field.width(), field.height(), field.depth()};
    const unsigned int used_sizes[2] = {sizes[(dim + 1) % 3], sizes[(dim + 2) % 3]};
    for (unsigned int j = 0; j < used_sizes[1]; j++) {
    for (unsigned int i = 0; i < used_sizes[0]; i++) {
        float value = 0;
        if (dim == 0)
            value = field(level, i, j);
        else if (dim == 1)
            value = field(j, level, i);
        else if (dim == 2)
            value = field(i, j, level);
        max = std::max(max, glm::length(value));
    }}

    for (unsigned int j = 0; j < used_sizes[1]; j++) {
    for (unsigned int i = 0; i < used_sizes[0]; i++) {
        const unsigned int idx = j * used_sizes[0] + i;

        data[idx * 4 + 0] = 0;
        data[idx * 4 + 1] = 0;
        data[idx * 4 + 2] = 0;
        data[idx * 4 + 3] = 1.f;

        float value = 0;
        if (dim == 0)
            value = field(level, i, j);
        else if (dim == 1)
            value = field(j, level, i);
        else if (dim == 2)
            value = field(i, j, level);

        if (value > 0)
            data[idx * 4 + chanel] = value / max;
        else
            data[idx * 4 + ((chanel + 1) % 3)] = std::abs(value) / max;

        if (!std::isfinite(value))
            data[idx * 4 + ((chanel + 1) % 3)] = 1.f;
    }}
}

void Renderer3D::renderNode(const SceneNode* node, const glm::mat4& view, const glm::mat4& projection) const {
    if (!node->isVisible())
        return;

    node->draw(view, projection);

    for (const auto& child : node->getChildren())
        renderNode(child, view, projection);
}
