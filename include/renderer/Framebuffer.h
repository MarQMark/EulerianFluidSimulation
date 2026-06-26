//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_FRAMEBUFFER_H
#define FLUIDSIMSPH_FRAMEBUFFER_H

#include <vector>
#include "glm/vec2.hpp"
#include "glad/gl.h"

struct AttachmentFormat {
    GLuint internalFormat;
    GLuint format;
    GLuint type;
};

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    GLuint getID() const;

    void bind();
    static void unbind();

    GLuint addColorAttachment(GLuint filter, GLuint internalFormat = GL_RGB, GLuint format = GL_RGB, GLuint type = GL_UNSIGNED_BYTE);
    GLuint getColorAttachment(int index) const;

    void addDepthBuffer();
    void addDepthTexture();
    GLuint getDepthTexture() const;

    void resize(int width, int height);
    glm::vec2 getSize();

    bool checkStatus();

    static Framebuffer* getCurrent();

private:
    GLuint fbo = 0;
    GLuint rbo = 0;

    std::vector<GLuint> colorAttachments;
    std::vector<AttachmentFormat> attachments;

    GLuint depthTexture = 0;

    int width = -1, height = -1;

    static inline Framebuffer* currentBoundBuffer;
};

#endif //FLUIDSIMSPH_FRAMEBUFFER_H