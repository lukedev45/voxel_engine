#include "Chunk.h"
#include "World.h"
#include "BlockData.h"
#include <cstring>

static const int FACE_NORMALS[6][3] = {
    { 0,  0,  1},
    { 0,  0, -1},
    {-1,  0,  0},
    { 1,  0,  0},
    { 0, -1,  0},
    { 0,  1,  0},
};

static const float FACE_VERTICES[6][12] = {
    {0,0,1, 1,0,1, 1,1,1, 0,1,1},
    {1,0,0, 0,0,0, 0,1,0, 1,1,0},
    {0,0,0, 0,0,1, 0,1,1, 0,1,0},
    {1,0,1, 1,0,0, 1,1,0, 1,1,1},
    {0,0,0, 1,0,0, 1,0,1, 0,0,1},
    {0,1,1, 1,1,1, 1,1,0, 0,1,0},
};

Chunk::Chunk(glm::ivec3 position)
    : position(position), VAO(0), VBO(0), vertexCount(0)
{
    // Start with all air — fillTerrain() will populate voxels
    memset(voxels, 0, sizeof(voxels));

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Chunk::fillTerrain(FastNoiseLite& noise) {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Convert local x/z to world x/z for noise sampling
            int worldX = position.x * CHUNK_SIZE + x;
            int worldZ = position.z * CHUNK_SIZE + z;

            // Sample noise — returns value between -1.0 and 1.0
            float noiseVal = noise.GetNoise((float)worldX, (float)worldZ);

            // Map noise to a terrain height (between 4 and 28)
            int height = (int)(noiseVal * 12.0f + 16.0f);

            for (int y = 0; y < CHUNK_SIZE; y++) {
                if (y < height - 3)
                    setVoxel(x, y, z, 1); // stone
                else if (y < height - 1)
                    setVoxel(x, y, z, 2); // dirt
                else if (y < height)
                    setVoxel(x, y, z, 3); // grass
                // else air — already 0 from memset
            }
        }
    }
}

int Chunk::indexOf(int x, int y, int z) const {
    return x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE &&
           y >= 0 && y < CHUNK_SIZE &&
           z >= 0 && z < CHUNK_SIZE;
}

uint8_t Chunk::getVoxel(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) return 0;
    return voxels[indexOf(x, y, z)];
}

void Chunk::setVoxel(int x, int y, int z, uint8_t type) {
    if (!isInBounds(x, y, z)) return;
    voxels[indexOf(x, y, z)] = type;
}

// UV corners for each quad vertex — V is flipped so tiles render right-side-up
// without needing stbi_set_flip_vertically_on_load
static const float FACE_UVS[4][2] = {
    {0.0f, 1.0f},  // v0 -> bottom-left of tile (image bottom)
    {1.0f, 1.0f},  // v1 -> bottom-right
    {1.0f, 0.0f},  // v2 -> top-right (image top)
    {0.0f, 0.0f},  // v3 -> top-left
};

void Chunk::addFace(std::vector<float>& vertices, float x, float y, float z, int face, int tileIndex) {
    static const int QUAD_INDICES[6] = {0, 1, 2, 2, 3, 0};

    float tileU = (tileIndex % ATLAS_COLS) * TILE_UV;
    float tileV = (tileIndex / ATLAS_COLS) * TILE_UV;

    for (int i = 0; i < 6; i++) {
        int corner = QUAD_INDICES[i];
        int vi = corner * 3;

        // Position
        vertices.push_back(x + FACE_VERTICES[face][vi + 0]);
        vertices.push_back(y + FACE_VERTICES[face][vi + 1]);
        vertices.push_back(z + FACE_VERTICES[face][vi + 2]);

        // UV
        vertices.push_back(tileU + FACE_UVS[corner][0] * TILE_UV);
        vertices.push_back(tileV + FACE_UVS[corner][1] * TILE_UV);

        // Face index
        vertices.push_back((float)face);
    }
}

void Chunk::buildMesh(World* world) {
    std::vector<float> vertices;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                uint8_t type = getVoxel(x, y, z);
                if (type == 0) continue;
                for (int face = 0; face < 6; face++) {
                    int nx = x + FACE_NORMALS[face][0];
                    int ny = y + FACE_NORMALS[face][1];
                    int nz = z + FACE_NORMALS[face][2];
                    uint8_t neighbour = 0;
                    if (isInBounds(nx, ny, nz)) {
                        neighbour = getVoxel(nx, ny, nz);
                    } else if (world) {
                        int worldX = position.x * CHUNK_SIZE + nx;
                        int worldY = position.y * CHUNK_SIZE + ny;
                        int worldZ = position.z * CHUNK_SIZE + nz;
                        neighbour = world->getVoxel(worldX, worldY, worldZ);
                    }
                    if (neighbour == 0) {
                        int tileIndex = getTileIndex(type, face);
                        addFace(vertices, (float)x, (float)y, (float)z, face, tileIndex);
                    }
                }
            }
        }
    }

    vertexCount = vertices.size() / 6;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Position (location 0): vec3, stride 24, offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoord (location 1): vec2, stride 24, offset 12
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Face (location 2): float, stride 24, offset 20
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Chunk::render() const {
    if (vertexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}
