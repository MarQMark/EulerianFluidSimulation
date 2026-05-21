#ifndef EULERIANFLUIDSIMULATION_SCALFIELD_H
#define EULERIANFLUIDSIMULATION_SCALFIELD_H
#include <cstddef>
#include <cstring>
#include <vector>

#include "glm/vec2.hpp"


class ScalField {
public:
    ScalField(unsigned int width, unsigned int height);
    ScalField(const ScalField& field, bool copy = true);
    ~ScalField() = default;

    float& operator[](const std::size_t index) {
        return _field[index];
    }

    float& operator()(const std::size_t x, const std::size_t y) {
        return _field[y * _width + x];
    }
    float operator()(const std::size_t x, const std::size_t y) const {
        return _field[y * _width + x];
    }

    ScalField& operator=(const ScalField& field2) {
        if (this == &field2)
            return *this;

        //_field = field2._field;
        memcpy(_field.data(), field2._field.data(), field2.size() * sizeof(float));
        _width = field2._width;
        _height = field2._height;
        return *this;
    }
    ScalField& operator=(const float val) {
        //_field = field2._field;
        for(auto& v : _field)
            v = val;
        return *this;
    }
    ScalField& operator*=(const float scalar) {
        for (auto& v : _field)
            v *= scalar;
        return *this;
    }
    ScalField operator*(const float scalar) const {
        ScalField result = *this;
        result *= scalar;
        return result;
    }

    ScalField& operator/=(const float scalar) {
        for (auto& v : _field)
            v /= scalar;
        return *this;
    }
    ScalField operator/(const float scalar) const {
        ScalField result = *this;
        result /= scalar;
        return result;
    }

    ScalField& operator+=(const ScalField& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] += field2._field[i];
        return *this;
    }
    ScalField operator+(const ScalField& field2) const {
        ScalField result = *this;
        result += field2;
        return result;
    }

    ScalField& operator-=(const ScalField& field2) {
        for (int i = 0; i < _field.size(); i++)
            _field[i] -= field2._field[i];
        return *this;
    }
    ScalField operator-(const ScalField& field2) const {
        ScalField result = *this;
        result -= field2;
        return result;
    }

    float dot(const ScalField& field2) {
        //assert(_field.size() == field2.size());

        float dot = 0;
        for (int i = 0; i < field2.size(); i++)
            dot += _field[i] * field2._field[i];

        return dot;
    }
    float sqLen() {
        return dot(*this);
    }
    void addScaled(const ScalField& other, float alpha) {
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

    float interpolate(glm::vec2 pos) const;
    float BilinearInterpolation(const glm::vec2 &pos,
                                          int low_x,
                                          int low_y,
                                          int comp) const;

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