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
    glfwSetWindowUserPointer(_win, this);

    glfwSetErrorCallback([](int error, const char* description) {
        LOG_ERROR(description)
    });

    if (!gladLoadGL(glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLEW");

    glfwSwapInterval(0);

    // Cursor Callback
    glfwSetCursorPosCallback(_win, [](GLFWwindow* window, double x, double y) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseCallback)
            callback(win, x, y);
    });

    // Mouse Button Callback
    glfwSetMouseButtonCallback(_win, [](GLFWwindow* window, int button, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseButtonCallback)
            callback(win, static_cast<MouseButton>(button), static_cast<Action>(action), mods);
    });

    // Scroll Callback
    glfwSetScrollCallback(_win, [](GLFWwindow* window, double x, double y) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseScrollCallback)
            callback(win, x, y);
    });

    // Key Callback
    glfwSetKeyCallback(_win, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->keyCallback)
            callback(win, static_cast<Key>(key), static_cast<Action>(action), mods);
    });

    // Resize Callback
    glfwSetFramebufferSizeCallback(_win, [](GLFWwindow* window, int width, int height) {
        auto w = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!w)
            return;

        w->_width = width;
        w->_height = height;

        for (auto &callback : w->resizeCallback)
            callback(w, width, height);
    });
}

Window::~Window() {
    glfwTerminate();
}

void Window::processEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(_win);
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

GLFWwindow* Window::getWindow() const {
    return _win;
}
