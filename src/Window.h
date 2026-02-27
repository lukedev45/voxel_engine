#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    GLFWwindow* handle;
    int width, height;

    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void swapAndPoll() const;
    bool isKeyPressed(int key) const;
    float getAspectRatio() const { return (float)width / (float)height; }
};
