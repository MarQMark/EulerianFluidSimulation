//
// Created by ruut on 7/1/26.
//

#ifndef EULERIANFLUIDSIMULATION_ISIMULATIONDISPLAY_H
#define EULERIANFLUIDSIMULATION_ISIMULATIONDISPLAY_H

#include <map>
#include <string>
#include "views/Viewable.h"

class ISimulationDisplay {
protected:
    ISimulationDisplay() = default;
    std::map<std::string, Viewable*> _viewables;

public:
    virtual ~ISimulationDisplay() = default;

    virtual void update(void* renderData) = 0;
    virtual void reset(void* simulationData) {};

    virtual const std::map<std::string, Viewable*>& viewables() {
        return _viewables;
    };
};

#endif //EULERIANFLUIDSIMULATION_ISIMULATIONDISPLAY_H
