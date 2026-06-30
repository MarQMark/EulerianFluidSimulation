#ifndef EULERIANFLUIDSIMULATION_PROPERTIES_H
#define EULERIANFLUIDSIMULATION_PROPERTIES_H
#include <assert.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "util/Logger.h"

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

#define ADD_D(dim, cat, name, desc, val) {auto* p = new Property(name, desc, val, dim); \
                                          cat.add(p); \
                                          add_property(p); }
#define ADD_2D(cat, name, desc, val) ADD_D(2, cat, name, desc, val)
#define ADD_3D(cat, name, desc, val) ADD_D(3, cat, name, desc, val)
#define ADD(cat, name, desc, val)    ADD_D(0, cat, name, desc, val)

#define ADD_HIDDEN_D(dim, name, val)  add_property(new Property(name, "", val, dim));
#define ADD_HIDDEN_2D(dim, name, val) ADD_HIDDEN_D(2, name, val)
#define ADD_HIDDEN_3D(dim, name, val) ADD_HIDDEN_D(3, name, val)
#define ADD_HIDDEN(name, val)         ADD_HIDDEN_D(0, name, val)

#define PROP_TOKEN(token, name) static constexpr std::string_view token = name;

class Properties {
public:
    PROP_TOKEN(SIMULATION_TYPE, "Simulation Type")
    PROP_TOKEN(SIMULATION_REBUILD, "Simulation Rebuild")

    PROP_TOKEN(BRUSH_SIZE, "Brush Size")

    PROP_TOKEN(LAMBDA_MINUS, "Lambda-")
    PROP_TOKEN(LAMBDA_PLUS, "Lambda+")
    PROP_TOKEN(ETA, "Eta")
    PROP_TOKEN(LIQUID_THRESHOLD, "Liquid Threshold")
    PROP_TOKEN(RHO_D, "D")

    PROP_TOKEN(GRAVITY_FORCE, "Gravity Force")
    PROP_TOKEN(GRAVITY_ENABLED, "Gravity Enabled")
    PROP_TOKEN(PUSH_FORCE, "Push Force")
    PROP_TOKEN(PUSH_ENABLED, "Push Enabled")

    PROP_TOKEN(SOLVER_TYPE, "Solver")
    PROP_TOKEN(SOLVER_CHECK_INTERVAL, "Check Interval")
    PROP_TOKEN(MAX_STEPS, "Max Steps")
    PROP_TOKEN(SOLVER_THRESHOLD, "Threshold")

    PROP_TOKEN(DT, "DT")
    PROP_TOKEN(VISCOSITY, "Viscosity")

    PROP_TOKEN(WIDTH, "Width")
    PROP_TOKEN(HEIGHT, "Height")
    PROP_TOKEN(DEPTH, "Depth")
    PROP_TOKEN(INIT_SHAPE, "Init Shape")

    PROP_TOKEN(DRAW_MODE, "Draw Mode")
    PROP_TOKEN(DRAW_VEL_ARROWS, "Draw vel arrows")
    PROP_TOKEN(RE_CALC_DIV, "Re-Calc Div")

    PROP_TOKEN(PAUSED, "Paused")

    PROP_TOKEN(SIMULATION_STEPS, "Simulation Steps")
    PROP_TOKEN(MAX_VALUE, "Max Value")
    PROP_TOKEN(TOTAL_RHO, "Total Rho")
    PROP_TOKEN(SOLVE_STEPS, "Solve Steps")

