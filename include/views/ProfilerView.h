#ifndef EULERIANFLUIDSIMULATION_PROFILERVIEW_H
#define EULERIANFLUIDSIMULATION_PROFILERVIEW_H

#include <chrono>
#include <map>
#include <string>
#include <vector>

#include "imgui.h"
#include "Viewable.h"
#include "util/Profiler.h"

class ProfilerView : public Viewable{
public:
    void render(void* renderData) override;

private:
    std::map<std::string, bool> _disabled_profiles;
    int _avg_samples = 20;
    bool _maintain_max = false;

    void draw_plot(const std::vector<Profiler::Profile*>& profiles, float scaleMax, ImVec2 size);
    void draw_line(Profiler::Profile* profile, float scaleMax, ImVec2 p0, ImVec2 p1, ImVec2 size);

    void draw_table_entry(Profiler::Profile* profile, const float* avg, float total);

    void get_sub_avg(float* avg, const std::vector<Profiler::Profile*>& subProfiles);
    float get_max(Profiler::Profile* profile);
    void set_disabled(Profiler::Profile* profile, bool disabled, bool recursive = false);
};


#endif //EULERIANFLUIDSIMULATION_PROFILERVIEW_H