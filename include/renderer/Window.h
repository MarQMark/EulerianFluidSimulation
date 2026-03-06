#ifndef EULERIANFLUIDSIMULATION_WINDOW_H
#define EULERIANFLUIDSIMULATION_WINDOW_H
#include <string>

#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, std::string  title);
    ~Window();

    void update() const;
    bool shouldClose() const;

private:
    int _width;
    int _height;
    std::string _title;

    GLFWwindow* _win{};
};


#endif //EULERIANFLUIDSIMULATION_WINDOW_H