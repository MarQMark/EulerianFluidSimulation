#ifndef EULERIANFLUIDSIMULATION_PROPERTIES_H
#define EULERIANFLUIDSIMULATION_PROPERTIES_H


class Properties {
private:
    static Properties* s_instance;

public:
    static Properties* Get() {
        if (!s_instance)
            s_instance = new Properties;

        return s_instance;
    }

    float BRUSH_SIZE = 50;

    float VISCOSITY = 0.f;

    int MAX_STEPS = 10000;
    float THRESHOLD = 1.e-5f;

    float LIQUID_THRESHOLD = 0.5f;
    float LAMBDA = .5f;
    float ETA = 1.f;

    int WIDTH = 72;
    int HEIGHT = 72;

    float DT = 1.67f;

    bool GRAVITY = false;
    float GRAVITY_FORCE = .0098f;

    bool PUSH = false;
    float PUSH_FORCE = .1f;

    enum InitShape {
        FULL,
        HALF,
        SQUARE,
        EMPTY,
    };

    InitShape INIT_SHAPE = HALF;

    bool PAUSED = false;
    int SIMULATION_STEPS = 0;

    enum DrawMode {
        VELOCITY,
        DENSITY,
        DIVERGENCE,
        PRESSURE,
        FLUID,
        GAMMA,
        BETA,
    };
    DrawMode DRAW_MODE = FLUID;
    bool DRAW_VEL_ARROWS = false;
    float MAX_VALUE = 0;

    bool RE_CALC_DIV = false;

    float TOTAL_RHO = 0;
private:
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIES_H