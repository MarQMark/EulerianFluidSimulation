#ifndef EULERIANFLUIDSIMULATION_PROPERTIES_H
#define EULERIANFLUIDSIMULATION_PROPERTIES_H
#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define PROPERTY(name, type) Properties::Get()->getPropertyValue<type>(Properties::name)
#define PROPERTY_I(name) PROPERTY(name, int)
#define PROPERTY_UI(name) PROPERTY(name, unsigned int)
#define PROPERTY_F(name) PROPERTY(name, float)
#define PROPERTY_B(name) PROPERTY(name, bool)

#define SET_PROPERTY(name, value, type) Properties::Get()->setPropertyValue<type>(Properties::name, value)
#define SET_PROPERTY_I(name, value) SET_PROPERTY(name, value, int)
#define SET_PROPERTY_UI(name, value) SET_PROPERTY(name, value, unsigned int)
#define SET_PROPERTY_F(name, value) SET_PROPERTY(name, value, float)
#define SET_PROPERTY_B(name, value) SET_PROPERTY(name, value, bool)

#define ADD_PROPERTY(name, value, type) Properties::Get()->addPropertyValue<type>(Properties::name, value)
#define ADD_PROPERTY_I(name, value) ADD_PROPERTY(name, value, int)
#define ADD_PROPERTY_UI(name, value) ADD_PROPERTY(name, value, unsigned int)
#define ADD_PROPERTY_F(name, value) ADD_PROPERTY(name, value, float)
#define ADD_PROPERTY_B(name, value) ADD_PROPERTY(name, value, bool)

#define ADD(cat, name, desc, val) {auto* p = new Property(name, desc, val); \
                                   cat.add(p); \
                                   add_property(p); }
#define ADD_HIDDEN(name, val) add_property(new Property(name, "", val));

#define PROP_TOKEN(token, name) static constexpr std::string_view token = name;

class Properties {
public:
    PROP_TOKEN(BRUSH_SIZE, "Brush Size")

    PROP_TOKEN(LAMBDA_MINUS, "Lambda-")
    PROP_TOKEN(LAMBDA_PLUS, "Lambda+")
    PROP_TOKEN(ETA, "Eta")
    PROP_TOKEN(LIQUID_THRESHOLD, "Liquid Threshold")

    PROP_TOKEN(GRAVITY_FORCE, "Gravity Force")
    PROP_TOKEN(GRAVITY_ENABLED, "Gravity Enabled")
    PROP_TOKEN(PUSH_FORCE, "Push Force")
    PROP_TOKEN(PUSH_ENABLED, "Push Enabled")

    PROP_TOKEN(MAX_STEPS, "Max Steps")
    PROP_TOKEN(SOLVER_THRESHOLD, "Threshold")

    PROP_TOKEN(DT, "DT")
    PROP_TOKEN(VISCOSITY, "Viscosity")

    PROP_TOKEN(WIDTH, "Width")
    PROP_TOKEN(HEIGHT, "Height")
    PROP_TOKEN(INIT_SHAPE, "Init Shape")

    PROP_TOKEN(DRAW_MODE, "Draw Mode")
    PROP_TOKEN(DRAW_VEL_ARROWS, "Draw vel arrows")
    PROP_TOKEN(RE_CALC_DIV, "Re-Calc Div")

    PROP_TOKEN(PAUSED, "Paused")

    PROP_TOKEN(SIMULATION_STEPS, "Simulation Steps")
    PROP_TOKEN(MAX_VALUE, "Max Value")
    PROP_TOKEN(TOTAL_RHO, "Total Rho")

