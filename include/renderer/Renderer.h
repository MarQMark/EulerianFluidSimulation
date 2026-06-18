#ifndef RENDERER_H
#define RENDERER_H

#include "simulation/Grid.h"
#include "simulation/ScalField.h"

class Texture2D;

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void render(const Grid* grid);

    Texture2D* getTexture2D() const;

    static void drawField(std::vector<float>& data, VecField& field, int chanel, float& max);
    static void drawField(std::vector<float>& data, ScalField& field, int chanel, float& max);
private:
    Texture2D* _txt;
    int _width;
    int _height;
};



#endif //RENDERER_H
