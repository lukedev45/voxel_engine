#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Window.h"
#include "Shader.h"
#include "Camera.h"
#include "World.h"
#include "TextureLoader.h"

Camera camera(glm::vec3(0.0f, 20.0f, 0.0f));
float lastX = 960.0f, lastY = 540.0f;
bool firstMouse = true;
bool leftMousePressed = false;
bool rightMousePressed = false;

// Block selection: 1=stone, 2=dirt, 3=grass
constexpr uint8_t BLOCK_TYPES[] = {1, 2, 3};
constexpr int NUM_BLOCK_TYPES = 3;
int selectedSlot = 2; // default to grass

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    selectedSlot -= (int)yoffset;
    selectedSlot = ((selectedSlot % NUM_BLOCK_TYPES) + NUM_BLOCK_TYPES) % NUM_BLOCK_TYPES;
}

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
    glfwSetScrollCallback(window.handle, scrollCallback);

    World world;

    Shader shader("assets/shaders/cube.vert", "assets/shaders/cube.frag");

    unsigned int atlasTexture = TextureLoader::loadTexture("assets/textures/atlas.png");
    shader.use();
    shader.setInt("texAtlas", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Sky setup — fullscreen quad rendered at the far plane
    Shader skyShader("assets/shaders/sky.vert", "assets/shaders/sky.frag");
    float skyQuad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
        -1.0f, -1.0f,
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyQuad), skyQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Block highlight — wireframe cube drawn on the targeted block
    Shader highlightShader("assets/shaders/highlight.vert", "assets/shaders/highlight.frag");
    // 12 edges of a unit cube, slightly expanded to avoid z-fighting
    float e = 0.002f; // expansion offset
    float highlightVerts[] = {
        // Bottom face edges
        -e,-e,-e,  1+e,-e,-e,
        1+e,-e,-e, 1+e,-e,1+e,
        1+e,-e,1+e, -e,-e,1+e,
        -e,-e,1+e, -e,-e,-e,
        // Top face edges
        -e,1+e,-e,  1+e,1+e,-e,
        1+e,1+e,-e, 1+e,1+e,1+e,
        1+e,1+e,1+e, -e,1+e,1+e,
        -e,1+e,1+e, -e,1+e,-e,
        // Vertical edges
        -e,-e,-e, -e,1+e,-e,
        1+e,-e,-e, 1+e,1+e,-e,
        1+e,-e,1+e, 1+e,1+e,1+e,
        -e,-e,1+e, -e,1+e,1+e,
    };
    unsigned int highlightVAO, highlightVBO;
    glGenVertexArrays(1, &highlightVAO);
    glGenBuffers(1, &highlightVBO);
    glBindVertexArray(highlightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, highlightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(highlightVerts), highlightVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // HUD shader (crosshair + hotbar)
    Shader hudShader("assets/shaders/crosshair.vert", "assets/shaders/crosshair.frag");

    // Crosshair — two small lines in NDC space
    float crosshairSize = 0.02f;
    float crosshairVerts[] = {
        -crosshairSize, 0.0f,
         crosshairSize, 0.0f,
         0.0f, -crosshairSize,
         0.0f,  crosshairSize,
    };
    unsigned int crosshairVAO, crosshairVBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVerts), crosshairVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Hotbar — 3 slots at the bottom center of screen
    float slotSize = 0.04f;
    float slotGap  = 0.01f;
    float hotbarY  = -0.90f;
    // Build slot quads (2 triangles each) + outline for selected slot
    // We'll update the VBO each frame for the selection highlight
    unsigned int hotbarVAO, hotbarVBO;
    glGenVertexArrays(1, &hotbarVAO);
    glGenBuffers(1, &hotbarVBO);
    glBindVertexArray(hotbarVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hotbarVBO);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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

        // Block selection via number keys
        if (window.isKeyPressed(GLFW_KEY_1)) selectedSlot = 0;
        if (window.isKeyPressed(GLFW_KEY_2)) selectedSlot = 1;
        if (window.isKeyPressed(GLFW_KEY_3)) selectedSlot = 2;

        // Block breaking (left click) and placing (right click)
        bool leftDown = glfwGetMouseButton(window.handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window.handle, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (leftDown && !leftMousePressed) {
            RaycastHit hit = world.raycast(camera.position, camera.getForward());
            if (hit.hit)
                world.setVoxel(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z, 0);
        }
        if (rightDown && !rightMousePressed) {
            RaycastHit hit = world.raycast(camera.position, camera.getForward());
            if (hit.hit) {
                glm::ivec3 placePos = hit.blockPos + hit.normal;
                // Don't place a block where the player is standing
                glm::ivec3 playerBlock((int)floor(camera.position.x), (int)floor(camera.position.y), (int)floor(camera.position.z));
                glm::ivec3 playerHead = playerBlock + glm::ivec3(0, 1, 0);
                if (placePos != playerBlock && placePos != playerHead)
                    world.setVoxel(placePos.x, placePos.y, placePos.z, BLOCK_TYPES[selectedSlot]);
            }
        }
        leftMousePressed = leftDown;
        rightMousePressed = rightDown;

        world.update(camera.position);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(70.0f),
            window.getAspectRatio(),
            0.1f,
            500.0f
        );

        // Draw sky first (depth write off so world renders on top)
        glDepthMask(GL_FALSE);
        skyShader.use();
        glm::mat4 invViewProj = glm::inverse(projection * view);
        skyShader.setMat4("invViewProj", invViewProj);
        skyShader.setVec3("cameraPos", camera.position);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // Draw world
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture);
        shader.use();
        world.render(shader, view, projection);

        // Draw block highlight wireframe
        RaycastHit highlightHit = world.raycast(camera.position, camera.getForward());
        if (highlightHit.hit) {
            glDisable(GL_CULL_FACE);
            highlightShader.use();
            glm::mat4 highlightModel = glm::translate(glm::mat4(1.0f),
                glm::vec3(highlightHit.blockPos));
            highlightShader.setMat4("model", highlightModel);
            highlightShader.setMat4("view", view);
            highlightShader.setMat4("projection", projection);
            highlightShader.setVec3("color", glm::vec3(0.1f, 0.1f, 0.1f));
            glLineWidth(2.0f);
            glBindVertexArray(highlightVAO);
            glDrawArrays(GL_LINES, 0, 24);
            glBindVertexArray(0);
            glEnable(GL_CULL_FACE);
        }

        // Draw HUD on top of everything
        glDisable(GL_DEPTH_TEST);
        hudShader.use();

        // Crosshair
        hudShader.setVec3("color", glm::vec3(1.0f));
        glBindVertexArray(crosshairVAO);
        glDrawArrays(GL_LINES, 0, 4);

        // Hotbar slots
        // Block colors: stone=gray, dirt=brown, grass=green
        static const glm::vec3 blockColors[] = {
            {0.5f, 0.5f, 0.5f},  // stone
            {0.45f, 0.3f, 0.15f}, // dirt
            {0.3f, 0.65f, 0.2f},  // grass
        };

        float totalWidth = NUM_BLOCK_TYPES * slotSize * 2 + (NUM_BLOCK_TYPES - 1) * slotGap;
        float startX = -totalWidth / 2.0f;

        glBindVertexArray(hotbarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, hotbarVBO);

        for (int i = 0; i < NUM_BLOCK_TYPES; i++) {
            float x = startX + i * (slotSize * 2 + slotGap);
            float y = hotbarY;

            // Draw filled quad for each slot
            float quad[] = {
                x, y,
                x + slotSize * 2, y,
                x + slotSize * 2, y + slotSize * 2,
                x + slotSize * 2, y + slotSize * 2,
                x, y + slotSize * 2,
                x, y,
            };
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
            hudShader.setVec3("color", blockColors[i] * (i == selectedSlot ? 1.0f : 0.5f));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Draw selection outline
            if (i == selectedSlot) {
                float outline[] = {
                    x, y,  x + slotSize * 2, y,
                    x + slotSize * 2, y,  x + slotSize * 2, y + slotSize * 2,
                    x + slotSize * 2, y + slotSize * 2,  x, y + slotSize * 2,
                    x, y + slotSize * 2,  x, y,
                };
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(outline), outline);
                hudShader.setVec3("color", glm::vec3(1.0f));
                glDrawArrays(GL_LINES, 0, 8);
            }
        }

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        window.swapAndPoll();
    }

    return 0;
}
