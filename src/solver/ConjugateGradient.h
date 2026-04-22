#ifndef EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H
#define EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H
#include "simulation/ScalField.h"

float ConjugateGradient(ScalField& pressure, const ScalField& divergence, int steps);

#endif //EULERIANFLUIDSIMULATION_CONJUGATEGRADIENT_H