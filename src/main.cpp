#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "Chunk.h" 

Camera camera(glm::vec3(8.0f, 20.0f, 8.0f)); // Start above the chunk
float lastX = 640.0f, lastY = 360.0f;
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

    // Create a chunk at origin and build its mesh
    Chunk chunk(glm::ivec3(0, 0, 0));
    chunk.buildMesh();

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

        shader.use();

        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(70.0f),
            window.getAspectRatio(),
            0.1f,
            500.0f
        );

        shader.setMat4("model",      model);
        shader.setMat4("view",       view);
        shader.setMat4("projection", projection);

        // Render the chunk
        chunk.render();

        window.swapAndPoll();
    }

    return 0;
}
