#include "simulation/data/ScalField3D.h"

#include <algorithm>
#include <cmath>
#include <cstring>

ScalField3D::ScalField3D(const unsigned int width, const unsigned int height, const unsigned int depth) : _width(width), _height(height), _depth(depth) {
    _field.resize(_width * _height * _depth);
    clear();
}

ScalField3D::ScalField3D(const ScalField3D &field, const bool copy ) : _width(field.width()), _height(field.height()), _depth(field.depth()){
    _field.resize(_width * _height * _depth);
    if (copy)
        memcpy(_field.data(), field._field.data(), field.size() * sizeof(float));
}

float ScalField3D::interpolate(glm::vec3 pos) const {
    pos += glm::vec3(1.f);
    const int x = (int)pos.x;
    const int y = (int)pos.y;
    const int z = (int)pos.z;

    int ix1 = x;
    int ix2 = x + 1;
    if (pos.x - (float)x < 0.5f) {
        ix1 = x - 1;
        ix2 = x;
    }
    int iy1 = y;
    int iy2 = y + 1;
    if (pos.y - (float)y < 0.5f) {
        iy1 = y - 1;
        iy2 = y;
    }
    int iz1 = z;
    int iz2 = z + 1;
    if (pos.z - (float)z < 0.5f) {
        iz1 = z - 1;
        iz2 = z;
    }

    ix1 = std::clamp(ix1, 0, (int)_width  - 1);
    ix2 = std::clamp(ix2, 0, (int)_width  - 1);
    iy1 = std::clamp(iy1, 0, (int)_height - 1);
    iy2 = std::clamp(iy2, 0, (int)_height - 1);
    iz1 = std::clamp(iz1, 0, (int)_depth  - 1);
    iz2 = std::clamp(iz2, 0, (int)_depth  - 1);

    const float v[8] = {
        (*this)(ix1, iy1, iz1),
        (*this)(ix1, iy1, iz2),
        (*this)(ix1, iy2, iz1),
        (*this)(ix1, iy2, iz2),
        (*this)(ix2, iy1, iz1),
        (*this)(ix2, iy1, iz2),
        (*this)(ix2, iy2, iz1),
        (*this)(ix2, iy2, iz2),
    };

    const float x1 = (float)ix1 + 0.5f;
    const float x2 = (float)ix2 + 0.5f;
    const float y1 = (float)iy1 + 0.5f;
    const float y2 = (float)iy2 + 0.5f;
    const float z1 = (float)iz1 + 0.5f;
    const float z2 = (float)iz2 + 0.5f;

    const float dz2z1 = z2 - z1;
    float f00, f01, f10, f11;
    if (dz2z1 < 1e-9f) {
        f00 = v[1];
        f01 = v[3];
        f10 = v[5];
        f11 = v[7];
    } else {
        const float wz1 = (z2 - pos.z) / dz2z1;
        const float wz2 = (pos.z - z1) / dz2z1;
        f00 = wz1 * v[0] + wz2 * v[1];
        f01 = wz1 * v[2] + wz2 * v[3];
        f10 = wz1 * v[4] + wz2 * v[5];
        f11 = wz1 * v[6] + wz2 * v[7];
    }

    const float dy2y1 = y2 - y1;
    float fx1, fx2; // at ix1, ix2
    if (dy2y1 < 1e-9f) {
        fx1 = f01;
        fx2 = f11;
    } else {
        const float wy1 = (y2 - pos.y) / dy2y1;
        const float wy2 = (pos.y - y1) / dy2y1;
        fx1 = wy1 * f00 + wy2 * f01;
        fx2 = wy1 * f10 + wy2 * f11;
    }

    const float dx2x1 = x2 - x1;
    float fxyz;
    if (dx2x1 < 1e-9f) {
        fxyz = fx2;
    } else {
        const float wx1 = (x2 - pos.x) / dx2x1;
        const float wx2 = (pos.x - x1) / dx2x1;
        fxyz = wx1 * fx1 + wx2 * fx2;
    }

    return fxyz;
}

void ScalField3D::clear() {
    memset(_field.data(), 0, size() * sizeof(float));
}

unsigned int ScalField3D::size() const {
    return _width * _height * _depth;
}

unsigned int ScalField3D::width() const {
    return _width;
}

unsigned int ScalField3D::height() const {
    return _height;
}

unsigned int ScalField3D::depth() const {
    return _depth;
}


