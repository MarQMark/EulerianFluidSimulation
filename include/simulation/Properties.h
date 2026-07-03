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

#define ADD_D(cat, token, dim, name, desc, val) {auto* p = new Property(name, desc, val, dim); \
                                                 int a = token; \
                                                 cat.add(p); \
                                                 add_property(token, p); }
#define ADD_2D(cat, token, name, desc, val) ADD_D(cat, token, 2, name, desc, val)
#define ADD_3D(cat, token, name, desc, val) ADD_D(cat, token, 3, name, desc, val)
#define ADD(cat, token, name, desc, val)    ADD_D(cat, token, 0, name, desc, val)

#define ADD_HIDDEN_D(token, dim, val)  add_property(token, new Property("", "", val, dim));
#define ADD_HIDDEN_2D(token, dim, val) ADD_HIDDEN_D(token, 2, val)
#define ADD_HIDDEN_3D(token, dim, val) ADD_HIDDEN_D(token, 3, val)
#define ADD_HIDDEN(token, val)         ADD_HIDDEN_D(token, 0, val)

static constexpr unsigned int COUNTER_START = __COUNTER__ + 1;
#define PROP_TOKEN(token) static constexpr unsigned int token = __COUNTER__ - COUNTER_START;

class Properties {
public:
    PROP_TOKEN(SIMULATION_TYPE)
    PROP_TOKEN(SIMULATION_REBUILD)
    PROP_TOKEN(SIMULATION_RESET)
    PROP_TOKEN(SIMULATION_INITIALIZED)

    PROP_TOKEN(SIMULATION_GRID_RESIZED)

    PROP_TOKEN(BRUSH_SIZE)

    PROP_TOKEN(LAMBDA_MINUS)
    PROP_TOKEN(LAMBDA_PLUS)
    PROP_TOKEN(ETA)
    PROP_TOKEN(LIQUID_THRESHOLD)
    PROP_TOKEN(RHO_D)

    PROP_TOKEN(GRAVITY_FORCE)
    PROP_TOKEN(GRAVITY_ENABLED)
    PROP_TOKEN(PUSH_FORCE)
    PROP_TOKEN(PUSH_ENABLED)

    PROP_TOKEN(AVLB_SOLVERS)
    PROP_TOKEN(SOLVER_TYPE)
    PROP_TOKEN(SOLVER_CHECK_INTERVAL)
    PROP_TOKEN(MAX_STEPS)
    PROP_TOKEN(SOLVER_THRESHOLD)

    PROP_TOKEN(DT)
    PROP_TOKEN(VISCOSITY)

    PROP_TOKEN(WIDTH_2D)
    PROP_TOKEN(HEIGHT_2D)

    PROP_TOKEN(WIDTH_3D)
    PROP_TOKEN(HEIGHT_3D)
    PROP_TOKEN(DEPTH_3D)
    PROP_TOKEN(INIT_SHAPE)

    PROP_TOKEN(DRAW_MODE)
    PROP_TOKEN(DRAW_VEL_ARROWS)
    PROP_TOKEN(RE_CALC_DIV)

    PROP_TOKEN(PAUSED)

    PROP_TOKEN(SIMULATION_STEPS)
    PROP_TOKEN(MAX_VALUE)
    PROP_TOKEN(TOTAL_RHO)
    PROP_TOKEN(SOLVE_STEPS)

