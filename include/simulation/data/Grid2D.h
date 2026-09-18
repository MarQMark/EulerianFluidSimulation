#ifndef GRID2D_H
#define GRID2D_H

#include <map>

#include "IGrid.h"
#include "ScalField2D.h"
#include "VecField2D.h"
#include "StorageBuffer.h"
#include "glm/glm.hpp"

class Grid2D final : public IGrid2D{
private:
    struct _DisAdvCell {
        unsigned int i = 0;
        unsigned int j = 0;
        float val = 0.f;

        _DisAdvCell& operator+=(const struct _DisAdvCell& cell) {
            val += cell.val;
            return *this;
        }
        _DisAdvCell& operator=(const struct _DisAdvCell& cell) {
            i = cell.i;
            j = cell.j;
            val = cell.val;
            return *this;
        }
    } typedef DisAdvCell;

public:
    explicit Grid2D(unsigned int nWidth, unsigned int nHeight);
    ~Grid2D() override;

    glm::vec2 traceParticle(glm::vec2 pos, float dt, int substeps) const;

    VecField2D& vel() override;
    VecField2D& tmpVec() const;
    void swapVec();

    ScalField2D& rho() override;
    ScalField2D& tmpScal() const;
    void swapRho();

    ScalField2D& gamma() override;
    ScalField2D& beta() override;
    std::map<unsigned int, DisAdvCell>& A() const;
    void addA(unsigned int i, unsigned int j, float val) const;

    ScalField2D& divergence() override;
    ScalField2D& pressure() override;

    VecField2D& forces() override;
    ScalField2D& boundary() override;

private:

    VecField2D* _vel{};
    VecField2D* _v_tmp{};

    ScalField2D* _rho{};
    ScalField2D* _s_tmp{};

    ScalField2D* _gamma{};
    ScalField2D* _beta{};

    std::map<unsigned int, DisAdvCell>* _A{};

    ScalField2D* _divergence{};
    ScalField2D* _pressure{};

    VecField2D* _forces{};
    ScalField2D* _boundary{};
};



#endif //GRID_H
