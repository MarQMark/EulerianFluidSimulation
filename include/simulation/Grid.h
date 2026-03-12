#ifndef GRID_H
#define GRID_H

#include <vector>
#include "glm/glm.hpp"

class Grid {
public:
    explicit Grid(unsigned int nWidth, unsigned int nHeight);
    ~Grid() = default;

private:
    unsigned int _n_width;
    unsigned int _n_height;

    std::vector<glm::vec2> _v0;
    std::vector<glm::vec2> _v1;
    std::vector<float> _s0;
    std::vector<float> _s1;
};



#endif //GRID_H
