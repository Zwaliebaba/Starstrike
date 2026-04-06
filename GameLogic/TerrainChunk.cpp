#include "pch.h"
#include "TerrainChunk.h"
#include "PerlinNoise.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Windows.h min/max macros break std::clamp — use a local helper.
namespace
{
    template <typename T>
    constexpr T Clamp(T _val, T _lo, T _hi)
    {
        return _val < _lo ? _lo : (_val > _hi ? _hi : _val);
    }
}


// ============================================================================
// Construction / Destruction
// ============================================================================

TerrainChunk::TerrainChunk(int _chunkX, int _chunkZ)
    : m_chunkX(_chunkX),
      m_chunkZ(_chunkZ),
      m_active(false),
      m_dirty(false)
{
    std::memset(m_cells,          0, sizeof(m_cells));
    std::memset(m_phHomeSnapshot, 0, sizeof(m_phHomeSnapshot));
    std::memset(m_phFoodSnapshot, 0, sizeof(m_phFoodSnapshot));
    std::memset(m_phHomeScratch,  0, sizeof(m_phHomeScratch));
    std::memset(m_phFoodScratch,  0, sizeof(m_phFoodScratch));
    std::memset(m_haloN,          0, sizeof(m_haloN));
    std::memset(m_haloS,          0, sizeof(m_haloS));
    std::memset(m_haloE,          0, sizeof(m_haloE));
    std::memset(m_haloW,          0, sizeof(m_haloW));
}

TerrainChunk::~TerrainChunk() = default;


// ============================================================================
// Cell access
// ============================================================================

TerrainCell& TerrainChunk::GetCell(int _localX, int _localZ)
{
    DEBUG_ASSERT(_localX >= 0 && _localX < CHUNK_SIZE);
    DEBUG_ASSERT(_localZ >= 0 && _localZ < CHUNK_SIZE);
    return m_cells[_localZ * CHUNK_SIZE + _localX];
}

const TerrainCell& TerrainChunk::GetCell(int _localX, int _localZ) const
{
    DEBUG_ASSERT(_localX >= 0 && _localX < CHUNK_SIZE);
    DEBUG_ASSERT(_localZ >= 0 && _localZ < CHUNK_SIZE);
    return m_cells[_localZ * CHUNK_SIZE + _localX];
}

TerrainType TerrainChunk::GetTerrainType(int _localX, int _localZ) const
{
    return GetCell(_localX, _localZ).m_type;
}

bool TerrainChunk::IsPassable(int _localX, int _localZ) const
{
    TerrainType t = GetTerrainType(_localX, _localZ);
    return t != TerrainType::Water && t != TerrainType::Rock;
}


// ============================================================================
// Generation — deterministic biome classification from seed + heightmap
// ============================================================================

void TerrainChunk::Generate(int _seed, const float* _heightData, int _heightStride,
                            int _worldCellsX, int _worldCellsZ)
{
    if (_seed < 0)
    {
        // Legacy map: all Earth, zero food, zero pheromone.
        for (int i = 0; i < CELL_COUNT; ++i)
        {
            m_cells[i].m_type       = TerrainType::Earth;
            m_cells[i]._pad[0]     = 0;
            m_cells[i]._pad[1]     = 0;
            m_cells[i]._pad[2]     = 0;
            m_cells[i].m_foodAmount = 0;
            m_cells[i].m_phHome    = 0.0f;
            m_cells[i].m_phFood    = 0.0f;
        }
        return;
    }

    GenerateBiomes(_seed, _heightData, _heightStride, _worldCellsX, _worldCellsZ);
    GenerateFood(_seed, _worldCellsX, _worldCellsZ);

    // Pheromones start at zero.
    for (int i = 0; i < CELL_COUNT; ++i)
    {
        m_cells[i].m_phHome = 0.0f;
        m_cells[i].m_phFood = 0.0f;
    }
}

