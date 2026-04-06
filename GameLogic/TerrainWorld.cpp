#include "pch.h"
#include "TerrainWorld.h"
#include <cstring>
#include <cstdio>


// ============================================================================
// Static sentinel
// ============================================================================

TerrainCell TerrainWorld::s_outsideCell = { TerrainType::Empty, {0,0,0}, 0, 0.0f, 0.0f };


// ============================================================================
// Construction / Destruction
// ============================================================================

TerrainWorld::TerrainWorld()
    : m_worldCellsX(0),
      m_worldCellsZ(0),
      m_chunksX(0),
      m_chunksZ(0),
      m_chunks(nullptr),
      m_activeChunks(nullptr),
      m_activeChunkCount(0)
{
}

TerrainWorld::~TerrainWorld()
{
    Clear();
}


// ============================================================================
// Generate — allocate chunks and run deterministic biome classification
// ============================================================================

void TerrainWorld::Generate(int _worldCellsX, int _worldCellsZ, int _seed,
                            const float* _heightData, int _heightWidth, int _heightHeight)
{
    Clear();

    m_worldCellsX = _worldCellsX;
    m_worldCellsZ = _worldCellsZ;

    m_chunksX = (_worldCellsX + TerrainChunk::CHUNK_SIZE - 1) / TerrainChunk::CHUNK_SIZE;
    m_chunksZ = (_worldCellsZ + TerrainChunk::CHUNK_SIZE - 1) / TerrainChunk::CHUNK_SIZE;

    const int totalChunks = m_chunksX * m_chunksZ;
    m_chunks = new TerrainChunk*[totalChunks];
    std::memset(m_chunks, 0, sizeof(TerrainChunk*) * totalChunks);

    // Pre-allocate active chunk pointer array (worst case: all chunks active).
    m_activeChunks = new TerrainChunk*[totalChunks];
    m_activeChunkCount = 0;

    // Generate all chunks.
    for (int cz = 0; cz < m_chunksZ; ++cz)
    {
        for (int cx = 0; cx < m_chunksX; ++cx)
        {
            auto* chunk = new TerrainChunk(cx, cz);
            chunk->Generate(_seed, _heightData, _heightWidth, _worldCellsX, _worldCellsZ);
            chunk->m_active = true; // Initially all active
            m_chunks[cz * m_chunksX + cx] = chunk;
        }
    }

    RebuildActiveList();
}

void TerrainWorld::Clear()
{
    if (m_chunks)
    {
        const int totalChunks = m_chunksX * m_chunksZ;
        for (int i = 0; i < totalChunks; ++i)
            delete m_chunks[i];
        delete[] m_chunks;
        m_chunks = nullptr;
    }

    delete[] m_activeChunks;
    m_activeChunks = nullptr;
    m_activeChunkCount = 0;

    m_chunksX = 0;
    m_chunksZ = 0;
    m_worldCellsX = 0;
    m_worldCellsZ = 0;
}


// ============================================================================
// Cell access — chunk-agnostic world coordinates
// ============================================================================

TerrainCell& TerrainWorld::GetCell(int _x, int _z)
{
    if (_x < 0 || _x >= m_worldCellsX || _z < 0 || _z >= m_worldCellsZ)
        return s_outsideCell;

    int cx = _x / TerrainChunk::CHUNK_SIZE;
    int cz = _z / TerrainChunk::CHUNK_SIZE;
    TerrainChunk* chunk = m_chunks[cz * m_chunksX + cx];
    if (!chunk)
        return s_outsideCell;

    int lx = _x % TerrainChunk::CHUNK_SIZE;
    int lz = _z % TerrainChunk::CHUNK_SIZE;
    return chunk->GetCell(lx, lz);
}

const TerrainCell& TerrainWorld::GetCell(int _x, int _z) const
{
    if (_x < 0 || _x >= m_worldCellsX || _z < 0 || _z >= m_worldCellsZ)
        return s_outsideCell;

    int cx = _x / TerrainChunk::CHUNK_SIZE;
    int cz = _z / TerrainChunk::CHUNK_SIZE;
    const TerrainChunk* chunk = m_chunks[cz * m_chunksX + cx];
    if (!chunk)
        return s_outsideCell;

    int lx = _x % TerrainChunk::CHUNK_SIZE;
    int lz = _z % TerrainChunk::CHUNK_SIZE;
    return chunk->GetCell(lx, lz);
}

TerrainType TerrainWorld::GetTerrainType(int _x, int _z) const
{
    return GetCell(_x, _z).m_type;
}

bool TerrainWorld::IsPassable(int _x, int _z) const
{
    TerrainType t = GetTerrainType(_x, _z);
    return t != TerrainType::Water && t != TerrainType::Rock;
}


// ============================================================================
// Chunk access
// ============================================================================

TerrainChunk* TerrainWorld::GetChunk(int _cx, int _cz)
{
    if (_cx < 0 || _cx >= m_chunksX || _cz < 0 || _cz >= m_chunksZ)
        return nullptr;
    return m_chunks[_cz * m_chunksX + _cx];
}

const TerrainChunk* TerrainWorld::GetChunk(int _cx, int _cz) const
{
    if (_cx < 0 || _cx >= m_chunksX || _cz < 0 || _cz >= m_chunksZ)
        return nullptr;
    return m_chunks[_cz * m_chunksX + _cx];
}

TerrainChunk* TerrainWorld::GetActiveChunk(int _index)
{
    DEBUG_ASSERT(_index >= 0 && _index < m_activeChunkCount);
    return m_activeChunks[_index];
}


// ============================================================================
// Active list
// ============================================================================

