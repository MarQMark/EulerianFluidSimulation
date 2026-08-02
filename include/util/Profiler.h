#ifndef EULERIANFLUIDSIMULATION_PROFILER_H
#define EULERIANFLUIDSIMULATION_PROFILER_H

#include <chrono>
#include <vector>
#include <stack>
#include <string>

#define PROFILER_SAMPLES 1000


// This necessitates that the Profiler exists as a pointer with the name profiler
#define PROFILE(name, code)                \
                {                          \
                    profiler->start(name); \
                    code                   \
                    profiler->stop(name);  \
                }

#define PROFILE_FULL(name, code)                          \
                {                                         \
                    Profiler* profiler = Profiler::Get(); \
                    profiler->start(name);                \
                    code                                  \
                    profiler->stop(name);                 \
                }

class Profiler {
private:
    static Profiler* s_instance;
    static int s_id;

public:
    class Profile {
    public:
        Profile(const std::string& name);
        ~Profile();

        void start();
        void stop(int idx = -1);

        float* data() const;
        float max() const;
        float cur() const;
        float avg(int count = 20) const;
        std::string name();
        unsigned int idx() const;
        int id() const ;

        void resetMax();

        std::vector<Profile*>& profiles();
        void addProfile(Profile* profile);
        void updateSum() const;

        void setPaused(bool paused);

    private:
        std::string _name{};
        float* _data{};
        float _max = 0;
        int _id;
        unsigned int _idx = PROFILER_SAMPLES;
        std::chrono::high_resolution_clock::time_point _start;

        bool _paused = false;

        std::vector<Profile*> _profiles{};
    };

public:
    static Profiler* Get() {
        if (!s_instance)
            s_instance = new Profiler;

        return s_instance;
    }

    void start(const std::string& name);
    void stop(const std::string& name);

    void reset();

    std::vector<Profile*>& profiles();

    static int profileCount(const std::vector<Profile*>& profiles);

    void resetMax() const;

private:

    Profiler() = default;
    ~Profiler();

    std::vector<Profile*> _profiles{};
    std::stack<Profile*> _profiling{};

    static Profile* get_profile(const std::vector<Profile*>& profiles, const std::string& name);
};


#endif //EULERIANFLUIDSIMULATION_PROFILER_H