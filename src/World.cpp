#include "World.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

World::World() {}

World::~World() {
    for (auto& [pos, chunk] : chunks)
        delete chunk;
    chunks.clear();
}

void World::generate(int radius) {
    // Generate a flat grid fo chunks
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            glm::ivec3 chunkPos(x, 0, z);
            Chunk* chunk = new Chunk(chunkPos);
            chunks[chunkPos] = chunk;
        }
    }

    // Build meshes after all chunks exist so cross-chunk culling works
    for (auto& [pos, chunk] : chunks)
        chunk->buildMesh(this);
}

void World::render(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    for (auto& [pos, chunk] : chunks) {
        // Offset each chunk to its correct world position
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
            pos.x * CHUNK_SIZE,
            pos.y * CHUNK_SIZE,
            pos.z * CHUNK_SIZE
        ));

        shader.setMat4("model",      model);
        shader.setMat4("view",       view);
        shader.setMat4("projection", projection);

        chunk->render();
    }
}

Chunk* World::getChunk(glm::ivec3 chunkPos) {
    auto it = chunks.find(chunkPos);
    if (it != chunks.end())
        return it->second;
    return nullptr;
}

uint8_t World::getVoxel(int worldX, int worldY, int worldZ) {
    // Convert world position to chunk position and local position
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkY = (int)floor((float)worldY / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    int localX = worldX - chunkX * CHUNK_SIZE;
    int localY = worldY - chunkY * CHUNK_SIZE;
    int localZ = worldZ - chunkZ * CHUNK_SIZE;

    Chunk* chunk = getChunk(glm::ivec3(chunkX, chunkY, chunkZ));
    if (!chunk) return 0; // Treat missing chunks as air
    return chunk->getVoxel(localX, localY, localZ);
}
