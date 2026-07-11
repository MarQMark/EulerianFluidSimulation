#ifndef EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW3D_H
#define EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW3D_H

#include <string>

#include "Viewable.h"
#include "assets/Texture2D.h"
#include "simulation/Properties.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/data/VecField3D.h"

class FieldViewerView3D : public Viewable {
public:
    FieldViewerView3D(Properties::DrawMode type, std::string name);
    ~FieldViewerView3D() override;

    void render(void* renderData) override;

private:
    Properties::DrawMode _type;
    std::string _name{};

    Texture2D* _txt{};

    float _max = 0;

    bool _changed_dim = false;
    int _dim = 1;
    int _level = 0;
    unsigned int _sizes[3] = {0, 0, 0};
    unsigned int _used_sizes[2] = {0, 0};

    void render_vec_field(VecField3D& field);
    void render_scal_field(ScalField3D& field);
};

#endif //EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW3D_H