#ifndef EULERIANFLUIDSIMULATION_CUDAGRID3D_H
#define EULERIANFLUIDSIMULATION_CUDAGRID3D_H
#ifdef HAS_CUDA

#include "glm/vec3.hpp"
#include "simulation/data/IGrid.h"

class VecField3D;
class ScalField3D;

class CudaGrid3D final : public IGrid3D {
public:
    CudaGrid3D(unsigned int width, unsigned int height, unsigned int depth);
    ~CudaGrid3D() override;

    void setDirty();

    void upload() const;
    void download();

    void clearA() const;
    void clearProj() const;

    VecField3D& vel() override;
    ScalField3D& rho() override;
    ScalField3D& gamma() override;
    ScalField3D& beta() override;
    ScalField3D& divergence() override;
    ScalField3D& pressure() override;
    VecField3D& forces() override;
    ScalField3D& boundary() override;

    struct AdvCell {
        union {
            unsigned int j;
            unsigned int i;
        };
        float val;
    };
    struct CudaData{
        glm::vec3* vel{};
        glm::vec3* tmp_vel{};
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
        glm::vec3* forces{};
        float* boundary{};

        unsigned int width = 0;
        unsigned int height = 0;
        unsigned int depth = 0;
    };
    struct CudaData* cudaData() const;
    struct CudaData* cudaDataCPU();

    glm::vec3* validA{};
    glm::vec3* validB{};

private:

    struct CudaData _cuda_data;
    struct CudaData* _gpu_cuda_data{};

    VecField3D* _vel{};
    ScalField3D* _rho{};
    ScalField3D* _gamma{};
    ScalField3D* _beta{};
    ScalField3D* _divergence{};
    ScalField3D* _pressure{};
    VecField3D* _forces{};
    ScalField3D* _boundary{};

    bool _dirty = false;

    static void gpu_alloc(void** ptr, size_t size);
    static void gpu_alloc(float** ptr, size_t size);
    static void gpu_alloc(unsigned int** ptr, size_t size);
    static void gpu_alloc(glm::vec3** ptr, size_t size);
    static void gpu_alloc(AdvCell** ptr, size_t size);
    static void gpu_alloc(CudaData** ptr, size_t size);
    static void gpu_copy_H2D(void* dest, const void *src, size_t count);
    static void gpu_copy_D2H(void* dest, const void *src, size_t count);
};

#endif
#endif //EULERIANFLUIDSIMULATION_CUDAGRID3D_H