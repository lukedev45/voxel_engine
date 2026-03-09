#include "Chunk.h"
#include "World.h"
#include <cstring>

static const int FACE_NORMALS[6][3] = {
    { 0,  0,  1},  // front
    { 0,  0, -1},  // back
    {-1,  0,  0},  // left
    { 1,  0,  0},  // right
    { 0, -1,  0},  // bottom
    { 0,  1,  0},  // top
};

static const float FACE_VERTICES[6][12] = {
    {0,0,1, 1,0,1, 1,1,1, 0,1,1}, // front
    {1,0,0, 0,0,0, 0,1,0, 1,1,0}, // back
    {0,0,0, 0,0,1, 0,1,1, 0,1,0}, // left
    {1,0,1, 1,0,0, 1,1,0, 1,1,1}, // right
    {0,0,0, 1,0,0, 1,0,1, 0,0,1}, // bottom
    {0,1,1, 1,1,1, 1,1,0, 0,1,0}, // top
};

Chunk::Chunk(glm::ivec3 position) : position(position), VAO(0), VBO(0), vertexCount(0) {
    memset(voxels, 0, sizeof(voxels));

    // Fill bottom half with stone
    for (int x = 0; x < CHUNK_SIZE; x++)
        for (int z = 0; z < CHUNK_SIZE; z++)
            for (int y = 0; y < CHUNK_SIZE / 2; y++)
                setVoxel(x, y, z, 1);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
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

void Chunk::addFace(std::vector<float>& vertices, float x, float y, float z, int face) {
    static const int QUAD_INDICES[6] = {0, 1, 2, 2, 3, 0};
    for (int i = 0; i < 6; i++) {
        int vi = QUAD_INDICES[i] * 3;
        vertices.push_back(x + FACE_VERTICES[face][vi + 0]);
        vertices.push_back(y + FACE_VERTICES[face][vi + 1]);
        vertices.push_back(z + FACE_VERTICES[face][vi + 2]);
    }
}

void Chunk::buildMesh(World* world) {
    std::vector<float> vertices;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (getVoxel(x, y, z) == 0) continue;

                for (int face = 0; face < 6; face++) {
                    int nx = x + FACE_NORMALS[face][0];
                    int ny = y + FACE_NORMALS[face][1];
                    int nz = z + FACE_NORMALS[face][2];

                    uint8_t neighbour = 0;

                    if (isInBounds(nx, ny, nz)) {
                        // Neighbour is inside this chunk
                        neighbour = getVoxel(nx, ny, nz);
                    } else if (world) {
                        // Neighbour is in an adjacent chunk — ask the world
                        int worldX = position.x * CHUNK_SIZE + nx;
                        int worldY = position.y * CHUNK_SIZE + ny;
                        int worldZ = position.z * CHUNK_SIZE + nz;
                        neighbour = world->getVoxel(worldX, worldY, worldZ);
                    }

                    if (neighbour == 0)
                        addFace(vertices, (float)x, (float)y, (float)z, face);
                }
            }
        }
    }

    vertexCount = vertices.size() / 3;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Chunk::render() const {
    if (vertexCount == 0) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}
