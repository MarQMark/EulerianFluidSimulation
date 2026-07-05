//
// Created by ruut on 3/26/26.
//

#include "simulation/scene/nodes/CameraNode.h"
#include "core/Input.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "simulation/scene/NodeProperties.h"

CameraNode::CameraNode(Window* window, glm::vec3 position) : window(window) {
    this->name = "Camera";
    deletable = false;
    sceneGraphHideable = false;
    gizmoEnabled = false;
    savable = false;

    // Load config
    if (config.load("./configs/camera.json")) {
        movementSpeed                   = config.get<float>("movementSpeed",                movementSpeed                   );
        mouseSensitivity                = config.get<float>("mouseSensitivity",             mouseSensitivity                );
        keyboardSensitivity             = config.get<float>("keyboardSensitivity",          keyboardSensitivity             );
        orthoSensitivity                = config.get<float>("orthoSensitivity",             orthoSensitivity                );
        orthoScrollSensitivity          = config.get<float>("orthoScrollSensitivity",       orthoScrollSensitivity          );
        perspectiveScrollSensitivity    = config.get<float>("perspectiveScrollSensitivity", perspectiveScrollSensitivity    );
        speedMultiplier                 = config.get<float>("speedMultiplier",              speedMultiplier                 );
        fov                             = config.get<float>("fov",                          fov                             );
    } else {
        config.set("movementSpeed",                 movementSpeed                   );
        config.set("mouseSensitivity",              mouseSensitivity                );
        config.set("keyboardSensitivity",           keyboardSensitivity             );
        config.set("orthoSensitivity",              orthoSensitivity                );
        config.set("orthoScrollSensitivity",        orthoScrollSensitivity          );
        config.set("perspectiveScrollSensitivity",  perspectiveScrollSensitivity    );
        config.set("speedMultiplier",               speedMultiplier                 );
        config.set("fov",                           fov                             );
        config.save();
    }

    this->position = position;
    updateVectors();
}

void CameraNode::reset() {
    prevMousePosition = { -1, -1 };
}

void CameraNode::update(float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    if (Input::Get()->isKeyDown(Window::Key::LeftShift))
        velocity *= speedMultiplier;

    // Movement
    if (Input::Get()->isKeyDown(Window::Key::W))
        position += front * velocity;
    if (Input::Get()->isKeyDown(Window::Key::S))
        position -= front * velocity;
    if (Input::Get()->isKeyDown(Window::Key::A))
        position -= right * velocity;
    if (Input::Get()->isKeyDown(Window::Key::D))
        position += right * velocity;

    if (Input::Get()->isKeyDown(Window::Key::E))
        position += worldUp * velocity;
    if (Input::Get()->isKeyDown(Window::Key::Q))
        position -= worldUp * velocity;

    // Mouse Look (Keyboard)
    if (Input::Get()->isKeyDown(Window::Key::Up))
        pitch += keyboardSensitivity * deltaTime;
    if (Input::Get()->isKeyDown(Window::Key::Down))
        pitch -= keyboardSensitivity * deltaTime;
    if (Input::Get()->isKeyDown(Window::Key::Left))
        yaw -= keyboardSensitivity * deltaTime;
    if (Input::Get()->isKeyDown(Window::Key::Right))
        yaw += keyboardSensitivity * deltaTime;

    // Axis aligned presets
    if (Input::Get()->isKeyPressed(Window::Key::F1)) {
        position = { 0.0, 0.0, position.z == 0.0 ? 2.0f : position.z };
        pitch   = 0.0f;
        yaw     = -90.f;
    }

    if (Input::Get()->isKeyPressed(Window::Key::F2)) {
        position = { position.x == 0.0 ? -2.0f : position.x, 0.0, 0.0 };
        pitch   = 0.0f;
        yaw     = 0.0f;
    }

    if (Input::Get()->isKeyPressed(Window::Key::F3)) {
        position = { 0.0, position.y == 0.0 ? 2.0f : position.y, 0.0 };
        pitch   = -89.99f;
        yaw     = -90.0f;
    }

    if (Input::Get()->isKeyPressed(Window::Key::F5))
        mode = Mode::PERSPECTIVE;

    if (Input::Get()->isKeyPressed(Window::Key::F6))
        mode = Mode::ORTHOGRAPHIC;

    // Update mouse camera controls only if currently hovering the correct window or dragging
    if ((Input::Get()->isMouseButtonDown(Window::MouseButton::LEFT) && Input::Get()->isKeyDown(Window::Key::LeftControl)) || Input::Get()->isMouseButtonDown(Window::MouseButton::RIGHT)) {
        if (updateMouse) {
            mouseDragging = true;
            window->setCursorMode(Window::CursorMode::CURSOR_HIDDEN);
        }
    } else {
        mouseDragging = false;
        window->setCursorMode(Window::CursorMode::CURSOR_NORMAL);
    }

    if (!updateMouse && !mouseDragging)
        return;

    auto mousePos = Input::Get()->getMousePosition();

    float xDelta = prevMousePosition.x - mousePos.x;
    float yDelta = prevMousePosition.y - mousePos.y;

    if (prevMousePosition == glm::vec2(-1.0f, -1.0f)) {
        xDelta = 0;
        yDelta = 0;
    }

    // Mouse Look (MOUSE)
    if (Input::Get()->isMouseButtonDown(Window::MouseButton::RIGHT)) {
        yaw -= xDelta * mouseSensitivity;
        pitch += yDelta * mouseSensitivity;
        pitch = glm::clamp(pitch, -89.99f, 89.99f);
    }

    // Mouse Pan
    if (Input::Get()->isMouseButtonDown(Window::MouseButton::LEFT) && Input::Get()->isKeyDown(Window::Key::LeftControl)) {
        position += right * xDelta * orthoScale * orthoSensitivity;
        position -= up    * yDelta * orthoScale * orthoSensitivity;
    }

    prevMousePosition = Input::Get()->getMousePosition();

    // Mouse Scroll
    glm::vec2 mouseScroll = Input::Get()->getSrollDelta();
    if (mouseScroll.y != 0.0) {
        if (mode == Mode::PERSPECTIVE)
            position += front * mouseScroll.y * perspectiveScrollSensitivity;
        else
            orthoScale -= mouseScroll.y * orthoScrollSensitivity;
    }

    updateVectors();
}

