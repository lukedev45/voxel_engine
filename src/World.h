#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "Chunk.h"
#include "Shader.h"
#include "FastNoiseLite.h"

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

enum class ChunkState {
    TerrainReady,  // voxel data filled, mesh not yet built
    MeshReady      // fully renderable
};

struct ChunkEntry {
    Chunk* chunk = nullptr;
    ChunkState state = ChunkState::TerrainReady;
};

class World {
public:
    FastNoiseLite noise;

    static constexpr int LOAD_RADIUS    = 12;
    static constexpr int UNLOAD_RADIUS  = 14;
    static constexpr int LOAD_PER_FRAME = 2;
    static constexpr int REBUILD_PER_FRAME = 4;

    World();
    ~World();

    void update(const glm::vec3& cameraPos);
    void render(Shader& shader, const glm::mat4& view, const glm::mat4& projection);

    Chunk* getChunk(glm::ivec3 chunkPos);
    uint8_t getVoxel(int worldX, int worldY, int worldZ);

private:
    std::unordered_map<glm::ivec3, ChunkEntry, IVec3Hash> chunks;

    std::vector<glm::ivec3> loadQueue;
    std::vector<glm::ivec3> meshRebuildQueue;
    glm::ivec3 lastCameraChunk = glm::ivec3(INT_MAX);

    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    void rebuildLoadQueue(const glm::ivec3& cameraChunk);
    void processLoadQueue();
    void ensureMesh(const glm::ivec3& pos);
    void processMeshRebuilds();
    void unloadDistantChunks(const glm::ivec3& cameraChunk);
};
