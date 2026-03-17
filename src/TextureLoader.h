#pragma once
#include <string>
#include <glad/gl.h>

class TextureLoader {
public:
    static unsigned int loadTexture(const std::string& path);
};
