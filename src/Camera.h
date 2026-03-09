#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position;
    float yaw, pitch;
    float moveSpeed;
    float sensitivity;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f));

    glm::mat4 getViewMatrix() const;
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;

    void processKeyboard(bool forward, bool backward, bool left, bool right, float deltaTime);
    void processMouse(float xOffset, float yOffset);
};
