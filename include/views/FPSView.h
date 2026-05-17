#ifndef EULERIANFLUIDSIMULATION_FPSVIEW_H
#define EULERIANFLUIDSIMULATION_FPSVIEW_H

#include "Viewable.h"

class FPSView : public Viewable{
public:
    FPSView() = default;

    void render() override;

private:
    static constexpr unsigned int MAX_TIMES = 2000;
    float _times[MAX_TIMES];
    unsigned int _times_offset = 0;
};


#endif //EULERIANFLUIDSIMULATION_FPSVIEW_H