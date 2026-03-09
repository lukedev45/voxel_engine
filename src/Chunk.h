#pragma once
#include <cstdint>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

constexpr int CHUNK_SIZE = 16;

// Forward declaration to avoid circular include
class World;

class Chunk {
    public:
        glm::ivec3 position;

        Chunk(glm::ivec3 position);
        ~Chunk();

        uint8_t getVoxel(int x, int y, int z) const;
        void setVoxel(int x, int y, int z, uint8_t type);
        bool isInBounds(int x, int y, int z) const;

        void buildMesh(World* world = nullptr); // World pointer for cross-chunk culling
        void render() const;

    private:
        uint8_t voxels[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];

        unsigned int VAO, VBO;
        int vertexCount;

        int indexOf(int x, int y, int z) const;
        void addFace(std::vector<float>& vertices, float x, float y, float z, int face);
};
