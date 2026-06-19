#include "renderer/Window.h"
#include <stdexcept>
#include <utility>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "util/Logger.h"

Window::Window(const int width, const int height, std::string title) : _width(width), _height(height), _title(std::move(title)){
    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    _win = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);

    glfwMakeContextCurrent(_win);

    glfwSetErrorCallback([](int error, const char* description) {
        LOG_ERROR(description)
    });

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

void Window::setCursorMode(CursorMode mode) const {
    glfwSetInputMode(_win, GLFW_CURSOR, static_cast<int>(mode));
}

void Window::addMouseCallback(const std::function<void(Window*, double, double)>&callback) {
    mouseCallback.push_back(callback);
}

void Window::addMouseButtonCallback(const std::function<void(Window*, MouseButton, Action, int)>& callback) {
    mouseButtonCallback.push_back(callback);
}

void Window::addMouseScrollCallback(const std::function<void(Window*, double, double)>& callback) {
    mouseScrollCallback.push_back(callback);
}

void Window::addKeyCallback(const std::function<void(Window*, Key, Action, int)>& callback) {
    keyCallback.push_back(callback);
}

void Window::addResizeCallback(const std::function<void(Window*, int, int)>& callback) {
    resizeCallback.push_back(callback);
}

GLFWwindow * Window::getWindow() const {
    return _win;
}
