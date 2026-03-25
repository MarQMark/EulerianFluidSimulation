#ifndef GRID_H
#define GRID_H

#include <vector>
#include "glm/glm.hpp"

class Grid {
public:
    explicit Grid(unsigned int nWidth, unsigned int nHeight);
    ~Grid() = default;

    std::vector<glm::vec2>& getVel();
    std::vector<glm::vec2>& getNewVel();

    std::vector<float>& getPres();
    std::vector<float>& getNewPres();

    void swapVel();
    void swapPres();

private:
    unsigned int _n_width;
    unsigned int _n_height;

    uint8_t _vel = 0;
    uint8_t _pres = 0;

    std::vector<glm::vec2> _v0;
    std::vector<glm::vec2> _v1;
    std::vector<float> _s0;
    std::vector<float> _s1;

    void init();
};



#endif //GRID_H
