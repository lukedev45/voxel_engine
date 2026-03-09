#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(glm::vec3 startPos)
    : position(startPos), yaw(-90.0f), pitch(0.0f),
      moveSpeed(5.0f), sensitivity(0.01f)
{}

glm::vec3 Camera::getForward() const {
    return glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));
}

glm::vec3 Camera::getRight() const {
    return glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + getForward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Camera::processKeyboard(bool forward, bool backward, bool left, bool right, float deltaTime) {
    float velocity = moveSpeed * deltaTime;
    if (forward)  position += getForward() * velocity;
    if (backward) position -= getForward() * velocity;
    if (right)    position += getRight() * velocity;
    if (left)     position -= getRight() * velocity;
}

void Camera::processMouse(float xOffset, float yOffset) {
    yaw   += xOffset * sensitivity;
    pitch += yOffset * sensitivity;
    // Clamp pitch so the camera can't flip upside down
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}
