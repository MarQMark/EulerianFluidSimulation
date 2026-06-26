//
// Created by ruut on 3/26/26.
//

#ifndef FLUIDSIMSPH_INPUT_H
#define FLUIDSIMSPH_INPUT_H

#include "glm/vec2.hpp"
#include "renderer/Window.h"

class InputManager {
public:
    void addInputWindow(Window* window);

    void reset();

    bool isKeyPressed(Window::Key key);
    bool isMouseButtonPressed(Window::MouseButton button);

    glm::vec2 getMousePosition() const;
    glm::vec2 getSrollDelta() const;

private:
    std::unordered_map<Window::Key, bool> keys;
    std::unordered_map<Window::MouseButton, bool> mouseButtons;

    glm::vec2 mousePosition { 0, 0 };
    glm::vec2 mouseScrollDelta { 0.0, 0.0 };
};

inline InputManager Input;

#endif //FLUIDSIMSPH_INPUT_H