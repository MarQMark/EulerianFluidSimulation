#ifndef RENDERER_H
#define RENDERER_H
#include <simulation/Grid.h>

#include "Texture2D.h"


class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void render(Grid* grid);

    Texture2D* getTexture2D() const;

private:
    Texture2D* _txt;
    int _width;
    int _height;
};



#endif //RENDERER_H