    PROP_TOKEN(DISABLE_VEL_EXTRAPOLATE, "Disable Velocity Extrapolation");
    PROP_TOKEN(DISABLE_RHO_SHARPEN, "Disable Density Sharpen");
    PROP_TOKEN(DISABLE_RHO_ADVECT, "Disable Density Advection");
    PROP_TOKEN(DISABLE_VEL_ADVECT, "Disable Velocity Advection");
    PROP_TOKEN(DISABLE_ADD_FORCE, "Disable Add Force");
    PROP_TOKEN(DISABLE_PROJECT, "Disable Projection");

private:
    static Properties* s_instance;
    Properties() {
        auto& interaction = add_category("Interaction");
        ADD_2D(interaction, BRUSH_SIZE, "Radius of Brush in screen pixels", 50.f)

        auto& simulation = add_category("Simulation");
        auto& simulation_rho = add_sub_category(simulation, "Density");
        ADD(simulation_rho, LAMBDA_MINUS, "Control how much compression is penalized", .5f)
        ADD(simulation_rho, LAMBDA_PLUS, "Control how much expansion is penalized", .1f)
        ADD(simulation_rho, ETA, "", 1.f)
        ADD(simulation_rho, LIQUID_THRESHOLD, "", 0.5f)
        ADD(simulation_rho, RHO_D, "", 3.f)

        auto& simulation_forces = add_sub_category(simulation, "Forces");
        ADD(simulation_forces, GRAVITY_FORCE, "", 0.02f)
        ADD(simulation_forces, GRAVITY_ENABLED, "", true)
        ADD(simulation_forces, PUSH_FORCE, "", 0.1f)
        ADD(simulation_forces, PUSH_ENABLED, "", false)

        auto& simulation_solver = add_sub_category(simulation, "Solver");
        ADD(simulation_solver, SOLVER_TYPE, "Which algorithm is used to solve system of equations", SolverType::JACOBI)
        ADD(simulation_solver, SOLVER_CHECK_INTERVAL, "Amount of steps CG takes before checking for convergence", 100)
        ADD(simulation_solver, MAX_STEPS, "Maximum amount of iterations solver does", 2000)
        ADD(simulation_solver, SOLVER_THRESHOLD, "Threshold at which solver considers system solved", 1e-5f)

        ADD(simulation, DT, "Delta time", 1.67f)
        ADD(simulation, VISCOSITY, "", 0.f)

        auto& grid = add_category("Grid");
        ADD(grid, WIDTH, "Number of cells in width", 72)
        ADD(grid, HEIGHT, "Number of cells in height", 72)
        ADD_3D(grid, DEPTH, "Number of cells in depth", 72)
        ADD(grid, INIT_SHAPE, "", InitShape::HALF)

        auto& rendering = add_category("Rendering");
        ADD_2D(rendering, DRAW_MODE, "Which field is rendered in main window", DrawMode::FLUID)
        ADD_2D(rendering, DRAW_VEL_ARROWS, "Draw velocity arrows", false)
        ADD_2D(rendering, RE_CALC_DIV, "Recalculate Divergence after Projection", false)

        auto& controls = add_category("Controls");
        ADD(controls, PAUSED, "If simulation is paused", false)
        auto& controls_steps = add_sub_category(controls, "Steps");
        ADD(controls_steps, DISABLE_VEL_EXTRAPOLATE, "", false);
        ADD(controls_steps, DISABLE_RHO_SHARPEN, "", false);
        ADD(controls_steps, DISABLE_RHO_ADVECT, "", false);
        ADD(controls_steps, DISABLE_VEL_ADVECT, "", false);
        ADD(controls_steps, DISABLE_ADD_FORCE, "", false);
        ADD(controls_steps, DISABLE_PROJECT, "", false);

        ADD_HIDDEN(SIMULATION_STEPS, 0)
        ADD_HIDDEN(MAX_VALUE, 0)
        ADD_HIDDEN(TOTAL_RHO, 0.f)
        ADD_HIDDEN(SOLVE_STEPS, 0)

        ADD_HIDDEN(SIMULATION_TYPE, SimulatorType::CPU_2D);
        ADD_HIDDEN(SIMULATION_REBUILD, false);
    }
    ~Properties();

#undef ADD
#undef ADD_HIDDEN
#undef PROP_TOKEN

public:
    class IProperty {
    public:
        IProperty(std::string name, std::string  description, const int dimension = 0)
        : name(std::move(name)), description(std::move(description)), dimension(dimension) {}
        IProperty(const std::string_view &name, std::string  description, const int dimension = 0)
       : name(name), description(std::move(description)), dimension(dimension) {}

