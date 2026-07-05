//
// Created by ruut on 3/26/26.
//

#ifndef FLUIDSIMSPH_CAMERA_H
#define FLUIDSIMSPH_CAMERA_H

#include "core/Config.h"
#include "glm/glm.hpp"
#include "simulation/scene/Transform.h"
#include "SceneNode.h"

class Window;

enum class Mode {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

class CameraNode : public SceneNode, public Transform {
public:
    CameraNode(Window* window, glm::vec3 position);

    void reset();

    void update(float deltaTime) override;
    void draw(const glm::mat4& view, const glm::mat4& projection) const override;

    void setHover(bool hover);
    void setAspectRatio(float aspect);
    void setMode(Mode mode);
    Mode getMode() const;

    float getFOV() const;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::mat4 getPerspectiveProjectionMatrix() const;
    glm::mat4 getOrthoProjectionMatrix() const;

    NodeProperties buildProperties() override;

    std::string getDefaultName() const override;

private:
    void updateVectors();

    Window* window;

    bool updateMouse = false;
    bool mouseDragging = false;

    Config config;

    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
    float keyboardSensitivity = 100.0f;
    float orthoSensitivity = 0.003f;
    float orthoScrollSensitivity = 0.1f;
    float perspectiveScrollSensitivity = 0.1f;
    float speedMultiplier = 2.0f;

    float fov = 90.0f;
    float orthoScale = 3.0f;

    float yaw = -90.0f;
    float pitch = 0.0f;
    float aspect = 1.0f;

    Mode mode = Mode::PERSPECTIVE;

    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec2 prevMousePosition = glm::vec2(-1.0f, -1.0f);
    glm::vec3 front, up, right;
};


#endif //FLUIDSIMSPH_CAMERA_H