#ifndef EULERIANFLUIDSIMULATION_TEXTURE2D_H
#define EULERIANFLUIDSIMULATION_TEXTURE2D_H

#include <string>

#include "ITexture.h"
#include "glad/gl.h"

class Texture2D : public ITexture {
public:
    struct Options {
        GLint level = 0;
        GLint internalformat = GL_RGBA;
        GLenum format = GL_RGBA;
        GLenum type = GL_FLOAT;

        GLint minFilter = GL_NEAREST;
        GLint maxFilter = GL_NEAREST;
        GLint warpS = GL_CLAMP_TO_EDGE;
        GLint warpT = GL_CLAMP_TO_EDGE;
    };

    Texture2D(const std::string& path, Options *opt = nullptr);
    Texture2D(GLsizei width, GLsizei height, uint8_t* data, Options* opt = nullptr);
    ~Texture2D() override;

    GLuint get() const override;

    void update(uint8_t* data);

    void bind(GLuint slot) const override;
    void unbind() override;

    GLsizei getWidth() const;
    GLsizei getHeight() const;
private:
    GLuint _id;
    GLsizei _width;
    GLsizei _height;
    int _nrChannels;

    Options* _opt{};
};

#endif //EULERIANFLUIDSIMULATION_TEXTURE2D_H