void CameraNode::draw(const glm::mat4& view, const glm::mat4& projection) const { }

void CameraNode::setHover(bool hover) {
    this->updateMouse = hover;
}

void CameraNode::setAspectRatio(const float aspect) {
    this->aspect = aspect;
}

void CameraNode::setMode(Mode mode) {
    this->mode = mode;
}

Mode CameraNode::getMode() const {
    return mode;
}

float CameraNode::getFOV() const {
    return fov;
}

glm::mat4 CameraNode::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 CameraNode::getProjectionMatrix() const {
    return mode == Mode::PERSPECTIVE ? getPerspectiveProjectionMatrix() : getOrthoProjectionMatrix();
}

glm::mat4 CameraNode::getPerspectiveProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspect, 0.001f, 1000.0f);
}

glm::mat4 CameraNode::getOrthoProjectionMatrix() const {
    float halfH = orthoScale;
    float halfW = orthoScale * aspect;
    return glm::ortho(-halfW,halfW, -halfH, halfH, -100.f, 1000.0f);
}

NodeProperties CameraNode::buildProperties() {
    NodeProperties properties = SceneNode::buildProperties();

    properties.addSection("Transform");
    properties.addProperty(std::make_unique<Vec3Property>(
        "Position",
        [this] () { return getPosition(); },
        [this] (glm::vec3 position) { setPosition(position); }
    ));

    properties.addSection("Camera");
    properties.addProperty(std::make_unique<FloatProperty>(
        "FOV",
        [this] () { return fov; },
        [this] (float fov) { this->fov = fov; },
        30.0, 130, 0.5f
    ));

    properties.addProperty(std::make_unique<BoolProperty>(
        "Orthographic",
        [this] () { return mode == Mode::ORTHOGRAPHIC; },
        [this] (bool enabled) { setMode(enabled ? Mode::ORTHOGRAPHIC : Mode::PERSPECTIVE); }
    ));

    return properties;
}

std::string CameraNode::getDefaultName() const {
    return "Camera";
}

void CameraNode::updateVectors() {
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