void TerrainChunk::GenerateBiomes(int _seed, const float* _heightData, int _heightStride,
                                  int _worldCellsX, int _worldCellsZ)
{
    // Noise generators for biome classification.
    PerlinNoise baseNoise(_seed);
    PerlinNoise moistureNoise(_seed + 1);

    // Noise scale controls biome region size.
    // Lower values = larger biome blobs.
    static constexpr float NOISE_SCALE = 0.01f;

    const int originX = m_chunkX * CHUNK_SIZE;
    const int originZ = m_chunkZ * CHUNK_SIZE;

    for (int lz = 0; lz < CHUNK_SIZE; ++lz)
    {
        const int worldZ = originZ + lz;
        for (int lx = 0; lx < CHUNK_SIZE; ++lx)
        {
            const int worldX = originX + lx;
            TerrainCell& cell = m_cells[lz * CHUNK_SIZE + lx];

            // Clamp to world bounds for height lookup.
            float height = 0.0f;
            if (worldX >= 0 && worldX < _worldCellsX &&
                worldZ >= 0 && worldZ < _worldCellsZ)
            {
                height = _heightData[worldZ * _heightStride + worldX];
            }

            // Integer-quantised noise for deterministic classification (§A.8).
            float baseVal     = baseNoise.FBM(worldX * NOISE_SCALE, worldZ * NOISE_SCALE, 4, 0.5f);
            float moistureVal = moistureNoise.FBM(worldX * NOISE_SCALE, worldZ * NOISE_SCALE, 3, 0.6f);

            // Map from [-1,1] to [0,1] before quantisation.
            int base1000     = static_cast<int>((baseVal     * 0.5f + 0.5f) * 1000.0f + 0.5f);
            int moisture1000 = static_cast<int>((moistureVal * 0.5f + 0.5f) * 1000.0f + 0.5f);

            // Classification hierarchy:
            if (height <= 0.0f)
                cell.m_type = TerrainType::Water;
            else if (base1000 > 700)
                cell.m_type = TerrainType::Rock;
            else if (moisture1000 < 200)
                cell.m_type = TerrainType::Desert;
            else if (moisture1000 > 700 && height > 50.0f)
                cell.m_type = TerrainType::Ice;
            else
                cell.m_type = TerrainType::Earth;

            cell._pad[0] = 0;
            cell._pad[1] = 0;
            cell._pad[2] = 0;
        }
    }
}

void TerrainChunk::GenerateFood(int _seed, int _worldCellsX, int _worldCellsZ)
{
    PerlinNoise foodNoise(_seed + 2);
    static constexpr float FOOD_SCALE = 0.02f;

    const int originX = m_chunkX * CHUNK_SIZE;
    const int originZ = m_chunkZ * CHUNK_SIZE;

    for (int lz = 0; lz < CHUNK_SIZE; ++lz)
    {
        const int worldZ = originZ + lz;
        for (int lx = 0; lx < CHUNK_SIZE; ++lx)
        {
            const int worldX = originX + lx;
            TerrainCell& cell = m_cells[lz * CHUNK_SIZE + lx];

            if (cell.m_type == TerrainType::Earth)
            {
                float foodVal = foodNoise.FBM(worldX * FOOD_SCALE, worldZ * FOOD_SCALE, 2, 0.5f);
                // Map [-1,1] → [0,1] then scale to MAX_FOOD.
                float normalized = foodVal * 0.5f + 0.5f;
                cell.m_foodAmount = static_cast<int>(normalized * MAX_FOOD);
                cell.m_foodAmount = Clamp(cell.m_foodAmount, 0, MAX_FOOD);
            }
            else
            {
                cell.m_foodAmount = 0;
            }
        }
    }
}


// ============================================================================
// Pheromone CA tick — diffusion + evaporation
// ============================================================================

void TerrainChunk::TickPheromones(float _alpha, float _beta, float _maxPh,
                                  const TerrainChunk* _neighborN,
                                  const TerrainChunk* _neighborS,
                                  const TerrainChunk* _neighborE,
                                  const TerrainChunk* _neighborW)
{
    // Halo exchange: copy edge values from neighbours.
    CopyHaloFrom(_neighborN, 0); // North (−Z)
    CopyHaloFrom(_neighborS, 1); // South (+Z)
    CopyHaloFrom(_neighborE, 2); // East  (+X)
    CopyHaloFrom(_neighborW, 3); // West  (−X)

    // Diffusion pass: read from m_cells + halo, write to scratch.
    static constexpr int DX[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static constexpr int DZ[] = { -1,-1,-1,  0, 0,  1, 1, 1 };
    static constexpr float INV_8 = 1.0f / 8.0f;

    for (int z = 0; z < CHUNK_SIZE; ++z)
    {
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            const int idx = z * CHUNK_SIZE + x;
            const float selfHome = m_cells[idx].m_phHome;
            const float selfFood = m_cells[idx].m_phFood;

            float sumHome = 0.0f;
            float sumFood = 0.0f;

            for (int n = 0; n < 8; ++n)
            {
                int nx = x + DX[n];
                int nz = z + DZ[n];
                sumHome += ReadNeighborHome(nx, nz);
                sumFood += ReadNeighborFood(nx, nz);
            }

            // Diffusion: move towards local average.
            m_phHomeScratch[idx] = selfHome + _alpha * (sumHome * INV_8 - selfHome);
            m_phFoodScratch[idx] = selfFood + _alpha * (sumFood * INV_8 - selfFood);
        }
    }

    // Evaporation + write-back pass.
    for (int i = 0; i < CELL_COUNT; ++i)
    {
        m_cells[i].m_phHome = Clamp(m_phHomeScratch[i] * _beta, 0.0f, _maxPh);
        m_cells[i].m_phFood = Clamp(m_phFoodScratch[i] * _beta, 0.0f, _maxPh);
    }

    m_dirty = true;
}


