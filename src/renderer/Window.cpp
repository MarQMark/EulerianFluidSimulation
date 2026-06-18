#include "renderer/Window.h"
#include <stdexcept>
#include <utility>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

Window::Window(const int width, const int height, std::string title) : _width(width), _height(height), _title(std::move(title)){
    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    _win = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);

    glfwMakeContextCurrent(_win);

    if (!gladLoadGL(glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLEW");

    glfwSwapInterval(0);
}

Window::~Window() {
    glfwTerminate();
}

void Window::update() const {
    glfwSwapBuffers(_win);
    glfwPollEvents();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(_win);
}

GLFWwindow * Window::getWindow() const {
    return _win;
}
