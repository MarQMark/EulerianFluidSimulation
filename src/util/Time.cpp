#include "util/Time.h"
#include <thread>

Time* Time::s_instance = nullptr;

void Time::update() {
    _dt = ((std::chrono::duration<double, std::milli>)(std::chrono::high_resolution_clock::now() - _last_time)).count();
    _last_time = std::chrono::high_resolution_clock::now();

    //if dt is higher than a day -> error -> set to 0
    if(_dt > 1e7)
        _dt = 1e-7;

    _frames_last_second++;

    // set window title
    if(_time_last_second >= 500.0)
    {
        fps = _frames_last_second * 2;
        _time_last_second = 0;
        _frames_last_second = 0;
    }

    _time_last_second += _dt;

    dt = _dt;
    tt += _dt;
}

void Time::limitFPS() {
    if(targetFps == 0)
        return;

    auto delay = (double)(1000./(double)targetFps - ((std::chrono::duration<double, std::milli>)(std::chrono::high_resolution_clock::now() - _last_time)).count());
    if(delay < 0)
        ;//printf("[WARNING] Overload could not keep target FPS\n");
    else
        std::this_thread::sleep_for(std::chrono::microseconds((uint32_t)(delay * 1000)));
}
