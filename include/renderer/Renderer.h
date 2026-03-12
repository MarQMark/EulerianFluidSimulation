#ifndef RENDERER_H
#define RENDERER_H
#include <simulation/Grid.h>

#include "Texture2D.h"


class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void render(const Grid* grid);

private:
};



#endif //RENDERER_H
