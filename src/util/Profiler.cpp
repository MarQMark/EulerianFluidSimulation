#include "util/Profiler.h"

#include <cstring>
#include <ranges>

Profiler* Profiler::s_instance = nullptr;

void Profiler::start(const std::string &name) {
    Profile* profile = nullptr;
    for (const auto p : _profiles) {
        if (name == p->name()) {
            profile = p;
            break;
        }
    }

    if (!profile) {
        profile = new Profile(name);
        _profiles.push_back(profile);
    }

    profile->start();
    _profiling.push(profile);
}

void Profiler::stop() {
    if (_profiling.empty())
        return;

    Profile* profile = _profiling.top();
    profile->stop();
    _profiling.pop();
}

std::vector<Profiler::Profile*>& Profiler::profiles() {
    return _profiles;
}

void Profiler::resetMax() const {
    for (const auto &profile: _profiles) {
        profile->resetMax();
    }
}

Profiler::Profile::Profile(const std::string& name) : _name(std::move(name)){
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

float Profiler::Profile::avg(const int count) const {
    const int nSamples = PROFILER_SAMPLES / count;
    float a = cur();
    unsigned int idx = _idx;
    for (int i = 0; i < nSamples - 1; ++i) {
        idx = idx == 0 ? (PROFILER_SAMPLES - 1) : idx - 1;
        a += _data[idx];
    }
    return a / nSamples;
}

std::string Profiler::Profile::name() {
    return _name;
}

unsigned int Profiler::Profile::idx() const {
    return _idx;
}

void Profiler::Profile::resetMax() {
    _max = _data[_idx];
}

Profiler::~Profiler() {
    for (const auto profile: _profiles)
        delete profile;
}
