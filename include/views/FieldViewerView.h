#ifndef EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H
#define EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H

#include <string>

#include "Viewable.h"
#include "assets/Texture2D.h"
#include "simulation/data/ScalField2D.h"
#include "simulation/data/VecField2D.h"

class FieldViewerView : public Viewable {
public:
    FieldViewerView(VecField2D* field, const std::string& name);
    FieldViewerView(ScalField2D* field, const std::string& name);
    ~FieldViewerView();

    void render(void* renderData) override;

private:
    std::string _name{};

    Texture2D* _txt{};

    VecField2D* _v_field{};
    ScalField2D* _s_field{};

    float _max = 0;

    void render_vec_field();
    void render_scal_field();

    unsigned int field_width() const;
    unsigned int field_height() const;
};


#endif //EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H