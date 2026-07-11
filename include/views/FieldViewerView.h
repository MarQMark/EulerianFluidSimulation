#ifndef EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H
#define EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H

#include <string>

#include "Viewable.h"
#include "assets/Texture2D.h"
#include "simulation/Properties.h"
#include "simulation/data/ScalField2D.h"
#include "simulation/data/VecField2D.h"

class FieldViewerView : public Viewable {
public:
    FieldViewerView(Properties::DrawMode type, std::string  name);
    ~FieldViewerView() override;

    void render(void* renderData) override;

private:
    Properties::DrawMode _type;
    std::string _name{};

    Texture2D* _txt{};

    float _max = 0;

    void render_vec_field(VecField2D& field);
    void render_scal_field(ScalField2D& field);
};


#endif //EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H