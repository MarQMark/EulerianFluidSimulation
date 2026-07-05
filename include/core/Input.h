//
// Created by ruut on 3/26/26.
//

#ifndef FLUIDSIMSPH_INPUT_H
#define FLUIDSIMSPH_INPUT_H

#include "glm/vec2.hpp"
#include "renderer/Window.h"

struct KeyState {
    bool current = false;
    bool previous = false;
};

class Input {
public:
    static Input* Get() {
        if (!s_instance)
            s_instance = new Input();

        return s_instance;
    }

    void addInputWindow(Window* window);

    void update();

    bool isKeyPressed(Window::Key key);
    bool isKeyDown(Window::Key key);
    bool isKeyReleased(Window::Key key);

    bool isMouseButtonPressed(Window::MouseButton button);
    bool isMouseButtonDown(Window::MouseButton button);
    bool isMouseButtonReleased(Window::MouseButton button);

    glm::vec2 getMousePosition() const;
    glm::vec2 getSrollDelta() const;

    void setInputBlocked(bool blocked);
    bool isInputBlocked() const;

private:
    Input() = default;
    static Input* s_instance;

    bool _blocked = false;

    std::unordered_map<Window::Key, KeyState> _keys;
    std::unordered_map<Window::MouseButton, KeyState> _mouseButtons;

    glm::vec2 _mousePosition { 0, 0 };
    glm::vec2 _mouseScrollDelta { 0.0, 0.0 };
};

#endif //FLUIDSIMSPH_INPUT_H