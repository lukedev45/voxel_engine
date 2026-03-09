#pragma once
#include <cstdint>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

constexpr int CHUNK_SIZE = 16;

class Chunk {
public:
    glm::ivec3 position; // Position of this chunk in chunk coordinates (not world coordinates)

    Chunk(glm::ivec3 position);
    ~Chunk();

    // Voxel access
    uint8_t getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, uint8_t type);
    bool isInBounds(int x, int y, int z) const;

    // Mesh
    void buildMesh();
    void render() const;

private:
    uint8_t voxels[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];

    // GPU handles
    unsigned int VAO, VBO;
    int vertexCount;

    // Converts 3D index to flat 1D array index
    int indexOf(int x, int y, int z) const;

    // Adds a single quad (2 triangles) to the mesh
    void addFace(std::vector<float>& vertices,
                 float x, float y, float z,
                 int face);
};
