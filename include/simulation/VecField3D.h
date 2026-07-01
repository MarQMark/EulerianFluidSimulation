#ifndef EULERIANFLUIDSIMULATION_VECFIELD3D_H
#define EULERIANFLUIDSIMULATION_VECFIELD3D_H

#include <vector>
#include "glm/vec3.hpp"

class VecField3D {
public:
    VecField3D(unsigned int width, unsigned int height, unsigned int depth);
    VecField3D(const VecField3D& field, bool copy = true);

    glm::vec3& operator[](const std::size_t index) {
        return _field[index];
    }

    glm::vec3& operator()(const std::size_t x, const std::size_t y, const std::size_t z) {
        return _field[z * _width * _height + y * _width + x];
    }

    VecField3D& operator=(const VecField3D& field2) {
        if (this == &field2)
            return *this;

        _field = field2._field;
        return *this;
    }

    VecField3D& operator*=(const float scalar) {
        for (auto& v : _field)
            v *= scalar;
        return *this;
    }
    VecField3D operator*(const float scalar) const {
        VecField3D result = *this;
        result *= scalar;
        return result;
    }

    VecField3D& operator+=(VecField3D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] += field2[i];
        return *this;
    }
    VecField3D operator+(VecField3D& field2) const {
        VecField3D result = *this;
        result += field2;
        return result;
    }

    VecField3D& operator-=(VecField3D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] -= field2[i];
        return *this;
    }
    VecField3D operator-(VecField3D& field2) const {
        VecField3D result = *this;
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

    glm::vec3 interpolate(glm::vec3 pos);

    void clear();
    unsigned int size() const;
    unsigned int width() const;
    unsigned int height() const;
    unsigned int depth() const;

    float BilinearInterpolation(const glm::vec3 &pos,
                                          int low_x,
                                          int low_y,
                                          int comp);

    const glm::vec3* data() const { return _field.data(); }
    glm::vec3* data()           { return _field.data(); }

private:
    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _depth = 0;

    std::vector<glm::vec3> _field;
};


#endif //EULERIANFLUIDSIMULATION_VECFIELD_H