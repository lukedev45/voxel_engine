#pragma once
#include <cstdint>

constexpr int TILE_STONE      = 0;
constexpr int TILE_DIRT        = 1;
constexpr int TILE_GRASS_TOP   = 2;
constexpr int TILE_GRASS_SIDE  = 3;

constexpr int ATLAS_COLS = 16;
constexpr float TILE_UV  = 1.0f / ATLAS_COLS;

// Faces: 0=front, 1=back, 2=left, 3=right, 4=bottom, 5=top
inline int getTileIndex(uint8_t blockType, int face) {
    switch (blockType) {
        case 1: return TILE_STONE;
        case 2: return TILE_DIRT;
        case 3: // Grass
            if (face == 5) return TILE_GRASS_TOP;
            if (face == 4) return TILE_DIRT;
            return TILE_GRASS_SIDE;
        default: return TILE_STONE;
    }
}
