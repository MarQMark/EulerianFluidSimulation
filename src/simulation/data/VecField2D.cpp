#include "simulation/data/VecField2D.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "glm/common.hpp"

VecField2D::VecField2D(const unsigned int width, const unsigned int height) : _width(width), _height(height){
    _field.resize(width * height);
    clear();
}

VecField2D::VecField2D(const VecField2D &field, const bool copy) : _width(field.width()), _height(field.height()) {
    _field.resize(field.width() * field.height());
    if (copy)
        memcpy(_field.data(), field._field.data(), field.size() * sizeof(glm::vec2));
}


static inline glm::vec2 StaggeredOffset(const int comp) {
    return (comp == 0)  ? glm::vec2(-1.0f, -0.5f)
                        : glm::vec2(-0.5f, -1.0f);
}

float VecField2D::bilinear_interpolation(const glm::vec2 &pos, const int low_x, const int low_y, const int comp) {
    const glm::vec2 off = StaggeredOffset(comp);

    const float x0 = low_x + off.x;
    const float y0 = low_y + off.y;

    const float tx = pos.x - x0;
    const float ty = pos.y - y0;

    const float v00 = (*this)(low_x,     low_y    )[comp];
    const float v01 = (*this)(low_x,     low_y + 1)[comp];
    const float v10 = (*this)(low_x + 1, low_y    )[comp];
    const float v11 = (*this)(low_x + 1, low_y + 1)[comp];

    const float vLow  = v00 * (1.0f - ty) + v01 * ty;
    const float vHigh = v10 * (1.0f - ty) + v11 * ty;

    return vLow * (1.0f - tx) + vHigh * tx;
}

#define EPSILON 1.0e-10f

glm::vec2 VecField2D::interpolate(const glm::vec2 pos) {
    auto lowIndex = [](const float coord, const float offset) {
        return (int)(std::floor(coord - offset) + EPSILON);
    };

    const glm::vec2 offU = StaggeredOffset(0);
    const int low_x_u = lowIndex(pos.x, offU.x);
    const int low_y_u = lowIndex(pos.y, offU.y);
    const float vx = bilinear_interpolation(pos, low_x_u, low_y_u, 0);

    const glm::vec2 offV = StaggeredOffset(1);
    const int low_x_v = lowIndex(pos.x, offV.x);
    const int low_y_v = lowIndex(pos.y, offV.y);
    const float vy = bilinear_interpolation(pos, low_x_v, low_y_v, 1);

    return glm::vec2(vx, vy);
}
#undef EPSILON

void VecField2D::clear() {
    memset(_field.data(), 0, size() * sizeof(glm::vec2));
}

unsigned int VecField2D::size() const {
    return _width * _height;
}

unsigned int VecField2D::width() const {
    return _width;
}

unsigned int VecField2D::height() const {
    return _height;
}


