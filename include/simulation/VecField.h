#ifndef EULERIANFLUIDSIMULATION_VECFIELD_H
#define EULERIANFLUIDSIMULATION_VECFIELD_H
#include <vector>

#include "glm/vec2.hpp"

class VecField {
public:
    VecField(unsigned int width, unsigned int height);
    VecField(const VecField& field, bool copy = true);

    glm::vec2& operator[](const std::size_t index) {
        return _field[index];
    }

    glm::vec2& operator()(const std::size_t x, const std::size_t y) {
        return _field[y * _width + x];
    }

    VecField& operator=(const VecField& field2) {
        if (this == &field2)
            return *this;

        _field = field2._field;
        return *this;
    }

    VecField& operator*=(const float scalar) {
        for (auto& v : _field)
            v *= scalar;
        return *this;
    }
    VecField operator*(const float scalar) const {
        VecField result = *this;
        result *= scalar;
        return result;
    }

    VecField& operator+=(VecField& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] += field2[i];
        return *this;
    }
    VecField operator+(VecField& field2) const {
        VecField result = *this;
        result += field2;
        return result;
    }

    VecField& operator-=(VecField& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] -= field2[i];
        return *this;
    }
    VecField operator-(VecField& field2) const {
        VecField result = *this;
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

    float BilinearInterpolation(const glm::vec2 &pos,
                                          int low_x,
                                          int low_y,
                                          int comp) ;
private:
    unsigned int _width = 0;
    unsigned int _height = 0;

    std::vector<glm::vec2> _field;
};


#endif //EULERIANFLUIDSIMULATION_VECFIELD_H