    PROP_TOKEN(DISABLE_RHO_SHARPEN, "Disable Density Sharpen");
    PROP_TOKEN(DISABLE_RHO_ADVECT, "Disable Density Advection");
    PROP_TOKEN(DISABLE_VEL_ADVECT, "Disable Velocity Advection");
    PROP_TOKEN(DISABLE_ADD_FORCE, "Disable Add Force");
    PROP_TOKEN(DISABLE_PROJECT, "Disable Projection");

private:
    static Properties* s_instance;
    Properties() {
        auto& interaction = add_category("Interaction");
        ADD(interaction, BRUSH_SIZE, "Radius of Brush in screen pixels", 50.f)

        auto& simulation = add_category("Simulation");
        auto& simulation_rho = add_sub_category(simulation, "Density");
        ADD(simulation_rho, LAMBDA_MINUS, "Control how much compression is penalized", .5f)
        ADD(simulation_rho, LAMBDA_PLUS, "Control how much expansion is penalized", .1f)
        ADD(simulation_rho, ETA, "", 1.f)
        ADD(simulation_rho, LIQUID_THRESHOLD, "", 0.5f)

        auto& simulation_forces = add_sub_category(simulation, "Forces");
        ADD(simulation_forces, GRAVITY_FORCE, "", 0.02f)
        ADD(simulation_forces, GRAVITY_ENABLED, "", false)
        ADD(simulation_forces, PUSH_FORCE, "", 0.1f)
        ADD(simulation_forces, PUSH_ENABLED, "", false)

        auto& simulation_solver = add_sub_category(simulation, "Solver");
        ADD(simulation_solver, MAX_STEPS, "Maximum amount of iterations solver does", 2000)
        ADD(simulation_solver, SOLVER_THRESHOLD, "Threshold at which solver considers system solved", 1e-5f)

        ADD(simulation, DT, "Delta time", 1.67f)
        ADD(simulation, VISCOSITY, "", 0.f)

        auto& grid = add_category("Grid");
        ADD(grid, WIDTH, "Number of cells in width", 72)
        ADD(grid, HEIGHT, "Number of cells in height", 72)
        ADD(grid, INIT_SHAPE, "", InitShape::HALF)

        auto& rendering = add_category("Rendering");
        ADD(rendering, DRAW_MODE, "Which field is rendered in main window", DrawMode::FLUID)
        ADD(rendering, DRAW_VEL_ARROWS, "Draw velocity arrows", false)
        ADD(rendering, RE_CALC_DIV, "Recalculate Divergence after Projection", false)

        auto& controls = add_category("Controls");
        ADD(controls, PAUSED, "If simulation is paused", false)
        auto& controls_steps = add_sub_category(controls, "Steps");
        ADD(controls_steps, DISABLE_RHO_SHARPEN, "", false);
        ADD(controls_steps, DISABLE_RHO_ADVECT, "", false);
        ADD(controls_steps, DISABLE_VEL_ADVECT, "", false);
        ADD(controls_steps, DISABLE_ADD_FORCE, "", false);
        ADD(controls_steps, DISABLE_PROJECT, "", false);

        ADD_HIDDEN(SIMULATION_STEPS, 0)
        ADD_HIDDEN(MAX_VALUE, 0)
        ADD_HIDDEN(TOTAL_RHO, 0.f)
    }
    ~Properties();

#undef ADD
#undef ADD_HIDDEN
#undef PROP_TOKEN

public:
    class IProperty {
    public:
        IProperty(const std::string &name, const std::string& description)
        : name(name), description(description) {}
        IProperty(const std::string_view &name, const std::string& description)
       : name(name), description(description) {}

        virtual ~IProperty() = default;
        virtual void draw() = 0;

        const std::string name;
        const std::string description;
    };

private:
    template<typename T>
    class Property final : public IProperty{
    public:
        Property(const std::string &name, const std::string &description, T value)
            : IProperty(name, description), value(value) {
        }
        Property(const std::string_view &name, const std::string &description, T value)
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

    std::vector<Category*> _categories;
    Category& add_category(const std::string& name);
    SubCategory& add_sub_category(Category& cat, const std::string& name);

    std::map<std::string, IProperty*> _properties;
    void add_property(IProperty* p);

public:
    std::vector<Category*>& getCategories() {
        return _categories;
    }

    template<typename T>
    T getPropertyValue(const std::string_view& name) {
        if (!_properties.contains(name.data())) {
            std::cerr << "Property does not exist: " << name << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[name.data()]);
        if (!p) {
            std::cerr << "Property " << name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        return p->value;
    }
    template<typename T>
    void setPropertyValue(const std::string_view& name, T value) {
        if (!_properties.contains(name.data())) {
            std::cerr << "Property does not exist: " << name << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[name.data()]);
        if (!p) {
            std::cerr << "Property " << name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        p->value = value;
    }
    template<typename T>
    void addPropertyValue(const std::string_view& name, T value) {
        if (!_properties.contains(name.data())) {
            std::cerr << "Property does not exist: " << name << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[name.data()]);
        if (!p) {
            std::cerr << "Property " << name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        p->value += value;
    }


public:
    static Properties* Get() {
        if (!s_instance)
            s_instance = new Properties;

        return s_instance;
    }
    enum InitShape {
        FULL,
        HALF,
        SQUARE,
        EMPTY,
    };
    enum DrawMode {
        VELOCITY,
        DENSITY,
        DIVERGENCE,
        PRESSURE,
        FLUID,
        GAMMA,
        BETA,
    };
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIES_H