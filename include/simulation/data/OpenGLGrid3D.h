#ifndef EULERIANFLUIDSIMULATION_OPENGLGRID3D_H
#define EULERIANFLUIDSIMULATION_OPENGLGRID3D_H

#include "IGrid.h"
#include "StorageBuffer.h"
#include "glm/vec3.hpp"

class OpenGLGrid3D final : public IGrid3D {
public:
    OpenGLGrid3D(unsigned int width, unsigned int height, unsigned int depth);
    ~OpenGLGrid3D() override;

    void setDirty();

    void upload() const;
    void download();

    VecField3D& vel() override;
    ScalField3D& rho() override;
    ScalField3D& gamma() override;
    ScalField3D& beta() override;
    ScalField3D& divergence() override;
    ScalField3D& pressure() override;
    VecField3D& forces() override;
    ScalField3D& boundary() override;

    StorageBuffer& gpuVel() const;
    StorageBuffer& gpuTmpVel() const;
    StorageBuffer& gpuRho() const;
    StorageBuffer& gpuTmpRho() const;
    StorageBuffer& gpuGamma() const;
    StorageBuffer& gpuNewGamma() const;
    StorageBuffer& gpuBeta() const;
    StorageBuffer& gpuDivergence() const;
    StorageBuffer& gpuPressure() const;
    StorageBuffer& gpuForces() const;
    StorageBuffer& gpuBoundary() const;

    StorageBuffer& gpuValidA() const;
    StorageBuffer& gpuValidB() const;

    void bindAll() const;
    void clearA() const;

private:
    StorageBuffer* _gpu_vel{};
    StorageBuffer* _gpu_tmp_vel{};
    StorageBuffer* _gpu_rho{};
    StorageBuffer* _gpu_tmp_rho{};
    StorageBuffer* _gpu_gamma{};
    StorageBuffer* _gpu_new_gamma{};
    StorageBuffer* _gpu_beta{};
    StorageBuffer* _gpu_A_bwd{};
    StorageBuffer* _gpu_A_fwd{};
    StorageBuffer* _gpu_counts_bwd{};
    StorageBuffer* _gpu_counts_fwd{};
    StorageBuffer* _gpu_divergence{};
    StorageBuffer* _gpu_pressure{};
    StorageBuffer* _gpu_forces{};
    StorageBuffer* _gpu_boundary{};

    StorageBuffer* _gpu_valid_a{};
    StorageBuffer* _gpu_valid_b{};

    VecField3D* _vel{};
    ScalField3D* _rho{};
    ScalField3D* _gamma{};
    ScalField3D* _beta{};
    ScalField3D* _divergence{};
    ScalField3D* _pressure{};
    VecField3D* _forces{};
    ScalField3D* _boundary{};

    struct AdvCell { unsigned int j; float val; };

    bool _dirty = false;
};

#endif //EULERIANFLUIDSIMULATION_OPENGLGRID3D_H