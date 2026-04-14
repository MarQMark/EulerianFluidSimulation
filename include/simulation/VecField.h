#ifndef EULERIANFLUIDSIMULATION_VECFIELD_H
#define EULERIANFLUIDSIMULATION_VECFIELD_H
#include <vector>

#include "glm/vec2.hpp"

class VecField {
public:
    VecField(unsigned int width, unsigned int height);

    glm::vec2& operator[](const std::size_t index) {
        return _field[index];
    }

    glm::vec2& operator()(const std::size_t x, const std::size_t y) {
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

    glm::vec2 interpolate(glm::vec2 pos);

    void clear();

private:
    unsigned int _width = 0;
    unsigned int _height = 0;

    std::vector<glm::vec2> _field;
};


#endif //EULERIANFLUIDSIMULATION_VECFIELD_H