        virtual ~IProperty() = default;
        virtual void draw() = 0;

        const std::string name;
        const std::string description;
        const int dimension;
    };

private:
    template<typename T>
    class Property final : public IProperty{
    public:
        Property(const std::string &name, const std::string &description, T value, const int dimension = 0)
            : IProperty(name, description, dimension), value(value) {
        }
        Property(const std::string_view &name, const std::string &description, T value, const int dimension = 0)
            : IProperty(name, description, dimension), value(value) {
        }
        T value;
        void draw() override;
    };

    class ICategory {
    public:
        virtual ~ICategory() {
            for (const auto p : _properties)
                delete p;
        }
        virtual void add(IProperty* property) {
            _properties.push_back(property);
        }

        class Iterator {
        public:
            Iterator(const std::vector<IProperty*>::iterator current,
                     const std::vector<IProperty*>::iterator end)
                : current_(current), end_(end)
            {
                skipHidden();
            }

            IProperty*& operator*() const {
                return *current_;
            }

            Iterator& operator++() {
                ++current_;
                skipHidden();
                return *this;
            }

            bool operator!=(const Iterator& other) const {
                return current_ != other.current_;
            }

        private:
            void skipHidden() {
                while (current_ != end_ && !matchesDimension(*current_))
                    ++current_;
            }

            std::vector<IProperty*>::iterator current_;
            std::vector<IProperty*>::iterator end_;
        };
        Iterator begin() {
            return Iterator(_properties.begin(), _properties.end());
        }
        Iterator end() {
            return Iterator(_properties.end(), _properties.end());
        }

        bool empty() {
            unsigned int count = 0;
            for (const auto p : *this)
                count++;

            return (count == 0);
        }
    private:
        std::vector<IProperty*> _properties;
    };
    class SubCategory final : public ICategory {
    public:
        explicit SubCategory(const std::string& name) : name(name) {}
        std::string name;
    };
    class Category final : public ICategory {
    public:
        explicit Category(const std::string& name) : name(name) {}
        ~Category() override {
            for (const auto cat : sub_categories)
                delete cat;
        }
        std::string name;
        std::vector<SubCategory*> sub_categories;
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

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Accessing Property ";
            ss << name;
            ss << " while simulation dimension does not match property dimension (" << p->dimension << ")";
            LOG_WARN(ss.str())
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

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Setting Property ";
            ss << name;
            ss << " while simulation dimension does not match property dimension (" << p->dimension << ")";
            LOG_WARN(ss.str())
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

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Adding Property ";
            ss << name;
            ss << " while simulation dimension does not match property dimension (" << p->dimension << ")";
            LOG_WARN(ss.str())
        }

        p->value += value;
    }

private:
    static bool matchesDimension(const IProperty* property) {
        if (property->dimension == 0)
            return true;

        const auto simType = PROPERTY(SIMULATION_TYPE, SimulatorType);
        if (property->dimension == 2 && simType <= END_2D_SIMULATIONS)
            return true;

        if (property->dimension == 3 && simType >  END_2D_SIMULATIONS)
            return true;

        return false;
    }

public:
    static Properties* Get() {
        if (!s_instance)
            s_instance = new Properties;

        return s_instance;
    }
    enum SimulatorType {
        CPU_2D,
        OPENGL_2D,
        END_2D_SIMULATIONS = OPENGL_2D,
        CPU_3D,
    };
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
    enum SolverType {
        EIGEN_CG,
        EIGEN_BiCGSTAB,
        SELF_CG,
        JACOBI
    };
};


#endif //EULERIANFLUIDSIMULATION_PROPERTIES_H