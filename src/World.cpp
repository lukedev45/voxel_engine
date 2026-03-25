#include "World.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

static const glm::ivec3 NEIGHBOR_OFFSETS[4] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
};

World::World() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.005f);
    noise.SetSeed(1337);
}

World::~World() {
    for (auto& [pos, entry] : chunks)
        delete entry.chunk;
    chunks.clear();
}

glm::ivec3 World::worldToChunkPos(const glm::vec3& worldPos) const {
    return glm::ivec3(
        (int)floor(worldPos.x / CHUNK_SIZE),
        0,
        (int)floor(worldPos.z / CHUNK_SIZE)
    );
}

void World::update(const glm::vec3& cameraPos) {
    glm::ivec3 cameraChunk = worldToChunkPos(cameraPos);

    // First call: synchronously load a small area so the first frame isn't empty
    if (lastCameraChunk.x == INT_MAX) {
        lastCameraChunk = cameraChunk;

        constexpr int INITIAL_RADIUS = 4;
        for (int x = -INITIAL_RADIUS; x <= INITIAL_RADIUS; x++) {
            for (int z = -INITIAL_RADIUS; z <= INITIAL_RADIUS; z++) {
                glm::ivec3 pos(cameraChunk.x + x, 0, cameraChunk.z + z);
                Chunk* chunk = new Chunk(pos);
                chunk->fillTerrain(noise);
                chunks[pos] = ChunkEntry{ chunk, ChunkState::TerrainReady };
            }
        }
        for (auto& [pos, entry] : chunks) {
            entry.chunk->buildMesh(this);
            entry.state = ChunkState::MeshReady;
        }

        rebuildLoadQueue(cameraChunk);
        return;
    }

    if (cameraChunk != lastCameraChunk) {
        lastCameraChunk = cameraChunk;
        rebuildLoadQueue(cameraChunk);
    }

    processLoadQueue();
    processMeshRebuilds();
    unloadDistantChunks(cameraChunk);
}

void World::rebuildLoadQueue(const glm::ivec3& cameraChunk) {
    loadQueue.clear();

    for (int x = -LOAD_RADIUS; x <= LOAD_RADIUS; x++) {
        for (int z = -LOAD_RADIUS; z <= LOAD_RADIUS; z++) {
            if (x * x + z * z > LOAD_RADIUS * LOAD_RADIUS)
                continue;

            glm::ivec3 pos(cameraChunk.x + x, 0, cameraChunk.z + z);
            if (chunks.find(pos) == chunks.end())
                loadQueue.push_back(pos);
        }
    }

    std::sort(loadQueue.begin(), loadQueue.end(),
        [&cameraChunk](const glm::ivec3& a, const glm::ivec3& b) {
            int da = (a.x - cameraChunk.x) * (a.x - cameraChunk.x)
                   + (a.z - cameraChunk.z) * (a.z - cameraChunk.z);
            int db = (b.x - cameraChunk.x) * (b.x - cameraChunk.x)
                   + (b.z - cameraChunk.z) * (b.z - cameraChunk.z);
            return da < db;
        }
    );
}

void World::processLoadQueue() {
    int loaded = 0;
    int idx = 0;

    while (idx < (int)loadQueue.size() && loaded < LOAD_PER_FRAME) {
        glm::ivec3 pos = loadQueue[idx++];

        if (chunks.find(pos) != chunks.end())
            continue;

        Chunk* chunk = new Chunk(pos);
        chunk->fillTerrain(noise);
        chunks[pos] = ChunkEntry{ chunk, ChunkState::TerrainReady };

        ensureMesh(pos);

        // Also try to build meshes for neighbors that were waiting on this chunk
        for (const auto& offset : NEIGHBOR_OFFSETS) {
            glm::ivec3 neighborPos = pos + offset;
            auto nit = chunks.find(neighborPos);
            if (nit != chunks.end() && nit->second.state == ChunkState::TerrainReady)
                ensureMesh(neighborPos);
        }

        loaded++;
    }

    if (idx > 0)
        loadQueue.erase(loadQueue.begin(), loadQueue.begin() + idx);
}

void World::ensureMesh(const glm::ivec3& pos) {
    auto it = chunks.find(pos);
    if (it == chunks.end()) return;

    ChunkEntry& entry = it->second;
    if (entry.state == ChunkState::MeshReady) return;

    // Check that all 4 horizontal neighbors have terrain data
    for (const auto& offset : NEIGHBOR_OFFSETS) {
        if (chunks.find(pos + offset) == chunks.end())
            return;  // neighbor missing, defer mesh build
    }

    entry.chunk->buildMesh(this);
    entry.state = ChunkState::MeshReady;

    // Queue neighbor mesh rebuilds so their boundary faces update
    for (const auto& offset : NEIGHBOR_OFFSETS) {
        glm::ivec3 neighborPos = pos + offset;
        auto nit = chunks.find(neighborPos);
        if (nit != chunks.end() && nit->second.state == ChunkState::MeshReady)
            meshRebuildQueue.push_back(neighborPos);
    }
}

