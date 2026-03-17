cmake_minimum_required(VERSION 3.20)
project(VoxelEngine)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(VoxelEngine
    src/main.cpp
    src/Window.cpp
    src/Shader.cpp
    src/Camera.cpp
    src/Chunk.cpp
    src/World.cpp
    src/TextureLoader.cpp
    vendor/glad/src/gl.c
)

target_include_directories(VoxelEngine PRIVATE src vendor vendor/glad/include)

target_link_libraries(VoxelEngine PRIVATE
    glfw
    glm::glm
)
