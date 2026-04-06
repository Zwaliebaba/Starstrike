#pragma once

// ============================================================================
// TerrainType — biome classification for each terrain cell.
// Static after generation; never changes at runtime.
// ============================================================================

enum class TerrainType : unsigned char
{
    Empty  = 0,
    Earth  = 1,
    Desert = 2,
    Ice    = 3,
    Rock   = 4,
    Water  = 5,
    Count
};

// ============================================================================
// TerrainCell — per-cell terrain data.
// sizeof == 16 (cache-line friendly: 4 cells per 64-byte line).
// m_type is static after generation.  m_foodAmount is server-authoritative
// (never transmitted to clients).  Pheromone channels are mutable.
// ============================================================================

static constexpr int   MAX_FOOD = 100;
static constexpr float MAX_PHEROMONE = 100.0f;

struct TerrainCell
{
    TerrainType   m_type;         // 1 byte — static after generation
    unsigned char _pad[3];        // alignment
    int           m_foodAmount;   // mutable (server-only)
    float         m_phHome;       // mutable — home pheromone
    float         m_phFood;       // mutable — food pheromone
};

static_assert(sizeof(TerrainCell) == 16, "TerrainCell must be 16 bytes for cache efficiency");
