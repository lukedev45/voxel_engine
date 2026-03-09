#include "World.h"
#include <glm/gtc/matrix_transform.hpp>

World::World() {
    // Set up the noise generator
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.005f);   // Lower = broader hills
    noise.SetSeed(1337);          // Change for different terrain
}

World::~World() {
    for (auto& [pos, chunk] : chunks)
        delete chunk;
    chunks.clear();
}

void World::generate(int radius) {
    // Create all chunks first
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            glm::ivec3 chunkPos(x, 0, z);
            Chunk* chunk = new Chunk(chunkPos);
            chunks[chunkPos] = chunk;
        }
    }

    // Fill terrain using noise, then build meshes
    for (auto& [pos, chunk] : chunks)
        chunk->fillTerrain(noise);

    for (auto& [pos, chunk] : chunks)
        chunk->buildMesh(this);
}

void World::render(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    for (auto& [pos, chunk] : chunks) {
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
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkY = (int)floor((float)worldY / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    int localX = worldX - chunkX * CHUNK_SIZE;
    int localY = worldY - chunkY * CHUNK_SIZE;
    int localZ = worldZ - chunkZ * CHUNK_SIZE;

    Chunk* chunk = getChunk(glm::ivec3(chunkX, chunkY, chunkZ));
    if (!chunk) return 0;
    return chunk->getVoxel(localX, localY, localZ);
}
