#ifndef EULERIANFLUIDSIMULATION_SOLVERSTEPSVIEW_H
#define EULERIANFLUIDSIMULATION_SOLVERSTEPSVIEW_H

#include "Viewable.h"

class SolverStepsView : public Viewable{
public:
    void render(void* renderData) override;

private:
    static constexpr unsigned int MAX_STEPS = 2000;
    float _steps[MAX_STEPS] = {};
    unsigned int _steps_offset = 0;
};


#endif //EULERIANFLUIDSIMULATION_SOLVERSTEPSVIEW_H