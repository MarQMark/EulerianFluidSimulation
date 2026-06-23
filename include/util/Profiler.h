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
        Profile(const std::string& name);
        ~Profile();

        void start();
        void stop();

        float* data() const;
        float max() const;
        float cur() const;
        float avg(int count = 20) const;
        std::string name();
        unsigned int idx() const;

        void resetMax();
    private:
        std::string _name{};
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
    void stop();

    std::vector<Profile*>& profiles();

    void resetMax() const;

private:

    Profiler() = default;
    ~Profiler();

    std::vector<Profile*> _profiles{};
    std::stack<Profile*> _profiling{};
};


#endif //EULERIANFLUIDSIMULATION_PROFILER_H