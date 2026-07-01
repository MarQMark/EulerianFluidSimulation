#ifndef RENDERER_H
#define RENDERER_H

#include "simulation/Grid2D.h"
#include "simulation/ScalField2D.h"

class Texture2D;

class Renderer2D {
public:
    Renderer2D(int width, int height);
    ~Renderer2D();

    void render(const Grid2D* grid);

    Texture2D* getTexture2D() const;

    static void drawField(std::vector<float>& data, VecField2D& field, int chanel, float& max);
    static void drawField(std::vector<float>& data, ScalField2D& field, int chanel, float& max);
private:
    Texture2D* _txt;
    int _width;
    int _height;
};



#endif //RENDERER_H
