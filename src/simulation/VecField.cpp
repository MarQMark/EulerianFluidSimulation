#include "simulation/VecField.h"

#include <algorithm>

VecField::VecField(const unsigned int width, const unsigned int height) : _width(width), _height(height){
    _field.resize(width * height);
    clear();
}

glm::vec2 VecField::interpolate(glm::vec2 pos) {
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
    glm::vec2 fxy1 = ((x2 - pos.x)/dx2x1) * v[0] + ((pos.x - x1)/dx2x1) * v[2];
    glm::vec2 fxy2 = ((x2 - pos.x)/dx2x1) * v[1] + ((pos.x - x1)/dx2x1) * v[3];
    if(dx2x1 < 1e-9) {
        fxy1 = v[1];
        fxy2 = v[3];
    }

    const float dy2y1 = y2 - y1;
    glm::vec2 fxy = ((y2 - pos.y)/dy2y1) * fxy1 + ((pos.y - y1)/dy2y1) * fxy2;
    if(dy2y1 < 1e-9)
        fxy = fxy1;

    return fxy;
}

void VecField::clear() {
    for (int i = 0; i < _field.size(); i++)
        _field[i] = glm::vec2(0);
}


