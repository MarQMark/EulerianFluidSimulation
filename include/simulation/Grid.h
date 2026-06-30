#ifndef GRID_H
#define GRID_H

#include <map>
#include <vector>

#include "ScalField2D.h"
#include "VecField2D.h"
#include "compute/StorageBuffer.h"
#include "glm/glm.hpp"

class Grid {
private:
    struct _DisAdvCell {
        //_DisAdvCell(const unsigned int _i, const unsigned int _j, const float _val)
        //    : i(_i), j(_j), val(_val) {}
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

#ifdef USE_GPU
public:
    StorageBuffer* gpuVel{};
    StorageBuffer* gpuTmpVel{};

    StorageBuffer* gpuRho{};
    StorageBuffer* gpuTmpRho{};
    StorageBuffer* gpuGamma{};
    StorageBuffer* gpuBeta{};

    StorageBuffer* gpuDivergence{};
    StorageBuffer* gpuPressure{};

    StorageBuffer* gpuForces{};
    StorageBuffer* gpuBoundary{};

    void gpuUpload() const;
    void gpuDownload() const;
#endif

public:
    explicit Grid(unsigned int nWidth, unsigned int nHeight);
    ~Grid();

    glm::vec2 traceParticle(glm::vec2 pos, float dt, int substeps) const;

    VecField2D& vel() const;
    VecField2D& tmpVec() const;
    void swapVec();

    ScalField2D& rho() const;
    ScalField2D& tmpScal() const;
    void swapRho();

    ScalField2D& gamma() const;
    ScalField2D& beta() const;
    std::map<unsigned int, DisAdvCell>& A() const;
    void addA(unsigned int i, unsigned int j, float val) const;

    ScalField2D& divergence() const;
    ScalField2D& pressure() const;

    VecField2D& forces() const;
    ScalField2D& boundary() const;

    unsigned int width() const;
    unsigned int height() const;
    unsigned int size() const;

private:
    const unsigned int _n_width;
    const unsigned int _n_height;
    const unsigned int _size;

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
