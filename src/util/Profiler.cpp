#include "util/Profiler.h"

#include <cstring>
#include <ranges>

Profiler* Profiler::s_instance = nullptr;

void Profiler::start(const std::string &name) {
    if (!_profiles.contains(name))
        _profiles[name] = new Profile;

    _profiles[name]->start();
    _profiling.push(name);
}

void Profiler::stop(const std::string &name) {
    Profile* profile{};
    if (name.empty()) {
        if (_profiling.empty())
            return;

        profile = _profiles[_profiling.top()];
        _profiling.pop();
    }
    else {
        if (!_profiles.contains(name))
            return;

        profile = _profiles[name];
    }

    profile->stop();
}

std::map<std::string, Profiler::Profile *>& Profiler::profiles() {
    return _profiles;
}

void Profiler::resetMax() {
    for (const auto &profile: _profiles | std::views::values) {
        profile->resetMax();
    }
}

Profiler::Profile::Profile() {
    _data = (float*)malloc(PROFILER_SAMPLES * sizeof(float));
    memset(_data, 0, PROFILER_SAMPLES * sizeof(float));
}

Profiler::Profile::~Profile() {
    free(_data);
}

void Profiler::Profile::start() {
    _start = std::chrono::high_resolution_clock::now();
}

void Profiler::Profile::stop() {
    const auto elapsed = std::chrono::high_resolution_clock::now() - _start;
    _idx = (_idx + 1) % PROFILER_SAMPLES;
    _data[_idx] = std::chrono::duration<float, std::milli>(elapsed).count();
    _max = std::max(_max, _data[_idx]);
}

float * Profiler::Profile::data() const {
    return _data;
}

float Profiler::Profile::max() const {
    return _max;
}

float Profiler::Profile::cur() const {
    return _data[_idx];
}

float Profiler::Profile::avg() const {
    constexpr int nSamples = PROFILER_SAMPLES / 20;
    float a = cur();
    unsigned int idx = _idx;
    for (int i = 0; i < nSamples - 1; ++i) {
        idx = idx == 0 ? (PROFILER_SAMPLES - 1) : idx - 1;
        a += _data[idx];
    }
    return a / nSamples;
}

unsigned int Profiler::Profile::idx() const {
    return _idx;
}

void Profiler::Profile::resetMax() {
    _max = _data[_idx];
}

Profiler::~Profiler() {
    for (const auto profile: _profiles | std::views::values)
        delete profile;
}
