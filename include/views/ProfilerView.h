#ifndef EULERIANFLUIDSIMULATION_PROFILERVIEW_H
#define EULERIANFLUIDSIMULATION_PROFILERVIEW_H

#include <chrono>
#include <string>
#include <vector>

#include "imgui.h"
#include "Viewable.h"
#include "util/Profiler.h"

class ProfilerView : public Viewable{

public:
    void drawMultiLinePlot(const std::map<std::string, Profiler::Profile*>& profiles,
                                         float scaleMax, ImVec2 size);

    void render() override;

private:
};


#endif //EULERIANFLUIDSIMULATION_PROFILERVIEW_H