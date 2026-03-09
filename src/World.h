#pragma once
#include <unordered_map>
#include <glm/glm.hpp>
#include "Chunk.h"
#include "Shader.h"

struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        size_t seed = 0;
        auto hash = std::hash<int>{};
        seed ^= hash(v.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

class World {
    public:
        World();
        ~World();

        void generate(int radius); // Gneerate chunks in a radius around origin
        void render(Shader& shader, const glm::mat4& view, const glm::mat4& projection);

        Chunk* getChunk(glm::ivec3 chunkPos);
        uint8_t getVoxel(int worldX, int worldY, int worldZ);

    private:
        std::unordered_map<glm::ivec3, Chunk*, IVec3Hash> chunks;
};
