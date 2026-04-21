#ifndef EULERIANFLUIDSIMULATION_SCALFIELD_H
#define EULERIANFLUIDSIMULATION_SCALFIELD_H
#include <cstddef>
#include <vector>

#include "glm/vec2.hpp"


class ScalField {
public:
    ScalField(unsigned int width, unsigned int height);
    ~ScalField() = default;

    float& operator[](const std::size_t index) {
        return _field[index];
    }

    float& operator()(const std::size_t x, const std::size_t y) {
        return _field[y * _width + x];
    }

    auto begin() {
        return _field.begin();
    }
    auto end() {
        return _field.end();
    }
    auto begin() const {
        return _field.begin();
    }
    auto end() const {
        return _field.end();
    }

    float interpolate(glm::vec2 pos);

    void clear();
    unsigned int size() const;
    unsigned int width() const;
    unsigned int height() const;

    float *data();

private:
    unsigned int _width = 0;
    unsigned int _height = 0;

    std::vector<float> _field;
};


#endif //EULERIANFLUIDSIMULATION_SCALFIELD_H