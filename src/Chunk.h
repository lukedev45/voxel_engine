#pragma once
#include <cstdint>
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include "FastNoiseLite.h"

constexpr int CHUNK_SIZE = 16;

class World;

class Chunk {
public:
    glm::ivec3 position;

    Chunk(glm::ivec3 position);
    ~Chunk();

    uint8_t getVoxel(int x, int y, int z) const;
    void setVoxel(int x, int y, int z, uint8_t type);
    bool isInBounds(int x, int y, int z) const;

    void fillTerrain(FastNoiseLite& noise); // NEW
    void buildMesh(World* world = nullptr);
    void render() const;

private:
    uint8_t voxels[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];

    unsigned int VAO, VBO;
    int vertexCount;

    int indexOf(int x, int y, int z) const;
    void addFace(std::vector<float>& vertices, float x, float y, float z, int face, int tileIndex, const int ao[4]);
};
