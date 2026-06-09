#ifndef EULERIANFLUIDSIMULATION_PROPERTIES_H
#define EULERIANFLUIDSIMULATION_PROPERTIES_H
#include <map>
#include <string>
#include <vector>


class Properties {
private:
    static Properties* s_instance;
    Properties();
    ~Properties();

    class IProperty {
    public:
        IProperty(const std::string &name, const std::string& description)
        : name(name), description(description) {}

        virtual ~IProperty() = default;
        virtual void draw() = 0;

        const std::string name;
        const std::string description;
    };

    template<typename T>
    class Property final : public IProperty{
    public:
        Property(const std::string &name, const std::string &description, T value)
            : IProperty(name, description), value(value) {
        }
        T value;
        void draw() override;
    };

    struct SubCategory {
        ~SubCategory() {
            for (const auto p : properties)
                delete p;
        }
        std::string name;
        std::vector<IProperty*> properties;
        void add(IProperty* property) {
            properties.push_back(property);
        }
    };
    struct Category {
        ~Category() {
            for (const auto cat : sub_categories)
                delete cat;
            for (const auto p : properties)
                delete p;
        }
        std::string name;
        std::vector<SubCategory*> sub_categories;
        std::vector<IProperty*> properties;

        void add(SubCategory* sub_category) {
            sub_categories.push_back(sub_category);
        }
        void add(IProperty* property) {
            properties.push_back(property);
        }
    };

public:
    std::vector<Category*> _categories;
    Category& add_category(const std::string& name);
    SubCategory& add_sub_category(Category& cat, const std::string& name);

    std::map<std::string, IProperty*> _properties;
    void add_property(IProperty* p);

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
    float LAMBDA2 = .1f;
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