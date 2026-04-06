#pragma once

#include "TerrainCell.h"

// ============================================================================
// TerrainChunk — 128×128 block of TerrainCells.
// The atomic unit for generation, CA ticking, delta sync, and persistence.
// Consumers use TerrainWorld (chunk-agnostic facade) and never address
// chunks directly.
// ============================================================================

class TerrainChunk
{
public:
    static constexpr int CHUNK_SIZE = 128; // cells per side
    static constexpr int CELL_COUNT = CHUNK_SIZE * CHUNK_SIZE;

    TerrainChunk(int _chunkX, int _chunkZ);
    ~TerrainChunk();

    int GetChunkX() const { return m_chunkX; }
    int GetChunkZ() const { return m_chunkZ; }

    // --- Cell access (local coordinates 0..CHUNK_SIZE-1) ---
    TerrainCell&       GetCell(int _localX, int _localZ);
    const TerrainCell& GetCell(int _localX, int _localZ) const;
    TerrainType        GetTerrainType(int _localX, int _localZ) const;
    bool               IsPassable(int _localX, int _localZ) const;

    // --- Generation ---
    // _heightData points to the *full* heightmap (row-major, stride = _heightStride).
    // This chunk reads the region starting at (m_chunkX * CHUNK_SIZE, m_chunkZ * CHUNK_SIZE).
    // Biome classification uses integer-quantised noise thresholds for cross-platform
    // determinism.  If _seed < 0 (legacy map), all cells are set to Earth with zero
    // pheromone and zero food.
    void Generate(int _seed, const float* _heightData, int _heightStride,
                  int _worldCellsX, int _worldCellsZ);

    // --- Pheromone CA (server-only) ---
    // Neighbours may be nullptr (world edge) — uses reflective boundary.
    void TickPheromones(float _alpha, float _beta, float _maxPh,
                        const TerrainChunk* _neighborN,
                        const TerrainChunk* _neighborS,
                        const TerrainChunk* _neighborE,
                        const TerrainChunk* _neighborW);

    // --- Delta sync ---
    struct PhDelta
    {
        int   m_cellIndex; // flat index within this chunk
        float m_phHome;
        float m_phFood;
    };

    int  BuildDelta(PhDelta* _outBuf, int _maxDeltas, float _threshold) const;
    void ApplyDelta(const PhDelta* _deltas, int _count);
    void SnapshotPheromones(); // copy current → last-sent baseline

    // --- Full sync ---
    int  SerializePheromones(char* _outBuf, int _maxBytes) const;
    void DeserializePheromones(const char* _buf, int _bytes);

    bool m_active;   // ticked when at least one client is subscribed or agents present
    bool m_dirty;    // pheromone changed since last delta build

    // --- Halo exchange ---
    // Side indices: 0 = North (−Z), 1 = South (+Z), 2 = East (+X), 3 = West (−X)
    void CopyHaloFrom(const TerrainChunk* _neighbor, int _side);

    // Read a neighbor's pheromone value during the CA tick.
    // If the coordinate falls outside the chunk, reads from the halo buffer.
    // If halo is empty (nullptr neighbor = world edge), uses reflective boundary.
    float ReadNeighborHome(int _localX, int _localZ) const;
    float ReadNeighborFood(int _localX, int _localZ) const;

private:
    int m_chunkX, m_chunkZ;

    TerrainCell m_cells[CELL_COUNT];

    // Snapshot of pheromone state at last delta build (for diff detection).
    float m_phHomeSnapshot[CELL_COUNT];
    float m_phFoodSnapshot[CELL_COUNT];

    // Double-buffer scratch for diffusion (avoids read-after-write hazard).
    float m_phHomeScratch[CELL_COUNT];
    float m_phFoodScratch[CELL_COUNT];

    // 1-cell ghost halo buffers for cross-chunk diffusion.
    // Interleaved [home, food] per cell along the edge (CHUNK_SIZE pairs).
    // Side indices: 0=N, 1=S, 2=E, 3=W
    float m_haloN[CHUNK_SIZE * 2];
    float m_haloS[CHUNK_SIZE * 2];
    float m_haloE[CHUNK_SIZE * 2];
    float m_haloW[CHUNK_SIZE * 2];

    void GenerateBiomes(int _seed, const float* _heightData, int _heightStride,
                        int _worldCellsX, int _worldCellsZ);
    void GenerateFood(int _seed, int _worldCellsX, int _worldCellsZ);
};
