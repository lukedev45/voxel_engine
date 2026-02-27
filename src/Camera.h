#pragma once
#include <glm/glm.hpp>

class Camera {
    public:
    glm::vec3 position;
    float yaw, pitch;
    float moveSpeed;
    float sensitivity;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 3.0f));

    glm::mat4 getView
}
