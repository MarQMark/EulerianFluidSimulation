#ifndef EULERIANFLUIDSIMULATION_ISIMULATOR_H
#define EULERIANFLUIDSIMULATION_ISIMULATOR_H

#include <vector>
#include "simulation/Properties.h"
#include "util/Profiler.h"

class ISimulator {
public:
    virtual ~ISimulator() = default;

    virtual void update() {
        Profiler* profiler = Profiler::Get();

        PROFILE("0 Vel Decay",
            if (!PROPERTY_B(DISABLE_VEL_DECAY)) vel_decay();
        )
        PROFILE("1 Vel Extrapolate",
            if (!PROPERTY_B(DISABLE_VEL_EXTRAPOLATE)) vel_extrapolate();
        )
        PROFILE("2 Rho Sharpen",
            if (!PROPERTY_B(DISABLE_RHO_SHARPEN)) rho_sharpen();
        )
        PROFILE("3 Rho Advect",
            if (!PROPERTY_B(DISABLE_RHO_ADVECT)) rho_advect();
        )
        PROFILE("4 Vel Advect",
            if (!PROPERTY_B(DISABLE_VEL_ADVECT)) vel_advect();
        )
        PROFILE("5 Add Force",
            if (!PROPERTY_B(DISABLE_ADD_FORCE)) add_force();
        )
        PROFILE("6 Project",
            if (!PROPERTY_B(DISABLE_PROJECT)) project();
        )
    }

    virtual void* getRenderData() = 0;

protected:
    virtual void vel_decay() = 0;
    virtual void vel_extrapolate() = 0;
    virtual void rho_advect() = 0;
    virtual void rho_sharpen() = 0;
    virtual void add_force() = 0;
    virtual void vel_advect() = 0;
    //virtual void diffuse() = 0;
    virtual void project() = 0;

    static bool availableSolvers(const std::vector<Properties::SolverType>& solvers);
};

#endif //EULERIANFLUIDSIMULATION_ISIMULATOR_H