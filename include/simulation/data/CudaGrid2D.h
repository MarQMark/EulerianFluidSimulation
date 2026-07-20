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

    void clearA() const;
    void clearProj() const;

    VecField2D& vel() override;
    ScalField2D& rho() override;
    ScalField2D& gamma() override;
    ScalField2D& beta() override;
    ScalField2D& divergence() override;
    ScalField2D& pressure() override;
    VecField2D& forces() override;
    ScalField2D& boundary() override;

    struct AdvCell {
        union {
            unsigned int j;
            unsigned int i;
        };
        float val;
    };
    struct CudaData{
        glm::vec2* vel{};
        glm::vec2* tmp_vel{};
        float* rho{};
        float* tmp_rho{};
        float* gamma{};
        union { // same buffer can be used for:
            float* new_gamma{}; // rho_advect
            float* del_rho;   // rho_sharpen
        };
        float* beta{};
        AdvCell* A_bwd{};
        AdvCell* A_fwd{};
        unsigned int* counts_bwd{};
        unsigned int* counts_fwd{};
        float* divergence{};
        float* pressure{};
        glm::vec2* forces{};
        float* boundary{};

        unsigned int width = 0;
        unsigned int height = 0;
    };
    struct CudaData* cudaData() const;
    struct CudaData* cudaDataCPU();

private:

    struct CudaData _cuda_data;
    struct CudaData* _gpu_cuda_data{};

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
    static void gpu_alloc(CudaData** ptr, size_t size);
    static void gpu_copy_H2D(void* dest, const void *src, size_t count);
    static void gpu_copy_D2H(void* dest, const void *src, size_t count);
};

#endif
#endif //EULERIANFLUIDSIMULATION_CUDAGRID2D_H