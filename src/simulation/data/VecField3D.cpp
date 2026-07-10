#include "simulation/data/VecField3D.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "glm/common.hpp"

VecField3D::VecField3D(const unsigned int width, const unsigned int height, const unsigned int depth) : _width(width), _height(height), _depth(depth) {
    _field.resize(width * height * depth);
    clear();
}

VecField3D::VecField3D(const VecField3D &field, const bool copy) : _width(field.width()), _height(field.height()), _depth(field.depth()) {
    _field.resize(field.width() * field.height() * field.depth());
    if (copy)
        memcpy(_field.data(), field._field.data(), field.size() * sizeof(glm::vec3));
}

static inline glm::vec3 StaggeredOffset3D(const int comp) {
    switch (comp) {
        case 0:  return glm::vec3(-1.0f, -0.5f, -0.5f);
        case 1:  return glm::vec3(-0.5f, -1.0f, -0.5f);
        case 2:
        default: return glm::vec3(-0.5f, -0.5f, -1.0f);
    }
}

float VecField3D::trilinear_interpolation(const glm::vec3 &pos, const int low_x, const int low_y, const int low_z, const int comp) {
    const glm::vec3 off = StaggeredOffset3D(comp);

    const float x0 = low_x + off.x;
    const float y0 = low_y + off.y;
    const float z0 = low_z + off.z;

    const float tx = pos.x - x0;
    const float ty = pos.y - y0;
    const float tz = pos.z - z0;

    const float v000 = (*this)(low_x,     low_y,     low_z    )[comp];
    const float v001 = (*this)(low_x,     low_y,     low_z + 1)[comp];
    const float v010 = (*this)(low_x,     low_y + 1, low_z    )[comp];
    const float v011 = (*this)(low_x,     low_y + 1, low_z + 1)[comp];
    const float v100 = (*this)(low_x + 1, low_y,     low_z    )[comp];
    const float v101 = (*this)(low_x + 1, low_y,     low_z + 1)[comp];
    const float v110 = (*this)(low_x + 1, low_y + 1, low_z    )[comp];
    const float v111 = (*this)(low_x + 1, low_y + 1, low_z + 1)[comp];

    const float v00 = v000 * (1.0f - tz) + v001 * tz;
    const float v01 = v010 * (1.0f - tz) + v011 * tz;
    const float v10 = v100 * (1.0f - tz) + v101 * tz;
    const float v11 = v110 * (1.0f - tz) + v111 * tz;

    const float vLow  = v00 * (1.0f - ty) + v01 * ty;
    const float vHigh = v10 * (1.0f - ty) + v11 * ty;

    return vLow * (1.0f - tx) + vHigh * tx;
}

#define EPSILON 1.0e-10f

glm::vec3 VecField3D::interpolate(const glm::vec3 pos) {
    auto lowIndex = [](const float coord, const float offset) {
        return (int)(std::floor(coord - offset) + EPSILON);
    };

    const glm::vec3 offU = StaggeredOffset3D(0);
    const int lx_u = lowIndex(pos.x, offU.x);
    const int ly_u = lowIndex(pos.y, offU.y);
    const int lz_u = lowIndex(pos.z, offU.z);
    const float vx = trilinear_interpolation(pos, lx_u, ly_u, lz_u, 0);

    const glm::vec3 offV = StaggeredOffset3D(1);
    const int lx_v = lowIndex(pos.x, offV.x);
    const int ly_v = lowIndex(pos.y, offV.y);
    const int lz_v = lowIndex(pos.z, offV.z);
    const float vy = trilinear_interpolation(pos, lx_v, ly_v, lz_v, 1);

    const glm::vec3 offW = StaggeredOffset3D(2);
    const int lx_w = lowIndex(pos.x, offW.x);
    const int ly_w = lowIndex(pos.y, offW.y);
    const int lz_w = lowIndex(pos.z, offW.z);
    const float vz = trilinear_interpolation(pos, lx_w, ly_w, lz_w, 2);

    return glm::vec3(vx, vy, vz);
}

#undef EPSILON

void VecField3D::clear() {
    memset(_field.data(), 0, size() * sizeof(glm::vec3));
}

unsigned int VecField3D::size() const {
    return _width * _height * _depth;
}

unsigned int VecField3D::width() const {
    return _width;
}

unsigned int VecField3D::height() const {
    return _height;
}

unsigned int VecField3D::depth() const {
    return _depth;
}