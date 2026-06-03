#ifndef EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H
#define EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H

#include <string>

#include "Viewable.h"
#include "renderer/Texture2D.h"
#include "simulation/ScalField.h"
#include "simulation/VecField.h"

class FieldViewerView : public Viewable {
public:
    FieldViewerView(VecField* field, const std::string& name);
    FieldViewerView(ScalField* field, const std::string& name);
    ~FieldViewerView();

    void render() override;

private:
    std::string _name{};

    Texture2D* _txt{};

    VecField* _v_field{};
    ScalField* _s_field{};

    float _max = 0;

    void render_vec_field();
    void render_scal_field();

    unsigned int field_width() const;
    unsigned int field_height() const;
};


#endif //EULERIANFLUIDSIMULATION_FIELDVIEWERVIEW_H