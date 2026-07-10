#ifndef EULERIANFLUIDSIMULATION_STATSVIEW_H
#define EULERIANFLUIDSIMULATION_STATSVIEW_H

#include "Viewable.h"

class StatsView : public Viewable{
public:
    void render(void* renderData) override;

private:
    static constexpr unsigned int MAX_TIMES = 2000;
    float _times[MAX_TIMES];
    unsigned int _times_offset = 0;
};


#endif //EULERIANFLUIDSIMULATION_STATSVIEW_H