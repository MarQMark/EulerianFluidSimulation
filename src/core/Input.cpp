//
// Created by ruut on 3/26/26.
//

#include "core/Input.h"

#include <iostream>
#include <ostream>
#include <ranges>

#include "renderer/Window.h"

Input* Input::s_instance = nullptr;

void Input::addInputWindow(Window* window) {
    // Mouse Position
    window->addMouseCallback([&](Window *, double x, double y) {
        _mousePosition = { static_cast<int>(x), static_cast<int>(y) };
    });

    // Mouse Scroll Delta
    window->addMouseScrollCallback([&](Window *, double x, double y) {
        _mouseScrollDelta = glm::vec2(x, y);
    });

    // Mouse Button
    window->addMouseButtonCallback([&](Window *, Window::MouseButton button, Window::Action action, int mods) {
        _mouseButtons[button].current = (action != Window::Action::RELEASE);
    });

    // Key
    window->addKeyCallback([&](Window *, Window::Key key, Window::Action action, int mods) {
        _keys[key].current = (action != Window::Action::RELEASE);
    });
}

void Input::update() {
    for (auto& state : _keys | std::views::values)
        state.previous = state.current;

    for (auto& state : _mouseButtons | std::views::values)
        state.previous = state.current;

    _mouseScrollDelta = glm::vec2(0.0f, 0.0f);
}

bool Input::isKeyPressed(Window::Key key) {
    if (_blocked)
        return false;

    auto it = _keys.find(key);
    if (it == _keys.end())
        return false;

    return it->second.current && !it->second.previous;
}

bool Input::isKeyDown(Window::Key key) {
    if (_blocked)
        return false;

    auto it = _keys.find(key);
    return it != _keys.end() && it->second.current;
}

bool Input::isKeyReleased(Window::Key key) {
    if (_blocked)
        return false;

    auto it = _keys.find(key);
    if (it == _keys.end())
        return false;

    return !it->second.current && it->second.previous;
}

bool Input::isMouseButtonPressed(Window::MouseButton button) {
    auto it = _mouseButtons.find(button);
    if (it == _mouseButtons.end())
        return false;

    return it->second.current && !it->second.previous;
}

bool Input::isMouseButtonDown(Window::MouseButton button) {
    auto it = _mouseButtons.find(button);
    return it != _mouseButtons.end() && it->second.current;
}

bool Input::isMouseButtonReleased(Window::MouseButton button) {
    auto it = _mouseButtons.find(button);
    if (it == _mouseButtons.end())
        return false;

    return !it->second.current && it->second.previous;
}

glm::vec2 Input::getMousePosition() const {
    return _mousePosition;
}

glm::vec2 Input::getSrollDelta() const {
    return _mouseScrollDelta;
}

void Input::setInputBlocked(bool blocked) {
    _blocked = blocked;
}

bool Input::isInputBlocked() const {
    return _blocked;
}
