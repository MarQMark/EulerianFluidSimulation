#ifndef EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H
#define EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H
#include "simulation/ScalField.h"

float ConjugateGradient2(ScalField& pressure, const ScalField& divergence, const ScalField& rho, const ScalField& boundary, int steps);

#endif //EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H