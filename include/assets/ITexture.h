//
// Created by ruut on 6/19/26.
//

#ifndef EULERIANFLUIDSIMULATION_ITEXTURE_H
#define EULERIANFLUIDSIMULATION_ITEXTURE_H

#include "glad/gl.h"

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual GLuint get() const = 0;
    virtual void bind(GLuint slot) const = 0;
    virtual void unbind() = 0;
};

#endif //EULERIANFLUIDSIMULATION_ITEXTURE_H