// ============================================================================
// Halo exchange — ghost-cell copy from neighbouring chunks
// ============================================================================

void TerrainChunk::CopyHaloFrom(const TerrainChunk* _neighbor, int _side)
{
    // Side: 0=N(−Z), 1=S(+Z), 2=E(+X), 3=W(−X)
    float* halo = nullptr;
    switch (_side)
    {
    case 0: halo = m_haloN; break;
    case 1: halo = m_haloS; break;
    case 2: halo = m_haloE; break;
    case 3: halo = m_haloW; break;
    }

    if (!_neighbor)
    {
        // World edge: reflective boundary — fill halo with this chunk's
        // own edge values so edge cells see themselves as the neighbor.
        switch (_side)
        {
        case 0: // North: copy row 0 of this chunk
            for (int x = 0; x < CHUNK_SIZE; ++x)
            {
                const auto& c = m_cells[0 * CHUNK_SIZE + x];
                halo[x * 2 + 0] = c.m_phHome;
                halo[x * 2 + 1] = c.m_phFood;
            }
            break;
        case 1: // South: copy last row of this chunk
            for (int x = 0; x < CHUNK_SIZE; ++x)
            {
                const auto& c = m_cells[(CHUNK_SIZE - 1) * CHUNK_SIZE + x];
                halo[x * 2 + 0] = c.m_phHome;
                halo[x * 2 + 1] = c.m_phFood;
            }
            break;
        case 2: // East: copy last column of this chunk
            for (int z = 0; z < CHUNK_SIZE; ++z)
            {
                const auto& c = m_cells[z * CHUNK_SIZE + (CHUNK_SIZE - 1)];
                halo[z * 2 + 0] = c.m_phHome;
                halo[z * 2 + 1] = c.m_phFood;
            }
            break;
        case 3: // West: copy first column of this chunk
            for (int z = 0; z < CHUNK_SIZE; ++z)
            {
                const auto& c = m_cells[z * CHUNK_SIZE + 0];
                halo[z * 2 + 0] = c.m_phHome;
                halo[z * 2 + 1] = c.m_phFood;
            }
            break;
        }
        return;
    }

    // Copy the edge row/column from the neighbor chunk.
    switch (_side)
    {
    case 0: // North halo: neighbor's last row (its southernmost edge)
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            const auto& c = _neighbor->m_cells[(CHUNK_SIZE - 1) * CHUNK_SIZE + x];
            halo[x * 2 + 0] = c.m_phHome;
            halo[x * 2 + 1] = c.m_phFood;
        }
        break;
    case 1: // South halo: neighbor's first row (its northernmost edge)
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            const auto& c = _neighbor->m_cells[0 * CHUNK_SIZE + x];
            halo[x * 2 + 0] = c.m_phHome;
            halo[x * 2 + 1] = c.m_phFood;
        }
        break;
    case 2: // East halo: neighbor's first column (its westernmost edge)
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            const auto& c = _neighbor->m_cells[z * CHUNK_SIZE + 0];
            halo[z * 2 + 0] = c.m_phHome;
            halo[z * 2 + 1] = c.m_phFood;
        }
        break;
    case 3: // West halo: neighbor's last column (its easternmost edge)
        for (int z = 0; z < CHUNK_SIZE; ++z)
        {
            const auto& c = _neighbor->m_cells[z * CHUNK_SIZE + (CHUNK_SIZE - 1)];
            halo[z * 2 + 0] = c.m_phHome;
            halo[z * 2 + 1] = c.m_phFood;
        }
        break;
    }
}

