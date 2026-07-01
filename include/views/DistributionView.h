#ifndef EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H
#define EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H

#include "Viewable.h"
#include "simulation/Grid2D.h"

class DistributionView : public Viewable {
public:
    void render(void* renderData) override;

private:
    int _y_level = -1;
};


#endif //EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H