    PROP_TOKEN(DISABLE_VEL_EXTRAPOLATE);
    PROP_TOKEN(DISABLE_RHO_SHARPEN);
    PROP_TOKEN(DISABLE_RHO_ADVECT);
    PROP_TOKEN(DISABLE_VEL_ADVECT);
    PROP_TOKEN(DISABLE_ADD_FORCE);
    PROP_TOKEN(DISABLE_PROJECT);

private:
    static Properties* s_instance;
    Properties() {
        _properties.resize((__COUNTER__ - COUNTER_START) + 1);

        auto& interaction = add_category("Interaction");
        ADD_2D(interaction, BRUSH_SIZE, "Brush Size", "Radius of Brush in screen pixels", 50.f)

        auto& simulation = add_category("Simulation");
        auto& simulation_rho = add_sub_category(simulation, "Density");
        ADD(simulation_rho, LAMBDA_MINUS, "Lambda-", "Control how much compression is penalized", .5f)
        ADD(simulation_rho, LAMBDA_PLUS, "Lambda+", "Control how much expansion is penalized", .1f)
        ADD(simulation_rho, ETA, "Eta", "", 1.f)
        ADD(simulation_rho, LIQUID_THRESHOLD, "Liquid Threshold", "", 0.5f)
        ADD(simulation_rho, RHO_D, "D", "", 3.f)

        auto& simulation_forces = add_sub_category(simulation, "Forces");
        ADD(simulation_forces, GRAVITY_FORCE, "Gravity Force", "", 0.02f)
        ADD(simulation_forces, GRAVITY_ENABLED, "Gravity Enabled", "", true)
        ADD(simulation_forces, PUSH_FORCE, "Push Force", "", 0.1f)
        ADD(simulation_forces, PUSH_ENABLED, "Push Enabled", "", false)

        auto& simulation_solver = add_sub_category(simulation, "Solver");
        ADD(simulation_solver, SOLVER_TYPE, "Solver", "Which algorithm is used to solve system of equations", SolverType::JACOBI)
        ADD(simulation_solver, SOLVER_CHECK_INTERVAL, "Check Interval", "Amount of steps CG takes before checking for convergence", 100)
        ADD(simulation_solver, MAX_STEPS, "Max Steps", "Maximum amount of iterations solver does", 2000)
        ADD(simulation_solver, SOLVER_THRESHOLD, "Threshold", "Threshold at which solver considers system solved", 1e-5f)

        ADD(simulation, DT, "DT", "Delta time", 1.67f)
        ADD(simulation, VISCOSITY, "Viscosity", "", 0.f)

        auto& grid = add_category("Grid");
        ADD_2D(grid, WIDTH_2D, "Width", "Number of cells in width", 72)
        ADD_2D(grid, HEIGHT_2D, "Height", "Number of cells in height", 72)
        ADD_2D(grid, INIT_SHAPE, "Init Shape", "", InitShape::HALF)

        auto& rendering = add_category("Rendering");
        ADD_2D(rendering, DRAW_MODE, "Draw Mode", "Which field is rendered in main window", DrawMode::FLUID)
        ADD_2D(rendering, DRAW_VEL_ARROWS, "Draw vel arrows", "Draw velocity arrows", false)
        ADD_2D(rendering, RE_CALC_DIV, "Re-Calc Div", "Recalculate Divergence after Projection", false)

        auto& controls = add_category("Controls");
        ADD(controls, PAUSED, "Paused", "If simulation is paused", false)
        auto& controls_steps = add_sub_category(controls, "Steps");
        ADD(controls_steps, DISABLE_VEL_EXTRAPOLATE, "Disable Velocity Extrapolation", "", false);
        ADD(controls_steps, DISABLE_RHO_SHARPEN, "Disable Density Sharpen", "", false);
        ADD(controls_steps, DISABLE_RHO_ADVECT, "Disable Density Advection", "", false);
        ADD(controls_steps, DISABLE_VEL_ADVECT, "Disable Velocity Advection", "", false);
        ADD(controls_steps, DISABLE_ADD_FORCE, "Disable Add Force", "", false);
        ADD(controls_steps, DISABLE_PROJECT, "Disable Projection", "", false);

        ADD_HIDDEN(WIDTH_3D, 10)
        ADD_HIDDEN(HEIGHT_3D, 10)
        ADD_HIDDEN(DEPTH_3D, 10)

        ADD_HIDDEN(SIMULATION_STEPS, 0)
        ADD_HIDDEN(MAX_VALUE, 0)
        ADD_HIDDEN(TOTAL_RHO, 0.f)
        ADD_HIDDEN(SOLVE_STEPS, 0)

        ADD_HIDDEN(SIMULATION_TYPE, SimulatorType::CPU_3D)
        ADD_HIDDEN(SIMULATION_REBUILD, false)
        ADD_HIDDEN(SIMULATION_RESET, false)
        ADD_HIDDEN(SIMULATION_INITIALIZED, false)
        ADD_HIDDEN(SIMULATION_GRID_RESIZED, false)
        ADD_HIDDEN(AVLB_SOLVERS, new std::vector<SolverType>())
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
        ~Property() override {
            if constexpr (std::is_pointer_v<T>) delete value;
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
        explicit SubCategory(std::string  name) : name(std::move(name)) {}
        std::string name;
    };
    class Category final : public ICategory {
    public:
        explicit Category(std::string  name) : name(std::move(name)) {}
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

    std::vector<IProperty*> _properties;
    void add_property(unsigned int token, IProperty* p);

public:
    std::vector<Category*>& getCategories() {
        return _categories;
    }

    template<typename T>
    T getPropertyValue(const unsigned int token) {
        if (_properties.size() <= token) {
            std::cerr << "Property does not exist: " << _properties[token] << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[token]);
        if (!p) {
            std::cerr << "Property " << p->name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Accessing Property ";
            ss << p->name;
            ss << " while simulation dimension does not match property dimension (" << p->dimension << ")";
            LOG_WARN(ss.str())
        }

        return p->value;
    }
    template<typename T>
    void setPropertyValue(const unsigned int token, T value) {
        if (_properties.size() <= token) {
            std::cerr << "Property does not exist: " << _properties[token]->name << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[token]);
        if (!p) {
            std::cerr << "Property " << p->name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Setting Property ";
            ss << p->name;
            ss << " while simulation dimension does not match property dimension (" << p->dimension << ")";
            LOG_WARN(ss.str())
        }

        p->value = value;
    }
    template<typename T>
    void addPropertyValue(const unsigned int token, T value) {
        if (_properties.size() <= token) {
            std::cerr << "Property does not exist: " << _properties[token] << std::endl;
            std::abort();
        }

        auto p = dynamic_cast<Property<T> *>(_properties[token]);
        if (!p) {
            std::cerr << "Property " << p->name << " type mismatch: " << typeid(T).name() << std::endl;
            std::abort();
        }

        if (!matchesDimension(p)) {
            std::stringstream ss;
            ss << "Adding Property ";
            ss << p->name;
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