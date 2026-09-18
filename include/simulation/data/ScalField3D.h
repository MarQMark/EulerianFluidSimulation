#ifndef EULERIANFLUIDSIMULATION_SCALFIELD3D_H
#define EULERIANFLUIDSIMULATION_SCALFIELD3D_H
#include <cstddef>
#include <cstring>
#include <vector>

#include "glm/vec3.hpp"

class ScalField3D {
public:
    ScalField3D(unsigned int width, unsigned int height, unsigned int depth);
    ScalField3D(const ScalField3D& field, bool copy = true);
    ~ScalField3D() = default;

    float& operator[](const std::size_t index) {
        return _field[index];
    }

    float& operator()(const std::size_t x, const std::size_t y, const std::size_t z) {
        return _field[z * _width * _height + y * _width + x];
    }

    float operator()(const std::size_t x, const std::size_t y, const std::size_t z) const {
        return _field[z * _width * _height + y * _width + x];
    }

    ScalField3D& operator=(const ScalField3D& field2) {
        if (this == &field2)
            return *this;

        memcpy(_field.data(), field2._field.data(), field2.size() * sizeof(float));
        _width = field2._width;
        _height = field2._height;
        return *this;
    }

    ScalField3D& operator=(const float val) {
        for(auto& v : _field)
            v = val;
        return *this;
    }

    ScalField3D& operator*=(const float scalar) {
        for (auto& v : _field)
            v *= scalar;
        return *this;
    }

    ScalField3D operator*(const float scalar) const {
        ScalField3D result = *this;
        result *= scalar;
        return result;
    }

    ScalField3D& operator/=(const float scalar) {
        for (auto& v : _field)
            v /= scalar;
        return *this;
    }

    ScalField3D operator/(const float scalar) const {
        ScalField3D result = *this;
        result /= scalar;
        return result;
    }

    ScalField3D& operator+=(const ScalField3D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] += field2._field[i];
        return *this;
    }

    ScalField3D operator+(const ScalField3D& field2) const {
        ScalField3D result = *this;
        result += field2;
        return result;
    }

    ScalField3D& operator-=(const ScalField3D& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] -= field2._field[i];
        return *this;
    }

    ScalField3D operator-(const ScalField3D& field2) const {
        ScalField3D result = *this;
        result -= field2;
        return result;
    }

    float dot(const ScalField3D& field2) {
        float dot = 0;
        for (int i = 0; i < field2.size(); i++)
            dot += _field[i] * field2._field[i];

        return dot;
    }

    float sqLen() {
        return dot(*this);
    }

    void addScaled(const ScalField3D& other, float alpha) {
        for (size_t i = 0; i < _field.size(); i++)
            _field[i] += alpha * other._field[i];
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

    float interpolate(glm::vec3 pos) const;

    void clear();
    unsigned int size() const;
    unsigned int width() const;
    unsigned int height() const;
    unsigned int depth() const;

    const float* data() const { return _field.data(); }
    float* data(){ return _field.data(); }

private:
    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _depth = 0;

    std::vector<float> _field;
};


#endif //EULERIANFLUIDSIMULATION_SCALFIELD_H