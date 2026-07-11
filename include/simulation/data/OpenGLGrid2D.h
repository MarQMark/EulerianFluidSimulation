#ifndef EULERIANFLUIDSIMULATION_OPENGLGRID2D_H
#define EULERIANFLUIDSIMULATION_OPENGLGRID2D_H

#include "IGrid.h"
#include "StorageBuffer.h"

class OpenGLGrid2D final : public IGrid2D {
public:
    OpenGLGrid2D(unsigned width, unsigned height);
    ~OpenGLGrid2D() override;

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

    StorageBuffer& gpuVel() const;
    StorageBuffer& gpuTmpVel() const;
    StorageBuffer& gpuRho() const;
    StorageBuffer& gpuTmpRho() const;
    StorageBuffer& gpuGamma() const;
    StorageBuffer& gpuNewGamma() const;
    StorageBuffer& gpuBeta() const;
    StorageBuffer& gpuA() const;
    StorageBuffer& gpuCounts() const;
    StorageBuffer& gpuDivergence() const;
    StorageBuffer& gpuPressure() const;
    StorageBuffer& gpuForces() const;
    StorageBuffer& gpuBoundary() const;

private:
    StorageBuffer* _gpu_vel{};
    StorageBuffer* _gpu_tmp_vel{};
    StorageBuffer* _gpu_rho{};
    StorageBuffer* _gpu_tmp_rho{};
    StorageBuffer* _gpu_gamma{};
    StorageBuffer* _gpu_new_gamma{};
    StorageBuffer* _gpu_beta{};
    StorageBuffer* _gpu_A{};
    StorageBuffer* _gpu_counts{};
    StorageBuffer* _gpu_divergence{};
    StorageBuffer* _gpu_pressure{};
    StorageBuffer* _gpu_forces{};
    StorageBuffer* _gpu_boundary{};

    VecField2D* _vel{};
    ScalField2D* _rho{};
    ScalField2D* _gamma{};
    ScalField2D* _beta{};
    ScalField2D* _divergence{};
    ScalField2D* _pressure{};
    VecField2D* _forces{};
    ScalField2D* _boundary{};

    struct AdvCell { unsigned int j; float val; };

    bool _dirty = false;
};

#endif //EULERIANFLUIDSIMULATION_OPENGLGRID2D_H