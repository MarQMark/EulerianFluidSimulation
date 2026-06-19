#include "assets/Texture2D.h"
#include "stb/stb_image.h"
#include "util/Logger.h"

Texture2D::Texture2D(const std::string& path, Options *opt) {
    if(!opt){
        opt = new Options();
    }

    glGenTextures(1, &_id);
    Texture2D::bind(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, opt->minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, opt->maxFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, opt->warpS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, opt->warpT);

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &_width, &_height, &_nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;

        if (_nrChannels == 1)
            format = GL_RED;
        else if (_nrChannels == 3)
            format = GL_RGB;
        else if (_nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            _width,
            _height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );

        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        LOG_ERROR("Failed to load texture: " + path);
    }

    stbi_image_free(data);
    Texture2D::unbind();

    _opt = opt;
}

Texture2D::Texture2D(GLsizei width, GLsizei height, uint8_t *data, Texture2D::Options *opt) : _width(width), _height(height){
    if(!opt){
        opt = new Options();
    }

    glGenTextures(1, &_id);
    Texture2D::bind(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, opt->minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, opt->maxFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, opt->warpS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, opt->warpT);

    glTexImage2D(GL_TEXTURE_2D, opt->level, opt->internalformat, width, height, 0, opt->format, opt->type, data);
    Texture2D::unbind();

    _opt = opt;
}

Texture2D::~Texture2D() {
    glDeleteTextures(1, &_id);
    delete _opt;
}

void Texture2D::bind(GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, _id);
}

void Texture2D::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture2D::get() const {
    return _id;
}

void Texture2D::update(uint8_t *data) {
    bind(0);
    glTexImage2D(GL_TEXTURE_2D, _opt->level, _opt->internalformat, _width, _height, 0, _opt->format, _opt->type, data);
}

GLsizei Texture2D::getWidth() const {
    return _width;
}

GLsizei Texture2D::getHeight() const {
    return _height;
}