float TerrainChunk::ReadNeighborHome(int _localX, int _localZ) const
{
    // Inside chunk — direct read.
    if (_localX >= 0 && _localX < CHUNK_SIZE &&
        _localZ >= 0 && _localZ < CHUNK_SIZE)
    {
        return m_cells[_localZ * CHUNK_SIZE + _localX].m_phHome;
    }

    // Outside chunk — read from halo (or reflective if halo not populated).
    if (_localZ < 0) // North halo
    {
        int cx = Clamp(_localX, 0, CHUNK_SIZE - 1);
        return m_haloN[cx * 2 + 0];
    }
    if (_localZ >= CHUNK_SIZE) // South halo
    {
        int cx = Clamp(_localX, 0, CHUNK_SIZE - 1);
        return m_haloS[cx * 2 + 0];
    }
    if (_localX >= CHUNK_SIZE) // East halo
    {
        int cz = Clamp(_localZ, 0, CHUNK_SIZE - 1);
        return m_haloE[cz * 2 + 0];
    }
    if (_localX < 0) // West halo
    {
        int cz = Clamp(_localZ, 0, CHUNK_SIZE - 1);
        return m_haloW[cz * 2 + 0];
    }

    return 0.0f; // unreachable
}

float TerrainChunk::ReadNeighborFood(int _localX, int _localZ) const
{
    if (_localX >= 0 && _localX < CHUNK_SIZE &&
        _localZ >= 0 && _localZ < CHUNK_SIZE)
    {
        return m_cells[_localZ * CHUNK_SIZE + _localX].m_phFood;
    }

    if (_localZ < 0)
    {
        int cx = Clamp(_localX, 0, CHUNK_SIZE - 1);
        return m_haloN[cx * 2 + 1];
    }
    if (_localZ >= CHUNK_SIZE)
    {
        int cx = Clamp(_localX, 0, CHUNK_SIZE - 1);
        return m_haloS[cx * 2 + 1];
    }
    if (_localX >= CHUNK_SIZE)
    {
        int cz = Clamp(_localZ, 0, CHUNK_SIZE - 1);
        return m_haloE[cz * 2 + 1];
    }
    if (_localX < 0)
    {
        int cz = Clamp(_localZ, 0, CHUNK_SIZE - 1);
        return m_haloW[cz * 2 + 1];
    }

    return 0.0f;
}


// ============================================================================
// Delta sync
// ============================================================================

void TerrainChunk::SnapshotPheromones()
{
    for (int i = 0; i < CELL_COUNT; ++i)
    {
        m_phHomeSnapshot[i] = m_cells[i].m_phHome;
        m_phFoodSnapshot[i] = m_cells[i].m_phFood;
    }
}

int TerrainChunk::BuildDelta(PhDelta* _outBuf, int _maxDeltas, float _threshold) const
{
    int count = 0;
    for (int i = 0; i < CELL_COUNT && count < _maxDeltas; ++i)
    {
        float dHome = std::abs(m_cells[i].m_phHome - m_phHomeSnapshot[i]);
        float dFood = std::abs(m_cells[i].m_phFood - m_phFoodSnapshot[i]);

        if (dHome > _threshold || dFood > _threshold)
        {
            _outBuf[count].m_cellIndex = i;
            _outBuf[count].m_phHome    = m_cells[i].m_phHome;
            _outBuf[count].m_phFood    = m_cells[i].m_phFood;
            ++count;
        }
    }
    return count;
}

void TerrainChunk::ApplyDelta(const PhDelta* _deltas, int _count)
{
    for (int i = 0; i < _count; ++i)
    {
        int idx = _deltas[i].m_cellIndex;
        if (idx >= 0 && idx < CELL_COUNT)
        {
            m_cells[idx].m_phHome = _deltas[i].m_phHome;
            m_cells[idx].m_phFood = _deltas[i].m_phFood;
        }
    }
}


// ============================================================================
// Full sync (serialization)
// ============================================================================

int TerrainChunk::SerializePheromones(char* _outBuf, int _maxBytes) const
{
    // Two floats per cell: [phHome, phFood]
    const int requiredBytes = CELL_COUNT * 2 * static_cast<int>(sizeof(float));
    if (_maxBytes < requiredBytes)
        return 0;

    float* out = reinterpret_cast<float*>(_outBuf);
    for (int i = 0; i < CELL_COUNT; ++i)
    {
        out[i * 2 + 0] = m_cells[i].m_phHome;
        out[i * 2 + 1] = m_cells[i].m_phFood;
    }
    return requiredBytes;
}

void TerrainChunk::DeserializePheromones(const char* _buf, int _bytes)
{
    const int expectedBytes = CELL_COUNT * 2 * static_cast<int>(sizeof(float));
    if (_bytes < expectedBytes)
        return;

    const float* in = reinterpret_cast<const float*>(_buf);
    for (int i = 0; i < CELL_COUNT; ++i)
    {
        m_cells[i].m_phHome = in[i * 2 + 0];
        m_cells[i].m_phFood = in[i * 2 + 1];
    }
}
