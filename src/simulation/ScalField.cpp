#include "simulation/ScalField.h"

#include <algorithm>
#include <cmath>
#include <cstring>

ScalField::ScalField(const unsigned int width, const unsigned int height) : _width(width), _height(height){
    _field.resize(_width * _height);
    clear();
}

ScalField::ScalField(const ScalField &field, const bool copy ) : _width(field.width()), _height(field.height()){
    _field.resize(_width * _height);
    if (copy)
        memcpy(_field.data(), field._field.data(), field.size() * sizeof(float));
}

#if 1
float ScalField::interpolate(glm::vec2 pos) const {
    pos += glm::vec2(1.f);
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

    float v[4] = {
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
    float fxy1 = ((x2 - pos.x)/dx2x1) * v[0] + ((pos.x - x1)/dx2x1) * v[2];
    float fxy2 = ((x2 - pos.x)/dx2x1) * v[1] + ((pos.x - x1)/dx2x1) * v[3];
    if(dx2x1 < 1e-9) {
        fxy1 = v[1];
        fxy2 = v[3];
    }

    const float dy2y1 = y2 - y1;
    float fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
    if(dy2y1 < 1e-9)
        fxy = fxy1;

    return fxy;
}
#else
float ScalField::BilinearInterpolation(const glm::vec2 &pos,
                                          int low_x,
                                          int low_y,
                                          int comp) const
{
    float x_low_inter, x_high_inter;

    // Interpolate between y values at the lower x positions
    x_low_inter = (((low_y + 0.5 * (1 - comp)) - pos[1])
                    * (*this)(low_x, low_y))
                + ((pos[1] - (low_y - 0.5 - 0.5 * comp))
                    * (*this)(low_x, low_y + 1));
    // Interpolate between y values at the higher x positions
    x_high_inter = (((low_y + 0.5 * (1 - comp)) - pos[1])
                    * (*this)(low_x + 1, low_y))
                 + ((pos[1] - (low_y - 0.5 - 0.5 * comp))
                    * (*this)(low_x + 1, low_y + 1));

    // Interpolate between the averaged values at the low and high x positions
    return ((low_x + 0.5 * comp - pos[0]) * x_low_inter)
        + ((pos[0] - (low_x - 0.5 - 0.5 * (1 - comp))) * x_high_inter);
}

#define EPSILON 1.0e-10
float ScalField::interpolate(glm::vec2 pos) const {
    float Vx, Vy;

    // Interpolate the x component
    int low_x = (int)(std::floor(pos[0] + 1) + EPSILON);
    int low_y = (int)(pos[1] + 0.5 + EPSILON);
    Vx = BilinearInterpolation(pos, low_x, low_y, 0);

    // Interpolate the y component
    //low_x = (int)(pos[0] + 0.5);
    //low_y = (int)(floor(pos[1] + 1) + EPSILON);
    //Vy = BilinearInterpolation(pos, low_x, low_y, 1);

    return Vx;//glm::vec2(Vx, Vy);
}
#endif

void ScalField::clear() {
    memset(_field.data(), 0, size() * sizeof(float));
}

unsigned int ScalField::size() const {
    return _width * _height;
}

unsigned int ScalField::width() const {
    return _width;
}

unsigned int ScalField::height() const {
    return _height;
}


