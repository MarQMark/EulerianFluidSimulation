#ifndef EULERIANFLUIDSIMULATION_STATSVIEW_H
#define EULERIANFLUIDSIMULATION_STATSVIEW_H

#include "Viewable.h"
#include "simulation/Grid.h"

class StatsView : public Viewable{
public:
    void render() override;

    void recreate(Grid* grid);

private:
    Grid* _grid{};

    static constexpr unsigned int MAX_TIMES = 2000;
    float _times[MAX_TIMES];
    unsigned int _times_offset = 0;
};


#endif //EULERIANFLUIDSIMULATION_STATSVIEW_H