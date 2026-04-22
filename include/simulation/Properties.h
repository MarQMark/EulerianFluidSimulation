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

    float a = 0.00366;

    int MAX_STEPS = 100000;
    float THRESHOLD = 1.e-5f;

    int WIDTH = 72;
    int HEIGHT = 72;

    float DT = 1.67f;
private:
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIES_H