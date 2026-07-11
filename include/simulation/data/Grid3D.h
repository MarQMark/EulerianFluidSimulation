#ifndef GRID3D_H
#define GRID3D_H

#include "IGrid.h"
#include "ScalField3D.h"
#include "VecField3D.h"
#include "StorageBuffer.h"
#include "glm/glm.hpp"

class Grid3D final : public IGrid3D {
private:
    struct _DisAdvCell3D {
        unsigned int i = 0;
        unsigned int j = 0;
        float val = 0.f;

        _DisAdvCell3D& operator+=(const struct _DisAdvCell3D& cell) {
            val += cell.val;
            return *this;
        }
        _DisAdvCell3D& operator=(const struct _DisAdvCell3D& cell) {
            i = cell.i;
            j = cell.j;
            val = cell.val;
            return *this;
        }
    } typedef DisAdvCell;

public:
    explicit Grid3D(unsigned int nWidth, unsigned int nHeight, unsigned int nDepth);
    ~Grid3D() override;

    glm::vec3 traceParticle(glm::vec3 pos, float dt, int substeps) const;

    VecField3D& vel() override;
    ScalField3D& rho() override;
    ScalField3D& gamma() override;
    ScalField3D& beta() override;
    ScalField3D& divergence() override;
    ScalField3D& pressure() override;
    VecField3D& forces() override;
    ScalField3D& boundary() override;

    std::unordered_map<unsigned int, DisAdvCell>& A() const;
    void addA(unsigned int i, unsigned int j, float val) const;

    VecField3D& tmpVec() const;
    void swapVec();
    ScalField3D& tmpScal() const;
    void swapRho();

private:
    VecField3D* _vel{};
    VecField3D* _v_tmp{};

    ScalField3D* _rho{};
    ScalField3D* _s_tmp{};

    ScalField3D* _gamma{};
    ScalField3D* _beta{};

    std::unordered_map<unsigned int, DisAdvCell>* _A{};

    ScalField3D* _divergence{};
    ScalField3D* _pressure{};

    VecField3D* _forces{};
    ScalField3D* _boundary{};
};

#endif //GRID_H
