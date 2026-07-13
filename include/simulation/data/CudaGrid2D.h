#ifndef EULERIANFLUIDSIMULATION_CUDAGRID2D_H
#define EULERIANFLUIDSIMULATION_CUDAGRID2D_H
#ifdef HAS_CUDA

#include "glm/vec2.hpp"
#include "simulation/data/IGrid.h"

class VecField2D;
class ScalField2D;

class CudaGrid2D final : public IGrid2D {
public:
    CudaGrid2D(unsigned width, unsigned height);
    ~CudaGrid2D() override;

    void setDirty();

    void upload() const;
    void download();

    VecField2D& vel() override;
    ScalField2D& rho() override;
    ScalField2D& gamma() override;
    ScalField2D& beta() override;
    ScalField2D& divergence() override;
    ScalField2D& pressure() override;
    VecField2D& forces() override;
    ScalField2D& boundary() override;

    void* gpuVel() const;
    void* gpuTmpVel() const;
    void* gpuRho() const;
    void* gpuTmpRho() const;
    void* gpuGamma() const;
    void* gpuNewGamma() const;
    void* gpuBeta() const;
    void* gpuA() const;
    void* gpuCounts() const;
    void* gpuDivergence() const;
    void* gpuPressure() const;
    void* gpuForces() const;
    void* gpuBoundary() const;

    struct AdvCell { unsigned int j; float val; };
    struct CudaData{
        glm::vec2* vel{};
        glm::vec2* tmp_vel{};
        float* rho{};
        float* tmp_rho{};
        float* gamma{};
        float* new_gamma{};
        float* beta{};
        AdvCell* A{};
        unsigned int* counts{};
        float* divergence{};
        float* pressure{};
        glm::vec2* forces{};
        float* boundary{};

        unsigned int width = 0;
        unsigned int height = 0;
    };
    struct CudaData* cudaData();

private:

    struct CudaData _cuda_data;

    VecField2D* _vel{};
    ScalField2D* _rho{};
    ScalField2D* _gamma{};
    ScalField2D* _beta{};
    ScalField2D* _divergence{};
    ScalField2D* _pressure{};
    VecField2D* _forces{};
    ScalField2D* _boundary{};

    bool _dirty = false;

    static void gpu_alloc(void** ptr, size_t size);
    static void gpu_alloc(float** ptr, size_t size);
    static void gpu_alloc(unsigned int** ptr, size_t size);
    static void gpu_alloc(glm::vec2** ptr, size_t size);
    static void gpu_alloc(AdvCell** ptr, size_t size);
    static void gpu_copy_H2D(void* dest, const void *src, size_t count);
    static void gpu_copy_D2H(void* dest, const void *src, size_t count);
};

#endif
#endif //EULERIANFLUIDSIMULATION_CUDAGRID2D_H