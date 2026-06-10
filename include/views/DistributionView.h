#ifndef EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H
#define EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H

#include "Viewable.h"
#include "simulation/Grid.h"

class DistributionView : public Viewable {
public:
    void render() override;

    void recreate(Grid* grid);

private:
    Grid* _grid{};
    int _y_level = 0;
};


#endif //EULERIANFLUIDSIMULATION_DISTRIBUTIONVIEW_H