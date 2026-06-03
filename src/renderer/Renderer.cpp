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

void Renderer::drawField(std::vector<float>& data, VecField& field, int chanel, float& max) {
    for(auto v : field)
        max = std::max(max, glm::length(v));
    for (int i = 0; i < field.width() * field.height(); i++) {
        data[i * 4 + 0] = 0;
        data[i * 4 + 1] = 0;
        data[i * 4 + 2] = 0;
        data[i * 4 + 3] = 1.f;

        if (const float val = glm::length(field[i]) > 0)
            data[i * 4 + chanel] = val / max;
        else
            data[i * 4 + ((chanel + 1) % 3)] = val / max;
    }
}
void Renderer::drawField(std::vector<float>& data, ScalField& field, int chanel, float& max) {
    for(auto s : field)
        max = std::max(max, std::abs(s));
    for (int i = 0; i < field.width() * field.height(); i++) {
        data[i * 4 + 0] = 0;
        data[i * 4 + 1] = 0;
        data[i * 4 + 2] = 0;
        data[i * 4 + 3] = 1.f;

        //data[i * 4 + chanel] = std::abs(field[i]) / max;
        const float val = field[i];
        if (val > 0)
            data[i * 4 + chanel] = val / max;
        else
            data[i * 4 + ((chanel + 1) % 3)] = std::abs(val) / max;


        if (!std::isfinite(field[i]))
            data[i * 4 + ((chanel + 1) % 3)] = 1.f;
    }
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
            drawField(data, grid->vel(), 2, max);
            break;
        case Properties::DENSITY:
            drawField(data, grid->rho(), 2, max);
            break;
        case Properties::DIVERGENCE:
            drawField(data, grid->divergence(), 0, max);
            break;
        case Properties::PRESSURE:
            drawField(data, grid->pressure(), 1, max);
            break;
        case Properties::GAMMA:
            drawField(data, grid->gamma(), 1, max);
            break;
        case Properties::BETA:
            drawField(data, grid->beta(), 0, max);
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
