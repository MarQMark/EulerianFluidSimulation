#ifndef EULERIANFLUIDSIMULATION_TIME_H
#define EULERIANFLUIDSIMULATION_TIME_H

#include <chrono>

class Time {
private:
    Time() = default;

    static Time* s_instance;

public:
    static Time* Get() {
        if(!s_instance)
            s_instance = new Time();

        return s_instance;
    }

    void update();
    void limitFPS();

    double dt = 0;
    double tt = 0;
    unsigned int fps = 0;

    unsigned int targetFps = 144;
private:
    std::chrono::high_resolution_clock::time_point _last_time;
    double _dt = 0;
    double _time_last_second = 0;
    unsigned int _frames_last_second = 0;
};


#endif //EULERIANFLUIDSIMULATION_TIME_H