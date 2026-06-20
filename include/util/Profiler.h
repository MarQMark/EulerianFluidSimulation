#ifndef EULERIANFLUIDSIMULATION_PROFILER_H
#define EULERIANFLUIDSIMULATION_PROFILER_H

#include <chrono>
#include <map>
#include <stack>
#include <string>

#define PROFILER_SAMPLES 1000

class Profiler {
private:
    static Profiler* s_instance;

public:
    class Profile {
    public:
        Profile();
        ~Profile();

        void start();
        void stop();

        float* data() const;
        float max() const;
        float cur() const;
        float avg() const;
        unsigned int idx() const;

        void resetMax();
    private:
        float* _data{};
        float _max = 0;
        unsigned int _idx = PROFILER_SAMPLES;
        std::chrono::high_resolution_clock::time_point _start;
    };

public:
    static Profiler* Get() {
        if (!s_instance)
            s_instance = new Profiler;

        return s_instance;
    }

    void start(const std::string& name);
    void stop(const std::string& name = "");

    std::map<std::string, Profile *>& profiles();

    void resetMax();

private:

    Profiler() = default;
    ~Profiler();

    std::map<std::string, Profile*> _profiles{};
    std::stack<std::string> _profiling{};
};


#endif //EULERIANFLUIDSIMULATION_PROFILER_H