cmake_minimum_required(VERSION 3.20)
project(VoxelEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.make"
    CACHE STRING "")

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(VoxelEngine
    src/main.cpp
    src/Window.cpp
    src/Shader.cpp
    src/Camera.cpp
    )

target_link_libraries(VoxelEngine PRIVATE
    glfw
    glad::glad
    glm::glm
    )
