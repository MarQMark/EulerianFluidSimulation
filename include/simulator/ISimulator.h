#ifndef EULERIANFLUIDSIMULATION_ISIMULATOR_H
#define EULERIANFLUIDSIMULATION_ISIMULATOR_H

#include <vector>
#include "simulation/Properties.h"

class ISimulator {
public:
    virtual ~ISimulator() = default;

    virtual void update() {
        vel_extrapolate();
        rho_advect();
        rho_sharpen();
        add_force();
        vel_advect();
        diffuse();
        project();
    }

    virtual void* getRenderData() = 0;

protected:
    virtual void vel_extrapolate() = 0;
    virtual void rho_advect() = 0;
    virtual void rho_sharpen() = 0;
    virtual void add_force() = 0;
    virtual void vel_advect() = 0;
    virtual void diffuse() = 0;
    virtual void project() = 0;

    static bool availableSolvers(const std::vector<Properties::SolverType>& solvers);
};

#endif //EULERIANFLUIDSIMULATION_ISIMULATOR_H