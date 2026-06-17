#ifndef GRID_H
#define GRID_H

#include <map>
#include <vector>

#include "ScalField.h"
#include "VecField.h"
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

    VecField& vel() const;
    VecField& tmpVec() const;
    void swapVec();

    ScalField& rho() const;
    ScalField& tmpScal() const;
    void swapRho();

    ScalField& gamma() const;
    ScalField& beta() const;
    std::map<unsigned int, DisAdvCell>& A() const;
    void addA(unsigned int i, unsigned int j, float val) const;

    ScalField& divergence() const;
    ScalField& pressure() const;

    VecField& forces() const;
    ScalField& boundary() const;

    unsigned int width() const;
    unsigned int height() const;
    unsigned int size() const;

private:
    const unsigned int _n_width;
    const unsigned int _n_height;
    const unsigned int _size;

    VecField* _vel{};
    VecField* _v_tmp{};

    ScalField* _rho{};
    ScalField* _s_tmp{};

    ScalField* _gamma{};
    ScalField* _beta{};


    std::map<unsigned int, DisAdvCell>* _A{};

    ScalField* _divergence{};
    ScalField* _pressure{};

    VecField* _forces{};
    ScalField* _boundary{};
};



#endif //GRID_H