void TerrainWorld::RebuildActiveList()
{
    m_activeChunkCount = 0;
    const int totalChunks = m_chunksX * m_chunksZ;
    for (int i = 0; i < totalChunks; ++i)
    {
        if (m_chunks[i] && m_chunks[i]->m_active)
            m_activeChunks[m_activeChunkCount++] = m_chunks[i];
    }
}


// ============================================================================
// Neighbor lookup helpers
// ============================================================================

TerrainChunk* TerrainWorld::GetNeighborN(const TerrainChunk* _chunk) const
{
    int cx = _chunk->GetChunkX();
    int cz = _chunk->GetChunkZ() - 1;
    if (cz < 0) return nullptr;
    return m_chunks[cz * m_chunksX + cx];
}

TerrainChunk* TerrainWorld::GetNeighborS(const TerrainChunk* _chunk) const
{
    int cx = _chunk->GetChunkX();
    int cz = _chunk->GetChunkZ() + 1;
    if (cz >= m_chunksZ) return nullptr;
    return m_chunks[cz * m_chunksX + cx];
}

TerrainChunk* TerrainWorld::GetNeighborE(const TerrainChunk* _chunk) const
{
    int cx = _chunk->GetChunkX() + 1;
    int cz = _chunk->GetChunkZ();
    if (cx >= m_chunksX) return nullptr;
    return m_chunks[cz * m_chunksX + cx];
}

TerrainChunk* TerrainWorld::GetNeighborW(const TerrainChunk* _chunk) const
{
    int cx = _chunk->GetChunkX() - 1;
    int cz = _chunk->GetChunkZ();
    if (cx < 0) return nullptr;
    return m_chunks[cz * m_chunksX + cx];
}


// ============================================================================
// Halo exchange
// ============================================================================

void TerrainWorld::ExchangeHalos()
{
    for (int i = 0; i < m_activeChunkCount; ++i)
    {
        TerrainChunk* chunk = m_activeChunks[i];
        chunk->CopyHaloFrom(GetNeighborN(chunk), 0);
        chunk->CopyHaloFrom(GetNeighborS(chunk), 1);
        chunk->CopyHaloFrom(GetNeighborE(chunk), 2);
        chunk->CopyHaloFrom(GetNeighborW(chunk), 3);
    }
}


// ============================================================================
// Delta sync support
// ============================================================================

void TerrainWorld::SnapshotAllDirtyChunks()
{
    for (int i = 0; i < m_activeChunkCount; ++i)
    {
        TerrainChunk* chunk = m_activeChunks[i];
        if (chunk->m_dirty)
            chunk->SnapshotPheromones();
    }
}


// ============================================================================
// CA tick — OMP parallel over active chunks
// ============================================================================

void TerrainWorld::TickActiveChunks(float _alpha, float _beta, float _maxPh)
{
    // Halo exchange must happen before parallel tick so each chunk
    // has consistent neighbor data.
    ExchangeHalos();

    int numActive = m_activeChunkCount;

#undef for
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < numActive; ++i)
    {
        TerrainChunk* chunk = m_activeChunks[i];
        chunk->TickPheromones(_alpha, _beta, _maxPh,
            GetNeighborN(chunk), GetNeighborS(chunk),
            GetNeighborE(chunk), GetNeighborW(chunk));
    }
}


// ============================================================================
// Persistence — binary snapshot of pheromone state
// ============================================================================

void TerrainWorld::SaveSnapshot(const char* _filename) const
{
    FILE* f = nullptr;
    fopen_s(&f, _filename, "wb");
    if (!f) return;

    // Header: chunksX, chunksZ
    fwrite(&m_chunksX, sizeof(int), 1, f);
    fwrite(&m_chunksZ, sizeof(int), 1, f);

    const int totalChunks = m_chunksX * m_chunksZ;
    for (int i = 0; i < totalChunks; ++i)
    {
        const TerrainChunk* chunk = m_chunks[i];
        if (!chunk)
            continue;

        // Write chunk header.
        int cx = chunk->GetChunkX();
        int cz = chunk->GetChunkZ();
        fwrite(&cx, sizeof(int), 1, f);
        fwrite(&cz, sizeof(int), 1, f);

        // Write pheromone data.
        char buf[TerrainChunk::CELL_COUNT * 2 * sizeof(float)];
        int bytes = chunk->SerializePheromones(buf, sizeof(buf));
        fwrite(buf, 1, bytes, f);
    }

    fclose(f);
}

void TerrainWorld::LoadSnapshot(const char* _filename)
{
    FILE* f = nullptr;
    fopen_s(&f, _filename, "rb");
    if (!f) return;

    int chunksX = 0, chunksZ = 0;
    fread(&chunksX, sizeof(int), 1, f);
    fread(&chunksZ, sizeof(int), 1, f);

    // Validate dimensions match.
    if (chunksX != m_chunksX || chunksZ != m_chunksZ)
    {
        fclose(f);
        return;
    }

    while (!feof(f))
    {
        int cx = 0, cz = 0;
        if (fread(&cx, sizeof(int), 1, f) != 1) break;
        if (fread(&cz, sizeof(int), 1, f) != 1) break;

        char buf[TerrainChunk::CELL_COUNT * 2 * sizeof(float)];
        int bytes = TerrainChunk::CELL_COUNT * 2 * static_cast<int>(sizeof(float));
        if (static_cast<int>(fread(buf, 1, bytes, f)) != bytes) break;

        TerrainChunk* chunk = GetChunk(cx, cz);
        if (chunk)
            chunk->DeserializePheromones(buf, bytes);
    }

    fclose(f);
}
