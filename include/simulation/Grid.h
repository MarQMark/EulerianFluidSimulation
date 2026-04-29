#ifndef GRID_H
#define GRID_H

#include <vector>

#include "ScalField.h"
#include "VecField.h"
#include "glm/glm.hpp"

class Grid {
public:
    explicit Grid(unsigned int nWidth, unsigned int nHeight);
    ~Grid();

    VecField& getVel() const;
    VecField& getNewVel() const;

    ScalField& getPres();
    ScalField& getNewPres();

    void swapVel();
    void swapPres();

private:
    unsigned int _n_width;
    unsigned int _n_height;

    uint8_t _vel = 0;
    uint8_t _pres = 0;

    VecField* _v0{};
    VecField* _v1{};
    ScalField* _s0;
    ScalField* _s1;

    void init();
};



#endif //GRID_H
