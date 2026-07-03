#ifndef GRID3D_H
#define GRID3D_H

#include <map>

#include "ScalField3D.h"
#include "VecField3D.h"
#include "compute/StorageBuffer.h"
#include "glm/glm.hpp"

class Grid3D {
private:
    struct _DisAdvCell3D {
        //_DisAdvCell(const unsigned int _i, const unsigned int _j, const float _val)
        //    : i(_i), j(_j), val(_val) {}
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
    explicit Grid3D(unsigned int nWidth, unsigned int nHeight, unsigned int nDepth);
    ~Grid3D();

    glm::vec3 traceParticle(glm::vec3 pos, float dt, int substeps) const;

    VecField3D& vel() const;
    VecField3D& tmpVec() const;
    void swapVec();

    ScalField3D& rho() const;
    ScalField3D& tmpScal() const;
    void swapRho();

    ScalField3D& gamma() const;
    ScalField3D& beta() const;
    std::map<unsigned int, DisAdvCell>& A() const;
    void addA(unsigned int i, unsigned int j, float val) const;

    ScalField3D& divergence() const;
    ScalField3D& pressure() const;

    VecField3D& forces() const;
    ScalField3D& boundary() const;

    unsigned int width() const;
    unsigned int height() const;
    unsigned int depth() const;
    unsigned int size() const;

private:
    const unsigned int _n_width;
    const unsigned int _n_height;
    const unsigned int _n_depth;
    const unsigned int _size;

    VecField3D* _vel{};
    VecField3D* _v_tmp{};

    ScalField3D* _rho{};
    ScalField3D* _s_tmp{};

    ScalField3D* _gamma{};
    ScalField3D* _beta{};


    std::map<unsigned int, DisAdvCell>* _A{};

    ScalField3D* _divergence{};
    ScalField3D* _pressure{};

    VecField3D* _forces{};
    ScalField3D* _boundary{};
};

#endif //GRID_H
