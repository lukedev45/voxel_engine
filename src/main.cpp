#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "World.h"

Camera camera(glm::vec3(0.0f, 20.0f, 0.0f));
float lastX = 960.0f, lastY = 540.0f;
bool firstMouse = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xOffset =  (xpos - lastX);
    float yOffset = -(ypos - lastY);
    lastX = xpos;
    lastY = ypos;
    camera.processMouse(xOffset, yOffset);
}

int main() {
    Window window(1920, 1080, "Voxel Engine");

    glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window.handle, mouseCallback);

    // Generate a 5x5 grid of chunks (radius 2)
    World world;
    world.generate(2);

    Shader shader("assets/shaders/cube.vert", "assets/shaders/cube.frag");

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float lastFrame = 0.0f;

    while (!window.shouldClose()) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (window.isKeyPressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window.handle, true);

        camera.processKeyboard(
            window.isKeyPressed(GLFW_KEY_W),
            window.isKeyPressed(GLFW_KEY_S),
            window.isKeyPressed(GLFW_KEY_A),
            window.isKeyPressed(GLFW_KEY_D),
            deltaTime
        );

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(70.0f),
            window.getAspectRatio(),
            0.1f,
            500.0f
        );

        shader.use();
        world.render(shader, view, projection);

        window.swapAndPoll();
    }

    return 0;
}
