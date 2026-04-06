#pragma once

#include "TerrainChunk.h"

// ============================================================================
// TerrainWorld — chunk-agnostic facade over a 2D grid of TerrainChunks.
// All consumer code (GetCell, GetTerrainType, IsPassable) calls this class;
// it resolves the chunk internally.  Landscape owns a single TerrainWorld*.
// ============================================================================

class TerrainWorld
{
public:
    TerrainWorld();
    ~TerrainWorld();

    // --- Generation ---
    // _heightData: flat float array from SurfaceMap2D<float>, row-major.
    // _heightWidth/_heightHeight: dimensions of the heightmap grid.
    // The world is divided into ceil(cellsX/CHUNK_SIZE) × ceil(cellsZ/CHUNK_SIZE) chunks.
    // If _seed < 0 (legacy map), all cells default to Earth with zero pheromones.
    void Generate(int _worldCellsX, int _worldCellsZ, int _seed,
                  const float* _heightData, int _heightWidth, int _heightHeight);
    void Clear();

    // --- High-level accessors (world cell coordinates) ---
    int                GetWidth()  const { return m_worldCellsX; }
    int                GetHeight() const { return m_worldCellsZ; }
    TerrainCell&       GetCell(int _x, int _z);
    const TerrainCell& GetCell(int _x, int _z) const;
    TerrainType        GetTerrainType(int _x, int _z) const;
    bool               IsPassable(int _x, int _z) const;

    // --- Chunk access ---
    int                  GetChunksX() const { return m_chunksX; }
    int                  GetChunksZ() const { return m_chunksZ; }
    TerrainChunk*        GetChunk(int _cx, int _cz);
    const TerrainChunk*  GetChunk(int _cx, int _cz) const;

    // --- Active chunk management ---
    int            GetActiveChunkCount() const { return m_activeChunkCount; }
    TerrainChunk*  GetActiveChunk(int _index);

    // --- CA tick (server-only, OMP parallel over active chunks) ---
    void TickActiveChunks(float _alpha, float _beta, float _maxPh);

    // --- Halo exchange (call before TickActiveChunks) ---
    void ExchangeHalos();

    // --- Active list maintenance ---
    void RebuildActiveList();

    // --- Persistence ---
    void SaveSnapshot(const char* _filename) const;
    void LoadSnapshot(const char* _filename);

private:
    int m_worldCellsX, m_worldCellsZ;
    int m_chunksX, m_chunksZ;

    TerrainChunk** m_chunks;          // flat [m_chunksZ * m_chunksX], nullable
    TerrainChunk** m_activeChunks;    // pre-allocated pointer array
    int            m_activeChunkCount;

    // Out-of-bounds sentinel cell for edge cases.
    static TerrainCell s_outsideCell;

    // Helper: get neighbor chunk (nullptr if out of bounds).
    TerrainChunk* GetNeighborN(const TerrainChunk* _chunk) const;
    TerrainChunk* GetNeighborS(const TerrainChunk* _chunk) const;
    TerrainChunk* GetNeighborE(const TerrainChunk* _chunk) const;
    TerrainChunk* GetNeighborW(const TerrainChunk* _chunk) const;
};
