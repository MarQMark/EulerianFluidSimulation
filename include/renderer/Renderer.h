#ifndef RENDERER_H
#define RENDERER_H

#include "Texture2D.h"
#include "simulation/Grid.h"
#include "simulation/ScalField.h"


class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void render(Grid* grid, ScalField& boundary);

    Texture2D* getTexture2D() const;

private:
    Texture2D* _txt;
    int _width;
    int _height;
};



#endif //RENDERER_H
