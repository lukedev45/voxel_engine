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

// Tangent axes for each face (two axes perpendicular to the normal)
static const int FACE_TANGENT1[6][3] = {
    {1,0,0}, {1,0,0}, {0,0,1}, {0,0,1}, {1,0,0}, {1,0,0}
};
static const int FACE_TANGENT2[6][3] = {
    {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {0,0,1}, {0,0,1}
};

// Per-face, per-corner signs along tangent1 and tangent2
// Derived from where each vertex sits relative to the face center
static const int CORNER_SIGNS[6][4][2] = {
    {{-1,-1}, { 1,-1}, { 1, 1}, {-1, 1}}, // face 0 (front +Z)
    {{ 1,-1}, {-1,-1}, {-1, 1}, { 1, 1}}, // face 1 (back -Z)
    {{-1,-1}, { 1,-1}, { 1, 1}, {-1, 1}}, // face 2 (left -X)
    {{ 1,-1}, {-1,-1}, {-1, 1}, { 1, 1}}, // face 3 (right +X)
    {{-1,-1}, { 1,-1}, { 1, 1}, {-1, 1}}, // face 4 (bottom -Y)
    {{-1, 1}, { 1, 1}, { 1,-1}, {-1,-1}}, // face 5 (top +Y)
};

Chunk::Chunk(glm::ivec3 position)
    : position(position), VAO(0), VBO(0), vertexCount(0)
{
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
            int worldX = position.x * CHUNK_SIZE + x;
            int worldZ = position.z * CHUNK_SIZE + z;

            float noiseVal = noise.GetNoise((float)worldX, (float)worldZ);
            int height = (int)(noiseVal * 12.0f + 16.0f);

            for (int y = 0; y < CHUNK_SIZE; y++) {
                if (y < height - 3)
                    setVoxel(x, y, z, 1); // stone
                else if (y < height - 1)
                    setVoxel(x, y, z, 2); // dirt
                else if (y < height)
                    setVoxel(x, y, z, 3); // grass
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

// Check if a voxel at local coords is solid, with cross-chunk support
static bool isSolid(const Chunk* chunk, World* world, int lx, int ly, int lz) {
    if (chunk->isInBounds(lx, ly, lz))
        return chunk->getVoxel(lx, ly, lz) != 0;
    if (world) {
        int wx = chunk->position.x * CHUNK_SIZE + lx;
        int wy = chunk->position.y * CHUNK_SIZE + ly;
        int wz = chunk->position.z * CHUNK_SIZE + lz;
        return world->getVoxel(wx, wy, wz) != 0;
    }
    return false;
}

// Compute AO for one corner of a face
// Returns 0 (fully occluded) to 3 (no occlusion)
static int computeCornerAO(const Chunk* chunk, World* world,
                           int x, int y, int z, int face, int corner) {
    int nx = FACE_NORMALS[face][0], ny = FACE_NORMALS[face][1], nz = FACE_NORMALS[face][2];
    int t1x = FACE_TANGENT1[face][0], t1y = FACE_TANGENT1[face][1], t1z = FACE_TANGENT1[face][2];
    int t2x = FACE_TANGENT2[face][0], t2y = FACE_TANGENT2[face][1], t2z = FACE_TANGENT2[face][2];
    int d1 = CORNER_SIGNS[face][corner][0];
    int d2 = CORNER_SIGNS[face][corner][1];

    bool side1 = isSolid(chunk, world,
        x + nx + d1*t1x, y + ny + d1*t1y, z + nz + d1*t1z);
    bool side2 = isSolid(chunk, world,
        x + nx + d2*t2x, y + ny + d2*t2y, z + nz + d2*t2z);
    bool cornerBlock = isSolid(chunk, world,
        x + nx + d1*t1x + d2*t2x, y + ny + d1*t1y + d2*t2y, z + nz + d1*t1z + d2*t2z);

    if (side1 && side2)
        return 0;
    return 3 - (side1 + side2 + cornerBlock);
}

// UV corners for each quad vertex
static const float FACE_UVS[4][2] = {
    {0.0f, 1.0f},
    {1.0f, 1.0f},
    {1.0f, 0.0f},
    {0.0f, 0.0f},
};

void Chunk::addFace(std::vector<float>& vertices, float x, float y, float z,
                    int face, int tileIndex, const int ao[4]) {
    float tileU = (tileIndex % ATLAS_COLS) * TILE_UV;
    float tileV = (tileIndex / ATLAS_COLS) * TILE_UV;

    // Flip quad triangulation when AO creates anisotropy to avoid ugly diagonal seam
    // Default: {0,1,2, 2,3,0}, Flipped: {1,2,3, 3,0,1}
    bool flip = (ao[0] + ao[2]) < (ao[1] + ao[3]);

    static const int QUAD_DEFAULT[6] = {0, 1, 2, 2, 3, 0};
    static const int QUAD_FLIPPED[6] = {1, 2, 3, 3, 0, 1};
    const int* indices = flip ? QUAD_FLIPPED : QUAD_DEFAULT;

    for (int i = 0; i < 6; i++) {
        int corner = indices[i];
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

        // AO value (0-3, normalized to 0.0-1.0 in shader)
        vertices.push_back((float)ao[corner]);
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

                        // Compute per-vertex AO for the 4 corners
                        int ao[4];
                        for (int c = 0; c < 4; c++)
                            ao[c] = computeCornerAO(this, world, x, y, z, face, c);

                        addFace(vertices, (float)x, (float)y, (float)z, face, tileIndex, ao);
                    }
                }
            }
        }
    }

    vertexCount = vertices.size() / 7; // 7 floats per vertex now

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    int stride = 7 * sizeof(float);
    // Position (location 0): vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoord (location 1): vec2
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Face (location 2): float
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // AO (location 3): float
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void Chunk::render() const {
    if (vertexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}
