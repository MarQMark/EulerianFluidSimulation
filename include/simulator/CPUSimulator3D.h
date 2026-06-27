#ifndef EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H
#define EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H

#include "ISimulator3D.h"

class CPUSimulator3D final : public ISimulator3D {
public:
    CPUSimulator3D(unsigned int width, unsigned int height, unsigned int depth);
    ~CPUSimulator3D() override;

    void update() override;

    void* getRenderData() override;

private:
    void vel_extrapolate() override;
    void rho_advect() override;
    void rho_sharpen() override;
    void add_force() override;
    void vel_advect() override;
    void diffuse() override;
    void project() override;

};


#endif //EULERIANFLUIDSIMULATION_CPUSIMULATOR3D_H