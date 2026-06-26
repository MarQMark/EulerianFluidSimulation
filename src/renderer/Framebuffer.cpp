//
// Created by ruut on 3/14/26.
//

#include "renderer/Framebuffer.h"

Framebuffer::Framebuffer(int width, int height) : width(width), height(height) {
    glGenFramebuffers(1, &fbo);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &fbo);

    for (auto tex : colorAttachments)
        glDeleteTextures(1, &tex);

    if (rbo)
        glDeleteRenderbuffers(1, &rbo);
}

GLuint Framebuffer::getID() const { return fbo; }
void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    currentBoundBuffer = this;
}
void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    currentBoundBuffer = nullptr;
}

GLuint Framebuffer::addColorAttachment(GLuint filter, GLuint internalFormat, GLuint format, GLuint type ) {
    bind();

    // Create new texture add attach
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        format,
        type,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0 + colorAttachments.size(),
        GL_TEXTURE_2D,
        texture,
        0
    );

    colorAttachments.push_back(texture);
    attachments.push_back({ internalFormat, format, type });

    std::vector<GLenum> buffers;
    for (size_t i = 0; i < colorAttachments.size(); i++)
        buffers.push_back(GL_COLOR_ATTACHMENT0 + i);

    glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());

    unbind();
    return texture;
}

GLuint Framebuffer::getColorAttachment(int index) const {
    if (index >= 0 && index < colorAttachments.size())
        return colorAttachments[index];
    return -1;
}

void Framebuffer::addDepthBuffer() {
    bind();

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        rbo);

    unbind();
}

void Framebuffer::addDepthTexture() {
    bind();

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT32F,
        width,
        height,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depthTexture,
        0
    );

    unbind();
}

GLuint Framebuffer::getDepthTexture() const {
    return depthTexture;
}

void Framebuffer::resize(int width, int height) {
    this->width = width;
    this->height = height;

    bind();

    // Resize color attachments
    for (int i = 0 ; i < colorAttachments.size(); i++) {
        auto fmt = attachments[i];

        glBindTexture(GL_TEXTURE_2D, colorAttachments[i]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            fmt.internalFormat,
            width,
            height,
            0,
            fmt.format,
            fmt.type,
            nullptr
        );
    }

    if (depthTexture) {
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_DEPTH_COMPONENT24,
            width,
            height,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }

    // Resize depth buffer if existing
    if (rbo) {
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );
    }

    unbind();
}

glm::vec2 Framebuffer::getSize() {
    return { width, height };
}

bool Framebuffer::checkStatus() {
    bind();
    bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    unbind();
    return ok;
}

Framebuffer* Framebuffer::getCurrent() {
    return currentBoundBuffer;
}
