#ifndef EULERIANFLUIDSIMULATION_VECFIELD2D_H
#define EULERIANFLUIDSIMULATION_VECFIELD2D_H
#include <vector>

#include "glm/vec2.hpp"

class VecField2D {
public:
    VecField2D(unsigned int width, unsigned int height);
    VecField2D(const VecField2D& field, bool copy = true);

    glm::vec2& operator[](const std::size_t index) {
        return _field[index];
    }

    glm::vec2& operator()(const std::size_t x, const std::size_t y) {
        return _field[y * _width + x];
    }

    VecField2D& operator=(const VecField2D& field2) {
        if (this == &field2)
            return *this;

        _field = field2._field;
        return *this;
    }

    VecField2D& operator*=(const float scalar) {
        for (auto& v : _field)
            v *= scalar;
        return *this;
    }
    VecField2D operator*(const float scalar) const {
        VecField2D result = *this;
        result *= scalar;
        return result;
    }

    VecField2D& operator+=(VecField2D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] += field2[i];
        return *this;
    }
    VecField2D operator+(VecField2D& field2) const {
        VecField2D result = *this;
        result += field2;
        return result;
    }

    VecField2D& operator-=(VecField2D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] -= field2[i];
        return *this;
    }
    VecField2D operator-(VecField2D& field2) const {
        VecField2D result = *this;
        result -= field2;
        return result;
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
    unsigned int size() const;
    unsigned int width() const;
    unsigned int height() const;


    const glm::vec2* data() const { return _field.data(); }
    glm::vec2* data()           { return _field.data(); }

private:
    unsigned int _width = 0;
    unsigned int _height = 0;

    std::vector<glm::vec2> _field;

    float bilinear_interpolation(const glm::vec2 &pos, int low_x, int low_y, int comp);
};


#endif //EULERIANFLUIDSIMULATION_VECFIELD_H