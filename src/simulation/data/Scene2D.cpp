#include "simulation/data/Scene2D.h"

#include "simulation/Properties.h"
#include "simulation/data/IGrid.h"

Scene2D::Scene2D() {
}

Scene2D::~Scene2D() {
    delete _boundary;
    delete _rho;
}

void Scene2D::saveScene(IGrid2D* grid, const std::string &path, const bool saveProperties) const {
    Config config;

    nlohmann::json boundary = nlohmann::json::array();
    nlohmann::json fluid = nlohmann::json::array();

    for (unsigned int i = 0; i < grid->size(); ++i) {
        if (grid->boundary()[i] != 0.0f)
            boundary.push_back(i);

        if (grid->rho()[i] >= 0.5f)
            fluid.push_back(i);
    }

    if (saveProperties)
        config.set<nlohmann::json>("properties", Properties::Get()->toJson());

    config.set("width", grid->width());
    config.set("height", grid->height());
    config.set("boundary", boundary);
    config.set("fluid", fluid);
    config.save(path);
}

void Scene2D::loadScene(IGrid2D* grid, const std::string &path, const bool loadProperties) {
    SET_PROPERTY_B(SIMULATION_RESET, true);
    SET_PROPERTY_B(SIMULATION_INITIALIZED, false);

    Config config;

    if (config.load(path)) {
        if (loadProperties)
            Properties::Get()->fromJson(config.get<nlohmann::json>("properties", {}));

        const auto width  = config.get<unsigned int>("width", 0);
        const auto height = config.get<unsigned int>("height", 0);
        SET_PROPERTY_I(WIDTH_2D, width);
        SET_PROPERTY_I(HEIGHT_2D, height);
        if (_boundary) delete _boundary;
        if (_rho) delete _rho;
        _boundary = new ScalField2D(width, height);
        _rho = new ScalField2D(width, height);

        for (const auto &idx : config.get<nlohmann::json>("boundary", {}))
            (*_boundary)[idx] = 1.0f;
        for (const auto &idx : config.get<nlohmann::json>("fluid", {}))
            (*_rho)[idx] = 1.0f;
    }
}

void Scene2D::resetScene(IGrid2D* grid) const {
    if (!_boundary || !_rho)
        return;
    if (grid->width() != _boundary->width() || grid->height() != _boundary->height())
        return;

    for (int i = 0; i < grid->size(); i++) {
        grid->boundary()[i] = (*_boundary)[i];
        grid->rho()[i] = (*_rho)[i];
    }
}

void Scene2D::clearScene() {
    if (_boundary) delete _boundary;
    if (_rho) delete _rho;

    _boundary = nullptr;
    _rho = nullptr;
}

bool Scene2D::loadedScene() const {
    return _boundary && _rho;
}
