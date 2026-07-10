#ifndef EULERIANFLUIDSIMULATION_OPENGLGRID2D_H
#define EULERIANFLUIDSIMULATION_OPENGLGRID2D_H

#include "IGrid.h"
#include "StorageBuffer.h"

class OpenGLGrid2D final : public IGrid2D {
public:
    OpenGLGrid2D(unsigned width, unsigned height);
    ~OpenGLGrid2D() override;

    void upload() const;
    void download() const;

    VecField2D& vel(bool download = false) const;
    ScalField2D& rho(bool download = false) const;
    ScalField2D& gamma(bool download = false) const;
    ScalField2D& beta(bool download = false) const;
    ScalField2D& divergence(bool download = false) const;
    ScalField2D& pressure(bool download = false) const;
    VecField2D& forces(bool download = false) const;
    ScalField2D& boundary(bool download = false) const;

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
};

#endif //EULERIANFLUIDSIMULATION_OPENGLGRID2D_H