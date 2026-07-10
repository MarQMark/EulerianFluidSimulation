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

#if 0
glm::vec2 VecField2D::interpolate(glm::vec2 pos) {
    pos += glm::vec2(.5);
    const int x = (int)pos.x;
    const int y = (int)pos.y;

    int ix1 = x;
    int ix2 = x + 1;
    if(pos.x - (float)x < 0.5f) {
        ix1 = x - 1;
        ix2 = x;
    }
    int iy1 = y;
    int iy2 = y + 1;
    if(pos.y - (float)y < 0.5f) {
        iy1 = y - 1;
        iy2 = y;
    }

    ix1 = std::clamp(ix1, 0, (int)_width - 1);
    ix2 = std::clamp(ix2, 0, (int)_width - 1);
    iy1 = std::clamp(iy1, 0, (int)_height - 1);
    iy2 = std::clamp(iy2, 0, (int)_height - 1);

    glm::vec2 v[4] = {
        (*this)(ix1, iy1),
        (*this)(ix1, iy2),
        (*this)(ix2, iy1),
        (*this)(ix2, iy2),
    };

    float x1 = (float)ix1 + 0.5f;
    float x2 = (float)ix2 + 0.5f;
    float y1 = (float)iy1 + 0.5f;
    float y2 = (float)iy2 + 0.5f;

    const float dx2x1 = x2 - x1;
    glm::vec2 fxy1 = ((x2 - pos.x)/dx2x1) * v.x + ((pos.x - x1)/dx2x1) * v[2];
    glm::vec2 fxy2 = ((x2 - pos.x)/dx2x1) * v.y + ((pos.x - x1)/dx2x1) * v[3];
    if(dx2x1 < 1e-9) {
        fxy1 = v.y;
        fxy2 = v[3];
    }

    const float dy2y1 = y2 - y1;
    glm::vec2 fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
    if(dy2y1 < 1e-9)
        fxy = fxy1;

    return fxy;
}
#elif 0
glm::vec2 VecField2D::interpolate(glm::vec2 pos) {
    // pos is in cell-center space: cell (i,j) has center at (i+0.5, j+0.5)
    // Find the left/bottom neighbor index
    float px = pos.x - 0.5f;
    float py = pos.y - 0.5f;

    int x0 = (int)std::floor(px);
    int y0 = (int)std::floor(py);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Fractional part [0,1) — no branching needed
    float tx = px - (float)x0;
    float ty = py - (float)y0;

    // Clamp indices to valid range
    x0 = std::clamp(x0, 0, (int)_width  - 1);
    x1 = std::clamp(x1, 0, (int)_width  - 1);
    y0 = std::clamp(y0, 0, (int)_height - 1);
    y1 = std::clamp(y1, 0, (int)_height - 1);

    // Bilinear interpolation
    glm::vec2 v00 = (*this)(x0, y0);
    glm::vec2 v10 = (*this)(x1, y0);
    glm::vec2 v01 = (*this)(x0, y1);
    glm::vec2 v11 = (*this)(x1, y1);

    glm::vec2 row0 = glm::mix(v00, v10, tx);
    glm::vec2 row1 = glm::mix(v01, v11, tx);
    return glm::mix(row0, row1, ty);
}
#elif 0
float VecField2D::BilinearInterpolation(const glm::vec2 &pos, const int low_x, const int low_y, const int comp) {
    // Interpolate between y values at the lower x positions
    const float lInter = (((low_y + 0.5 * (1 - comp)) - pos.y) * (*this)(low_x, low_y)[comp])
                       + ((pos.y - (low_y - 0.5 - 0.5 * comp)) * (*this)(low_x, low_y + 1)[comp]);
    // Interpolate between y values at the higher x positions
    const float hInter = (((low_y + 0.5 * (1 - comp)) - pos.y) * (*this)(low_x + 1, low_y)[comp])
                       + ((pos.y - (low_y - 0.5 - 0.5 * comp)) * (*this)(low_x + 1, low_y + 1)[comp]);

    // Interpolate between the averaged values at the low and high x positions
    return ((low_x + 0.5 * comp - pos.x) * lInter) + ((pos.x - (low_x - 0.5 - 0.5 * (1 - comp))) * hInter);
}

#define EPSILON 1.0e-10
glm::vec2 VecField2D::interpolate(const glm::vec2 pos) {
    // Interpolate the x component
    int low_x = (int)(std::floor(pos.x + 1) + EPSILON);
    int low_y = (int)(pos.y + 0.5 + EPSILON);
    const float vx = BilinearInterpolation(pos, low_x, low_y, 0);

    // Interpolate the y component
    low_x = (int)(pos.x + 0.5);
    low_y = (int)(floor(pos.y + 1) + EPSILON);
    const float vy = BilinearInterpolation(pos, low_x, low_y, 1);

    return glm::vec2(vx, vy);
}
#else
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
#endif

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