void World::processMeshRebuilds() {
    int rebuilt = 0;

    while (!meshRebuildQueue.empty() && rebuilt < REBUILD_PER_FRAME) {
        glm::ivec3 pos = meshRebuildQueue.back();
        meshRebuildQueue.pop_back();

        auto it = chunks.find(pos);
        if (it == chunks.end()) continue;
        if (it->second.state != ChunkState::MeshReady) continue;

        it->second.chunk->buildMesh(this);
        rebuilt++;
    }
}

void World::unloadDistantChunks(const glm::ivec3& cameraChunk) {
    for (auto it = chunks.begin(); it != chunks.end(); ) {
        int dx = it->first.x - cameraChunk.x;
        int dz = it->first.z - cameraChunk.z;

        if (dx * dx + dz * dz > UNLOAD_RADIUS * UNLOAD_RADIUS) {
            delete it->second.chunk;
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

void World::render(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    for (auto& [pos, entry] : chunks) {
        if (entry.state != ChunkState::MeshReady) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
            pos.x * CHUNK_SIZE,
            pos.y * CHUNK_SIZE,
            pos.z * CHUNK_SIZE
        ));

        shader.setMat4("model",      model);
        shader.setMat4("view",       view);
        shader.setMat4("projection", projection);

        entry.chunk->render();
    }
}

Chunk* World::getChunk(glm::ivec3 chunkPos) {
    auto it = chunks.find(chunkPos);
    if (it != chunks.end())
        return it->second.chunk;
    return nullptr;
}

void World::setVoxel(int worldX, int worldY, int worldZ, uint8_t type) {
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkY = (int)floor((float)worldY / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    int localX = worldX - chunkX * CHUNK_SIZE;
    int localY = worldY - chunkY * CHUNK_SIZE;
    int localZ = worldZ - chunkZ * CHUNK_SIZE;

    glm::ivec3 chunkPos(chunkX, chunkY, chunkZ);
    Chunk* chunk = getChunk(chunkPos);
    if (!chunk) return;

    chunk->setVoxel(localX, localY, localZ, type);
    chunk->buildMesh(this);

    // Rebuild neighbor chunk meshes if the voxel is on a chunk boundary
    if (localX == 0)              { Chunk* n = getChunk(chunkPos + glm::ivec3(-1,0,0)); if (n) n->buildMesh(this); }
    if (localX == CHUNK_SIZE - 1) { Chunk* n = getChunk(chunkPos + glm::ivec3( 1,0,0)); if (n) n->buildMesh(this); }
    if (localZ == 0)              { Chunk* n = getChunk(chunkPos + glm::ivec3(0,0,-1)); if (n) n->buildMesh(this); }
    if (localZ == CHUNK_SIZE - 1) { Chunk* n = getChunk(chunkPos + glm::ivec3(0,0, 1)); if (n) n->buildMesh(this); }
    if (localY == 0)              { Chunk* n = getChunk(chunkPos + glm::ivec3(0,-1,0)); if (n) n->buildMesh(this); }
    if (localY == CHUNK_SIZE - 1) { Chunk* n = getChunk(chunkPos + glm::ivec3(0, 1,0)); if (n) n->buildMesh(this); }
}

RaycastHit World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) {
    RaycastHit result;

    // DDA voxel traversal algorithm
    glm::vec3 dir = glm::normalize(direction);

    glm::ivec3 blockPos(
        (int)floor(origin.x),
        (int)floor(origin.y),
        (int)floor(origin.z)
    );

    glm::ivec3 step(
        dir.x >= 0 ? 1 : -1,
        dir.y >= 0 ? 1 : -1,
        dir.z >= 0 ? 1 : -1
    );

    // Distance along the ray to reach the next voxel boundary on each axis
    glm::vec3 tMax(
        dir.x != 0 ? ((dir.x > 0 ? blockPos.x + 1 : blockPos.x) - origin.x) / dir.x : 1e30f,
        dir.y != 0 ? ((dir.y > 0 ? blockPos.y + 1 : blockPos.y) - origin.y) / dir.y : 1e30f,
        dir.z != 0 ? ((dir.z > 0 ? blockPos.z + 1 : blockPos.z) - origin.z) / dir.z : 1e30f
    );

    // How far along the ray we must move to cross one voxel on each axis
    glm::vec3 tDelta(
        dir.x != 0 ? (float)step.x / dir.x : 1e30f,
        dir.y != 0 ? (float)step.y / dir.y : 1e30f,
        dir.z != 0 ? (float)step.z / dir.z : 1e30f
    );

    float distance = 0.0f;

    while (distance < maxDistance) {
        uint8_t voxel = getVoxel(blockPos.x, blockPos.y, blockPos.z);
        if (voxel != 0) {
            result.hit = true;
            result.blockPos = blockPos;
            return result;
        }

        // Step to the next voxel boundary
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            distance = tMax.x;
            blockPos.x += step.x;
            tMax.x += tDelta.x;
            result.normal = glm::ivec3(-step.x, 0, 0);
        } else if (tMax.y < tMax.z) {
            distance = tMax.y;
            blockPos.y += step.y;
            tMax.y += tDelta.y;
            result.normal = glm::ivec3(0, -step.y, 0);
        } else {
            distance = tMax.z;
            blockPos.z += step.z;
            tMax.z += tDelta.z;
            result.normal = glm::ivec3(0, 0, -step.z);
        }
    }

    return result;
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
