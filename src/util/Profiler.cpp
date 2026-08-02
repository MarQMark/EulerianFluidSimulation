#include "util/Profiler.h"

#include <cstring>
#include <iostream>
#include <ranges>

#include "simulation/Properties.h"

Profiler* Profiler::s_instance = nullptr;
int Profiler::s_id = 0;

void Profiler::start(const std::string &name) {

    Profile* profile = get_profile(_profiles,  name);

    if (!profile) {
        profile = new Profile(name);
        if (!_profiling.empty())
            _profiling.top()->addProfile(profile);
        else
            _profiles.push_back(profile);
    }

    profile->setPaused(PROPERTY_B(PAUSED));

    profile->start();
    _profiling.push(profile);
}

void Profiler::stop(const std::string& name) {
    if (_profiling.empty())
        return;

    Profile* profile = _profiling.top();
    if (profile->name() != name)
        LOG_WARN("Profiler::Stop called with " + name + " on Profiler: " + profile->name());

    _profiling.pop();
    if (!_profiling.empty())
        profile->stop(_profiling.top()->idx());
    else
        profile->stop();
}

void Profiler::reset() {
    while (!_profiling.empty())
        _profiling.pop();

    for (const auto p : _profiles)
        delete p;

    _profiles.clear();
    s_id = 0;
}

std::vector<Profiler::Profile*>& Profiler::profiles() {
    return _profiles;
}

int Profiler::profileCount(const std::vector<Profile*> &profiles) {
    int count = profiles.size();
    for (const auto p : profiles) {
        count += profileCount(p->profiles());
    }
    return count;
}

void Profiler::resetMax() const {
    for (const auto &profile: _profiles) {
        profile->resetMax();
    }
}

Profiler::Profile::Profile(const std::string& name) : _name(std::move(name)){
    _data = (float*)malloc(PROFILER_SAMPLES * sizeof(float));
    memset(_data, 0, PROFILER_SAMPLES * sizeof(float));

    _id = s_id;
    s_id++;
}

Profiler::Profile::~Profile() {
    free(_data);
    for (const auto p : _profiles)
        delete p;
}

void Profiler::Profile::start() {
    _start = std::chrono::high_resolution_clock::now();
}

void Profiler::Profile::stop(const int idx) {
    if (_paused)
        return;

    const auto elapsed = std::chrono::high_resolution_clock::now() - _start;

    if (idx != _idx) {
        _idx = (_idx + 1) % PROFILER_SAMPLES;
        _data[_idx] = std::chrono::duration<float, std::milli>(elapsed).count();
    }
    else {
        _data[_idx] += std::chrono::duration<float, std::milli>(elapsed).count();
    }

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

int Profiler::Profile::id() const {
    return _id;
}

void Profiler::Profile::resetMax() {
    _max = _data[_idx];
    for (const auto profile : _profiles)
        profile->resetMax();
}

std::vector<Profiler::Profile *> & Profiler::Profile::profiles() {
    return _profiles;
}

void Profiler::Profile::addProfile(Profile *profile) {
    if (_profiles.empty()) {
        _profiles.push_back(profile);
        _profiles.push_back(new Profile("Sum: " + _name));
    }
    else {
        _profiles.push_back(_profiles[_profiles.size() - 1]);
        _profiles[_profiles.size() - 2] = profile;
    }
}

void Profiler::Profile::updateSum() const {
    if (_profiles.empty())
        return;

    Profile* sum = _profiles[_profiles.size() - 1];
    memset(sum->data(), 0, PROFILER_SAMPLES * sizeof(float));

    for (int i = 0; i < _profiles.size() - 1; i++) {
        for (int j = 0; j < PROFILER_SAMPLES; j++) {
            sum->data()[j] += _profiles[i]->data()[j];
            sum->_max = std::max(sum->_max, sum->data()[j]);
        }

        _profiles[i]->updateSum();
    }
    sum->_idx = _profiles[0]->_idx;
}

void Profiler::Profile::setPaused(const bool paused) {
    _paused = paused;
}

Profiler::~Profiler() {
    for (const auto profile: _profiles)
        delete profile;
}

Profiler::Profile * Profiler::get_profile(const std::vector<Profile*>& profiles, const std::string &name) {
    for (const auto p : profiles) {
        if (p->name() == name)
            return p;
        if (auto* result = get_profile(p->profiles(), name))
            return result;
    }

    return nullptr;
}
