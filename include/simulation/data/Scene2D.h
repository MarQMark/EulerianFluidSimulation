#ifndef EULERIANFLUIDSIMULATION_SCENE2D_H
#define EULERIANFLUIDSIMULATION_SCENE2D_H

#include "ScalField2D.h"
#include "simulation/scene/Scene.h"

class IGrid2D;

class Scene2D {
public:
    Scene2D();
    ~Scene2D();

    void saveScene(IGrid2D* grid, const std::string& path, const bool saveProperties) const;
    void loadScene(IGrid2D* grid, const std::string& path, const bool loadProperties);

    void resetScene(IGrid2D* grid) const;
    void clearScene();

    void loadImage(IGrid2D* grid, const std::string& path, int threshold);

    bool loadedScene() const;

private:
    ScalField2D* _boundary{};
    ScalField2D* _rho{};
};

#endif //EULERIANFLUIDSIMULATION_SCENE2D_H