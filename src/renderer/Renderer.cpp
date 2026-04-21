#include "renderer/Renderer.h"

#include <iostream>

Renderer::Renderer(const int width, const int height) : _width(width), _height(height){
    _txt = new Texture2D(_width, _height, nullptr);
}

Renderer::~Renderer() {
    delete _txt;
}

void Renderer::render( Grid *grid) {
    std::vector<float> data(_width * _height * 4);
    auto vel = grid->getVel();
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
    for(auto v : vel) {
        max = std::max(max, glm::length(v));
    }
    for (int i = 0; i < _width * _height; i++) {
        float len = glm::length(vel[i]);
        if(glm::length(vel[i]) > .001f){
            float angle = atan2(vel[i].y, vel[i].x);
            float value = 0.5f * (1.0f - cos(angle));

            data[i * 4 + 0] = len / max;
            //data[i * 4 + 1] = value;//std::atan2(vel[i].y, vel[i].x) / 6.28f + 0.5f;
            data[i * 4 + 2] = 1;
            data[i * 4 + 3] = 1.f;
        }
        else {
            data[i * 4 + 0] = 0;
            data[i * 4 + 1] = 0;
            data[i * 4 + 2] = 0;
            data[i * 4 + 3] = 1.f;
        }
    }

    _txt->update(reinterpret_cast<uint8_t *>(data.data()));
}

Texture2D* Renderer::getTexture2D() const {
    return _txt;
}
