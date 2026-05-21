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

    VecField& vel() const;
    VecField& tmpVec() const;
    void swapVec();

    ScalField& rho() const;
    ScalField& tmpScal() const;
    void swapRho();

    ScalField& gamma() const;
    ScalField& beta() const;

    ScalField& divergence() const;
    ScalField& pressure() const;

    VecField& forces() const;
    ScalField& boundary() const;

    unsigned int width() const;
    unsigned int height() const;
    unsigned int size() const;

private:
    unsigned int _n_width;
    unsigned int _n_height;

    VecField* _vel{};
    VecField* _v_tmp{};

    ScalField* _rho{};
    ScalField* _s_tmp{};

    ScalField* _gamma{};
    ScalField* _beta{};

    ScalField* _divergence{};
    ScalField* _pressure{};

    VecField* _forces{};
    ScalField* _boundary{};
};



#endif //GRID_H
