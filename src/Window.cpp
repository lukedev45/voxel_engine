#include "Window.h"
#include <iostream>

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

Window::Window(int width, int height, const std::string& title)
    : width(width), height(height)
{
    if (!glfwInit()) {
        std::cerr << "ERROR: Failed to initialise GLFW\n";
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle) {
        std::cerr << "ERROR: Failed to create GLFW window\n";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(handle);
    glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "ERROR: Failed to initialise GLAD\n";
        exit(-1);
    }

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glfwSwapInterval(1);
}

Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(handle); }
void Window::swapAndPoll() const { glfwSwapBuffers(handle); glfwPollEvents(); }
bool Window::isKeyPressed(int key) const { return glfwGetKey(handle, key) == GLFW_PRESS; }
