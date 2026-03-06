#include "renderer/Window.h"

#include <stdexcept>
#include <utility>

Window::Window(const int width, const int height, std::string title) : _width(width), _height(height), _title(std::move(title)){
    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Failed to initialize GLFW");

    _win = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
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
