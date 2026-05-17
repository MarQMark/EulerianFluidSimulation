#include "renderer/Renderer.h"

#include <iostream>

#include "simulation/Properties.h"
#include "simulation/ScalField.h"

Renderer::Renderer(const int width, const int height) : _width(width), _height(height){
    _txt = new Texture2D(_width, _height, nullptr);
}

Renderer::~Renderer() {
    delete _txt;
}

void Renderer::render(const Grid *grid) {
    std::vector<float> data(_width * _height * 4);
    /*glm::vec2 max(0);
    for(auto v : vel) {
        max.x = std::max(max.x, std::abs(v.x));
        max.y = std::max(max.y, std::abs(v.y));
    }

    for (int i = 0; i < _width * _height; i++) {
        data[i * 4 + 0] = vel[i].x / max.x;
        data[i * 4 + 1] = vel[i].y / max.y;
        data[i * 4 + 2] = 1.f;
        data[i * 4 + 3] = 1.f;
    }*/

    float max = 0;
    switch (Properties::Get()->DRAW_MODE) {
        case Properties::VELOCITY:
            for(auto v : grid->vel())
                max = std::max(max, glm::length(v));
            for (int i = 0; i < _width * _height; i++) {
                data[i * 4 + 0] = 0;
                data[i * 4 + 1] = 0;
                data[i * 4 + 2] = glm::length(grid->vel()[i]) / max;
                data[i * 4 + 3] = 1.f;
            }
            break;
        case Properties::DENSITY:
            for(auto r : grid->rho())
                max = std::max(max, r);
            for (int i = 0; i < _width * _height; i++) {
                data[i * 4 + 0] = 0;
                data[i * 4 + 1] = 0;
                data[i * 4 + 2] = glm::length(grid->rho()[i]) / max;
                data[i * 4 + 3] = 1.f;
            }
            break;
        case Properties::DIVERGENCE:
            for(auto v : grid->divergence())
                max = std::max(max, v);
            for (int i = 0; i < _width * _height; i++) {
                data[i * 4 + 0] = glm::length(grid->divergence()[i]) / max;
                data[i * 4 + 1] = 0;
                data[i * 4 + 2] = 0;
                data[i * 4 + 3] = 1.f;
            }
            break;
        case Properties::PRESSURE:
            for(auto v : grid->pressure())
                max = std::max(max, v);
            for (int i = 0; i < _width * _height; i++) {
                data[i * 4 + 0] = 0;
                data[i * 4 + 1] = glm::length(grid->pressure()[i]) / max;
                data[i * 4 + 2] = 0;
                data[i * 4 + 3] = 1.f;
            }
            break;
        case Properties::FLUID:
            for (int i = 0; i < _width * _height; i++) {
                data[i * 4 + 0] = 0;
                data[i * 4 + 1] = 0;
                data[i * 4 + 2] = grid->rho()[i] > Properties::Get()->LIQUID_THRESHOLD ? .8f : 0.f;
                data[i * 4 + 3] = 1.f;
            }
            break;
    }
    Properties::Get()->MAX_VALUE = max;

    for (int i = 0; i < _width * _height; i++) {
        if (grid->boundary()[i] == 1) {
            data[i * 4 + 0] = .3f;
            data[i * 4 + 1] = .3f;
            data[i * 4 + 2] = .3f;
            data[i * 4 + 3] = .3f;
        }
    }

    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

Texture2D* Renderer::getTexture2D() const {
    return _txt;
}
