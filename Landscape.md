# Landscape Overhaul — Stigmergic CA Substrate

## Executive Summary

Replace the current `Landscape` heightmap-only terrain with a dual-layer
system: the existing `SurfaceMap2D<float>` heightmap (geometry + ray-hit)
stays, but every grid cell gains a `TerrainCell` overlay carrying biome
type, food amount, and two pheromone channels.  The server owns all mutable
state (pheromone, food) and runs a cellular-automata tick at a fixed rate.
Clients reconstruct geometry and biome colour from the shared seed, then
receive only sparse pheromone deltas each tick.

---

## 1. Current State Analysis

### 1.1 Landscape class (`Starstrike/landscape.h/.cpp`)

| Member | Purpose |
|--------|---------|
| `SurfaceMap2D<float>* m_heightMap` | Per-cell terrain height; 272 references across codebase |
| `SurfaceMap2D<LegacyVector3>* m_normalMap` | Per-cell hit-check normals (38 references) |
| `float m_outsideHeight` | Default height for out-of-bounds lookups |
| `LandscapeRenderer* m_renderer` | GPU vertex buffer, colour array, triangle strips |
| `float m_worldSizeX, m_worldSizeZ` | Cached world extents |

The `Landscape` is initialised from a `LandscapeDef` (in `level_file.h`)
that carries `LandscapeTile` objects, each using diamond-square fractal
generation seeded by `m_randomSeed`.  The `LandscapeRenderer` reads a BMP
colour ramp from `terrain/<landscapeColourFilename>` to tint vertices by
height and gradient.

### 1.2 Key consumers (272 references to `m_landscape`)

| Access pattern | Count | Example files |
|---------------|-------|---------------|
| `m_heightMap->GetValue(x,z)` | ~130 | entity.cpp, darwinian.cpp, armyant.cpp, weapons.cpp |
| `m_normalMap->GetValue(x,z)` | ~38 | armour.cpp, DarwinianRenderer.cpp, ShadowRenderer.cpp |
| `GetWorldSizeX/Z()`, `IsInLandscape()` | ~30 | location.cpp, entity.cpp, obstruction_grid.cpp |
| `RayHit()` / `RayHitCell()` | ~15 | gamecursor.cpp, location.cpp, insertion_squad.cpp |
| `Init()` / `Empty()` / `Render()` | ~10 | location.cpp |
| `m_outsideHeight` | ~5 | landscape.cpp |

**Critical constraint**: Every one of these call sites must continue to
compile and work.  The heightmap and normal-map APIs cannot break.

### 1.3 Client/server split status

The project already has a `SERVER_BUILD` preprocessor guard
(`NeuronServer/NeuronServer.h`) and an existing `Server` class with
`ServerToClientLetter` + `NetworkUpdate` message passing.  The server
currently runs game logic ticks and sends update packets to clients; clients
apply updates locally.  The landscape itself is generated identically
client-side and server-side from the same `LandscapeDef` seed data.

### 1.4 Rendering pipeline

`LandscapeRenderer` (D3D12) builds a single GPU vertex buffer from the
heightmap.  Vertex colours are sampled from a BMP colour ramp
(`landscape_*.bmp`) based on height and gradient.  Two passes: `RenderMainSlow`
(lit, colour-material) and `RenderOverlaySlow` (blended triangle-outline
texture).

---

## 2. Requirements

### 2.1 Functional

| ID | Requirement |
|----|-------------|
| F1 | Each grid cell stores `TerrainType` (enum: Empty, Earth, Desert, Ice, Rock, Water), `foodAmount` (int), `m_phHome` (float), `m_phFood` (float). |
| F2 | Terrain generation uses layered Perlin/simplex noise to place biome regions deterministically from a seed. |
| F3 | Buildings are placed deterministically from the same seed. |
| F4 | CA tick: diffuse both pheromone channels to 8 neighbours (α ≈ 0.05), then evaporate (β ≈ 0.992), clamped to [0, MAX_PH]. |
| F5 | CA tick runs at a fixed rate decoupled from render frame rate (server-side only). |
| F6 | Only pheromone floats and `foodAmount` mutate; `TerrainType` is static after generation.  **`foodAmount` is server-authoritative state, not transmitted to clients.**  Clients render food from the initial seed-generated distribution; updated only on full sync (see §A.9). |
| F7 | Rendering maps `TerrainType` to `Assets/Terrain/landscape_*.bmp` textures. |

### 2.2 Client/server

| ID | Requirement |
|----|-------------|
| N1 | Server owns the authoritative grid and runs CA ticks + agent logic. |
| N2 | Clients receive sparse delta updates: cells whose pheromone changed beyond threshold (Δ > 0.05) as `{m_cellIndex, m_phHome, m_phFood}`. |
| N3 | Full grid sync on connect/reconnect only. |
| N4 | Clients regenerate geometry + biome from seed; overlay pheromone values for rendering. No agent logic client-side. |

### 2.3 Performance

| ID | Budget |
|----|--------|
| P1 | CA tick < 0.5 ms for a 256×256 grid on server (single-threaded). |
| P2 | Delta encoding: typical frame sends < 200 cells (< 2.4 KB). |
| P3 | No per-frame GPU re-upload for static terrain; only pheromone overlay needs updating. |
| P4 | Pheromone overlay rendering < 0.3 ms on client. |

---

## 3. Data Structures

### 3.1 TerrainType enum

```cpp
// In a new header: TerrainCell.h  (GameLogic project — no rendering deps)
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
```

### 3.2 TerrainCell

```cpp
struct TerrainCell
{
    TerrainType m_type;         // 1 byte — static after generation
    unsigned char _pad[3];      // alignment
    int           m_foodAmount; // mutable
    float         m_phHome;     // mutable — home pheromone
    float         m_phFood;     // mutable — food pheromone
};
// sizeof(TerrainCell) == 16  → cache-line friendly
```

### 3.3 TerrainChunk

The world is divided into fixed-size chunks.  The `TerrainChunk` is the
atomic unit for generation, CA ticking, delta sync, and persistence.
Consumers use the `TerrainWorld` facade (§3.4) and never address chunks
directly — the API is chunk-agnostic.

```cpp
// GameLogic/TerrainChunk.h
class TerrainChunk
{
public:
    static constexpr int CHUNK_SIZE = 128; // cells per side

    TerrainChunk(int _chunkX, int _chunkZ);
    ~TerrainChunk();

    int GetChunkX() const;
    int GetChunkZ() const;

    // --- Cell access (local coordinates 0..CHUNK_SIZE-1) ---
    TerrainCell&       GetCell(int _localX, int _localZ);
    const TerrainCell& GetCell(int _localX, int _localZ) const;
    TerrainType        GetTerrainType(int _localX, int _localZ) const;
    bool               IsPassable(int _localX, int _localZ) const;

    // --- Generation ---
    // _heightData points to the flat heightmap region covering this chunk's
    // world-space footprint.  _heightStride = floats per row in the source map.
    // Biome classification uses integer-quantised noise thresholds (§A.8)
    // so the result is deterministic across compilers/platforms.
    void Generate(int _seed, const float* _heightData, int _heightStride);

    // --- Pheromone CA (server-only) ---
    // Neighbours may be nullptr (world edge) — uses reflective boundary (§A.7).
    void TickPheromones(float _alpha, float _beta, float _maxPh,
                        const TerrainChunk* _neighborN,
                        const TerrainChunk* _neighborS,
                        const TerrainChunk* _neighborE,
                        const TerrainChunk* _neighborW);

    // --- Delta sync ---
    struct PhDelta { int m_cellIndex; float m_phHome; float m_phFood; };
    int  BuildDelta(PhDelta* _outBuf, int _maxDeltas, float _threshold) const;
    void ApplyDelta(const PhDelta* _deltas, int _count);
    void SnapshotPheromones(); // copy current → last-sent

    // --- Full sync ---
    int  SerializePheromones(char* _outBuf, int _maxBytes) const;
    void DeserializePheromones(const char* _buf, int _bytes);

    bool m_active;   // ticked when at least one client is subscribed or agents present
    bool m_dirty;    // pheromone changed since last delta build

private:
    int            m_chunkX, m_chunkZ;
    TerrainCell    m_cells[CHUNK_SIZE * CHUNK_SIZE];

    float          m_phHomeSnapshot[CHUNK_SIZE * CHUNK_SIZE];
    float          m_phFoodSnapshot[CHUNK_SIZE * CHUNK_SIZE];

    // Double-buffer scratch for diffusion (avoids read-after-write)
    float          m_phHomeScratch[CHUNK_SIZE * CHUNK_SIZE];
    float          m_phFoodScratch[CHUNK_SIZE * CHUNK_SIZE];

    // 1-cell halo buffers for ghost-cell edge exchange (§A.7)
    float          m_haloN[CHUNK_SIZE * 2]; // [home, food] interleaved
    float          m_haloS[CHUNK_SIZE * 2];
    float          m_haloE[CHUNK_SIZE * 2];
    float          m_haloW[CHUNK_SIZE * 2];

    void GenerateBiomes(int _seed, const float* _heightData, int _heightStride);
    void GenerateFood(int _seed);
    void CopyHaloFrom(const TerrainChunk* _neighbor, int _side);
};
```

### 3.4 TerrainWorld

`TerrainWorld` is the chunk-agnostic facade.  All consumer code
(`GetCell(worldX, worldZ)`, `GetTerrainType()`, `IsPassable()`) calls
this class; it resolves the chunk internally.  `Landscape` owns a single
`TerrainWorld*`.

```cpp
// GameLogic/TerrainWorld.h
class TerrainWorld
{
public:
    TerrainWorld();
    ~TerrainWorld();

    // _heightData: flat float array from SurfaceMap2D<float>, row-major.
    // _heightWidth/_heightHeight: dimensions of the heightmap.
    // The world is divided into ceil(w/CHUNK_SIZE) × ceil(h/CHUNK_SIZE) chunks.
    void Generate(int _worldCellsX, int _worldCellsZ, int _seed,
                  const float* _heightData, int _heightWidth, int _heightHeight);
    void Clear();

    // --- High-level accessors (world cell coordinates) ---
    int               GetWidth()  const;  // total cells X
    int               GetHeight() const;  // total cells Z
    TerrainCell&       GetCell(int _x, int _z);
    const TerrainCell& GetCell(int _x, int _z) const;
    TerrainType        GetTerrainType(int _x, int _z) const;
    bool               IsPassable(int _x, int _z) const;

    // --- Chunk access ---
    int                  GetChunksX() const;
    int                  GetChunksZ() const;
    TerrainChunk*        GetChunk(int _cx, int _cz);
    const TerrainChunk*  GetChunk(int _cx, int _cz) const;

    // --- CA tick (server-only, OMP parallel over active chunks) ---
    void TickActiveChunks(float _alpha, float _beta, float _maxPh);

    // --- Persistence (§A.4) ---
    void SaveSnapshot(const char* _filename) const;
    void LoadSnapshot(const char* _filename);

private:
    int             m_chunksX, m_chunksZ;
    TerrainChunk**  m_chunks; // flat [m_chunksZ * m_chunksX], nullable for streaming

    TerrainChunk**  m_activeChunks;
    int             m_activeChunkCount;

    void RebuildActiveList();
    void ExchangeHalos(); // copy edge pheromone into halo buffers before tick
};
```

### 3.5 Memory layout rationale

Per chunk (128×128 = 16 384 cells):
- `m_cells`: 16 384 × 16 bytes = **256 KB**
- `m_phHomeSnapshot` + `m_phFoodSnapshot`: 16 384 × 4 × 2 = **128 KB**
- `m_phHomeScratch` + `m_phFoodScratch`: 16 384 × 4 × 2 = **128 KB**
- `m_halo*`: 128 × 2 × 4 sides × 4 bytes = **4 KB**
- **Per-chunk total: ~516 KB**

For a 256×256 world (2×2 = 4 chunks): **~2 MB** — same as the old flat
layout.  For a 1024×1024 world (8×8 = 64 chunks): **~32 MB** — acceptable
on a server.  Chunks are nullable; only allocated when first accessed
(streaming-friendly).

The CA diffusion tick iterates `m_cells` linearly (hot data: `m_phHome`,
`m_phFood` at offset 8 and 12 in each 16-byte struct).  Cache lines are
64 bytes → 4 cells per line → good spatial locality for the 3×3 stencil.

**SoA fallback (R5):** If profiling reveals the interleaved AoS layout
hurts diffusion throughput, the first optimisation step is to split
pheromone into separate `float[]` arrays per chunk (SoA for the hot
channels only).  Use a ping-pong swap of the scratch pointer rather than a
memcpy write-back.  Profile after Phase 2 — the AoS layout should be
adequate for chunks up to 128×128.

---

## 4. Landscape Class Changes

### 4.1 Updated header (`landscape.h`)

> **Note — Landscape stays in Starstrike.**  `Landscape` owns
> `LandscapeRenderer*` (D3D12 GPU resources), so it cannot move to
> `GameLogic`.  The terrain *data* layer (`TerrainWorld`, `TerrainChunk`,
> `TerrainCell`) lives in `GameLogic` and has no rendering dependencies.

```cpp
#pragma once

#include "2d_array.h"
#include "2d_surface_map.h"

class LegacyVector3;
class BitmapRGBA;
class RGBAColour;
class LandscapeFlattenArea;
class Landscape;
class LandscapeDef;
class LandscapeRenderer;
class TerrainWorld;   // forward-declared; lives in GameLogic

// LandscapeTile stays unchanged — still used for heightmap fractal generation

class LandscapeTile { /* ... unchanged ... */ };

#define LIGHTMAP_SCALEFACTOR 1

class Landscape
{
public:
    // ---- Existing geometry layer (unchanged API) ----
    SurfaceMap2D<float>*         m_heightMap;
    SurfaceMap2D<LegacyVector3>* m_normalMap;
    float                        m_outsideHeight;
    LandscapeRenderer*           m_renderer;

    // ---- NEW: CA substrate layer ----
    // Always non-null after Init().  Legacy maps get an all-Earth grid
    // with zeroed pheromones (R3).
    TerrainWorld*                m_terrainWorld;

private:
    float m_worldSizeX;
    float m_worldSizeZ;

    void MergeTileIntoLandscape(const LandscapeTile* _tile);
    void GenerateHeightMap(LandscapeDef* _def);
    void GenerateNormals();
    void FlattenArea(const LandscapeFlattenArea* _area);
    void RenderHitNormals() const;
    bool UnsafeRayHit(const LegacyVector3& _rayStart,
                      const LegacyVector3& _rayEnd,
                      LegacyVector3* _result) const;

public:
    Landscape();
    ~Landscape();

    void BuildOpenGlState();

    void Init(LandscapeDef* _def, bool _justMakeTheHeightMap = false);
    void Empty();

    void Render();

    void DeleteTile(int tileId);

    float GetWorldSizeX() const;
    float GetWorldSizeZ() const;
    bool  IsInLandscape(const LegacyVector3& _pos);

    bool  RayHit(const LegacyVector3& _rayStart,
                 const LegacyVector3& _rayDir,
                 LegacyVector3* _result) const;
    bool  RayHitCell(int x0, int z0,
                     const LegacyVector3& _rayStart,
                     const LegacyVector3& _rayDir,
                     LegacyVector3* _result) const;
    float SphereHit(const LegacyVector3& _center, float _radius) const;

    // ---- NEW ----
    // Generate the terrain world from seed + heightmap.  Called from
    // Location::Init() after InitLandscape() so heights are available.
    // If _seed < 0 (legacy map), creates an all-Earth grid with zero
    // pheromones so GetTerrainWorld() is always valid (R3).
    void GenerateTerrainWorld(int _seed);

    // Server-only: advance the CA substrate by one tick.
    void TickCA(float _alpha, float _beta, float _maxPh);

    // Never returns null after Init() (R3).
    TerrainWorld*       GetTerrainWorld();
    const TerrainWorld* GetTerrainWorld() const;
};
```

### 4.2 Key implementation notes

**`Landscape::Init()`** — unchanged.  It still generates heightmap from
`LandscapeDef` tiles.  The `Landscape(float, int, int)` constructor is
removed (unused outside unit tests).

**`Landscape::GenerateTerrainWorld(int _seed)`** — new function:
1. Computes grid dimensions from `m_worldSizeX / m_heightMap->m_cellSizeX`.
2. Extracts a flat `const float*` from the `SurfaceMap2D<float>` heightmap
   (R2 — avoids a GameLogic → Starstrike dependency on `SurfaceMap2D`).
3. Allocates `m_terrainWorld` if null, or verifies dimensions match.
4. Calls `m_terrainWorld->Generate(cellsX, cellsZ, _seed, heightData, w, h)`.
5. If `_seed < 0` (legacy map with no `terrainSeed`), the world is still
   allocated but all cells default to `TerrainType::Earth` with zero
   pheromones (R3).  This guarantees `GetTerrainWorld()` never returns null.

```cpp
void Landscape::GenerateTerrainWorld(int _seed)
{
    DEBUG_ASSERT(m_heightMap);

    const int cellsX = static_cast<int>(m_worldSizeX / m_heightMap->m_cellSizeX);
    const int cellsZ = static_cast<int>(m_worldSizeZ / m_heightMap->m_cellSizeX);

    // Extract flat height data from SurfaceMap2D (R2).
    // SurfaceMap2D stores data in a flat array accessible via GetData().
    const float* heightData = m_heightMap->GetData();
    const int    heightW    = m_heightMap->GetNumColumns();
    const int    heightH    = m_heightMap->GetNumRows();

    if (!m_terrainWorld)
        m_terrainWorld = new TerrainWorld();

    m_terrainWorld->Generate(cellsX, cellsZ, _seed, heightData, heightW, heightH);
}
```

**`Landscape::TickCA()`** — thin wrapper:
```cpp
void Landscape::TickCA(float _alpha, float _beta, float _maxPh)
{
    DEBUG_ASSERT(m_terrainWorld);
    m_terrainWorld->TickActiveChunks(_alpha, _beta, _maxPh);
}
```

**`Landscape::Empty()`** — extended:
```cpp
void Landscape::Empty()
{
    delete m_renderer;      m_renderer     = nullptr;
    delete m_heightMap;     m_heightMap    = nullptr;
    delete m_normalMap;     m_normalMap    = nullptr;
    delete m_terrainWorld;  m_terrainWorld = nullptr;
}
```

---

## 5. TerrainGrid Generation (Deterministic)

### 5.1 Biome placement via Perlin noise (integer-quantised)

`TerrainChunk::Generate()` receives the heightmap slice and uses layered
Perlin noise seeded deterministically.  To guarantee identical biome
classification across compilers and platforms (§A.8), noise outputs are
quantised to fixed-point integers before threshold comparison:

```
1. base_noise     = Perlin(x, z, seed+0, octaves=4, persistence=0.5)
2. moisture_noise = Perlin(x, z, seed+1, octaves=3, persistence=0.6)
3. Quantise to integer (multiply ×1000, round):
     base1000     = int(base_noise     * 1000.0f + 0.5f)
     moisture1000 = int(moisture_noise  * 1000.0f + 0.5f)
4. Classify each cell (using heightData passed via Generate — R2):
     if heightData[x + z * stride] <= 0  → Water
     if base1000 > 700                   → Rock
     if moisture1000 < 200               → Desert
     if moisture1000 > 700 && cold       → Ice
     else                                → Earth
5. Scatter food:
     if type == Earth  → foodAmount = Perlin(x,z, seed+2) * MAX_FOOD
     else              → foodAmount = 0
6. Pheromones initialised to 0.
```

The noise function uses a portable, deterministic implementation (e.g.
Perlin reference or simplex) that produces identical output across x86/x64
regardless of compiler.  The integer quantisation step makes the rounding
mode irrelevant for values not exactly on a threshold boundary.

### 5.2 Building placement

Buildings are also placed deterministically from the seed.  The existing
`LevelFile::m_buildings` list is populated during `ParseBuildings` from the
map file.  For the new procedural mode, a new `GenerateBuildings(int _seed)`
method on `LevelFile` (or a standalone function) walks the terrain grid and
places buildings at biome-appropriate locations:

- AntHill on Earth near food clusters
- Factories on Rock edges
- Spirit Receivers on high ground

This replaces the handcrafted building list from map files when a procedural
seed is specified.

---

## 6. CA Substrate Tick (Server-Side)

### 6.1 Algorithm

Each chunk is ticked independently after ghost-cell halo exchange (§A.7).
World edges use reflective boundary (mirror the cell’s own value) so edge
cells behave identically to interior cells:

```
// Halo exchange (before tick)
for each active chunk:
    CopyHaloFrom(neighborN, North)  // nullptr → reflective
    CopyHaloFrom(neighborS, South)
    CopyHaloFrom(neighborE, East)
    CopyHaloFrom(neighborW, West)

// Per-chunk diffusion
for each cell (x, z) in chunk:
    sum_home = 0, sum_food = 0
    for each neighbour (dx, dz) in [-1,0,+1]×[-1,0,+1], skip (0,0):
        // Read from halo buffer if neighbour is in an adjacent chunk
        // Read cell’s own value if neighbour is out-of-world (reflective)
        sum_home += readNeighbour(x+dx, z+dz).m_phHome
        sum_food += readNeighbour(x+dx, z+dz).m_phFood
    // Always 8 neighbours (reflective boundary guarantees this)
    scratch_home[x,z] = cells[x,z].m_phHome + alpha * (sum_home / 8 - cells[x,z].m_phHome)
    scratch_food[x,z] = cells[x,z].m_phFood + alpha * (sum_food / 8 - cells[x,z].m_phFood)

// Apply evaporation + write back
for each cell (x, z):
    cells[x,z].m_phHome = clamp(scratch_home[x,z] * beta, 0, MAX_PH)
    cells[x,z].m_phFood = clamp(scratch_food[x,z] * beta, 0, MAX_PH)
```

### 6.2 Parameters (data-driven)

```
CA_ALPHA         = 0.05   // diffusion fraction
CA_BETA          = 0.992  // evaporation multiplier
CA_MAX_PH        = 100.0  // pheromone clamp
CA_TICK_RATE_HZ  = 10     // ticks per second (decoupled from render)
CA_DELTA_THRESHOLD = 0.05 // minimum change to transmit
```

These are loaded from a data file (`terrain_ca.txt`) or a preferences
entry, not hardcoded.  Tunable at runtime via console commands.

### 6.3 Tick scheduling

The CA tick runs in the server's existing `Advance()` loop, gated by an
accumulator.  **Entity logic (pheromone deposits) always runs before
`AdvanceCA()` in the same frame** (R4) so that deposits from the current
tick are diffused immediately:

```cpp
// In Location::Advance()  (server-only)
// 1. Entity logic runs first (deposits pheromone)
for (int slice = 0; slice < NUM_SLICES_PER_FRAME; ++slice)
    AdvanceEntities(slice);

// 2. Then CA tick
m_caAccumulator += SERVER_ADVANCE_PERIOD;
const float caTickInterval = 1.0f / CA_TICK_RATE_HZ;
while (m_caAccumulator >= caTickInterval)
{
    m_landscape.TickCA(CA_ALPHA, CA_BETA, CA_MAX_PH);
    m_caAccumulator -= caTickInterval;
}
```

This runs inside the existing fixed-timestep simulation loop, so it is
naturally decoupled from the render frame rate.

---

## 7. Client/Server Networking

### 7.1 New NetworkUpdate types

Extend the existing `NetworkUpdate::UpdateType` enum:

```cpp
enum UpdateType
{
    // ...existing...
    PheromoneDeltas,     // Sparse delta update (per-tick)
    PheromoneFullSync,   // Full grid sync (on connect)
    // ...
};
```

### 7.2 Delta message format

```
Header:
    uint16  m_deltaCount     // number of changed cells (typically < 200)
Per delta:
    uint32  m_cellIndex      // flat index into TerrainGrid
    float   m_phHome
    float   m_phFood
```

Size per delta: 12 bytes.  200 deltas = 2.4 KB + 2-byte header.  Fits
within a single UDP packet or `ServerToClientLetter`.

### 7.3 Server sends deltas

The correct ordering is **snapshot → tick → build → send** (R7).  The
snapshot captures the baseline *before* the tick modifies pheromone values,
so `BuildDelta` compares post-tick state against the pre-tick baseline.
This avoids a timing hole where deposits between snapshot and build would
be silently dropped:

```cpp
// Per CA tick (inside the accumulator loop)
m_terrainWorld->SnapshotAllDirtyChunks();         // 1. capture baseline
m_landscape.TickCA(CA_ALPHA, CA_BETA, CA_MAX_PH); // 2. diffuse + evaporate

// 3. Build + send deltas per dirty chunk
for (int i = 0; i < m_terrainWorld->GetActiveChunkCount(); ++i)
{
    TerrainChunk* chunk = m_terrainWorld->GetActiveChunk(i);
    if (!chunk->m_dirty) continue;

    TerrainChunk::PhDelta deltas[MAX_DELTAS_PER_CHUNK];
    int count = chunk->BuildDelta(deltas, MAX_DELTAS_PER_CHUNK, CA_DELTA_THRESHOLD);
    if (count > 0)
    {
        // Package into a bulk ServerToClientLetter (§A.5)
        // and send to subscribers of this chunk only (§A.2)
        auto* letter = new ServerToClientLetter();
        letter->m_type = ServerToClientLetter::ChunkPheromoneUpdate;
        // ... serialize chunk id + deltas into bulk payload ...
        server->SendLetterToChunkSubscribers(letter, chunk->GetChunkX(), chunk->GetChunkZ());
    }
    chunk->m_dirty = false;
}
```

### 7.4 Client receives deltas

In the client's `ReceiveLetter()` handler, when `ChunkPheromoneUpdate` type
is seen:

```cpp
case ServerToClientLetter::ChunkPheromoneUpdate:
{
    int chunkX, chunkZ;
    // ... deserialize chunkX, chunkZ, deltas from bulk payload ...
    TerrainChunk* chunk = g_context->m_location->m_landscape
        .GetTerrainWorld()->GetChunk(chunkX, chunkZ);
    if (chunk)
        chunk->ApplyDelta(deltas, count);
    // Set renderer dirty flag (R6)
    break;
}
```

### 7.5 Full sync on subscribe

When a client subscribes to a chunk (initial connect or camera move into a
new chunk), the server sends the complete pheromone state for that chunk as
a `ChunkPheromoneFullSync` message (§A.2).  Per-chunk cost:
128×128 × 8 bytes (two floats) = **128 KB**, sent as a reliable ordered
message.  On initial connect, ~9–25 chunks are synced for the visible area
around spawn (§A.2).

Periodic heartbeat re-sync: every 30 seconds (staggered across chunks),
the server sends a full sync to all subscribers of each chunk to cap
maximum pheromone drift from UDP packet loss (§A.6).

### 7.6 Client-side: no agent logic

The client never calls `TickCA()`.  It only:
1. Generates the static terrain grid from the seed (geometry + biome types).
2. Applies incoming pheromone deltas.
3. Renders the pheromone overlay.

---

## 8. Rendering

### 8.1 Static terrain geometry — unchanged

The existing `LandscapeRenderer` continues to build vertex positions,
normals, and triangle strips from the `SurfaceMap2D<float>` heightmap.
This geometry is uploaded to GPU once and never changes.

### 8.2 Vertex colour from TerrainType

Currently, `GetLandscapeColour()` samples a single BMP colour ramp
based on height and gradient.  The change:

1. Load one BMP per `TerrainType`:

| TerrainType | BMP file |
|-------------|----------|
| Earth | `landscape_earth.bmp` |
| Desert | `landscape_desert.bmp` |
| Ice | `landscape_icecaps.bmp` |
| Rock | `landscape_rock.bmp` |
| Water | (below water — existing water renderer handles this) |
| Empty | `landscape_earth.bmp` (fallback) |

2. In `LandscapeRenderer`, store an array of `BitmapRGBA*` indexed by
   `TerrainType`.

3. `GetLandscapeColour()` receives the cell's `TerrainType` and
   samples from the corresponding BMP:

```cpp
void LandscapeRenderer::GetLandscapeColour(
    float _height, float _gradient,
    unsigned int _x, unsigned int _y,
    TerrainType _type,
    RGBAColour* _colour)
{
    BitmapRGBA* ramp = m_landscapeColours[static_cast<int>(_type)];
    if (!ramp)
        ramp = m_landscapeColours[static_cast<int>(TerrainType::Earth)];
    // ... existing height/gradient → UV lookup against ramp ...
    *_colour = ramp->GetPixel(u, v);
}
```

4. `BuildColourArray()` passes the `TerrainType` for each vertex's
   world-space position by looking up the `TerrainWorld`.

### 8.3 Pheromone overlay (new)

A second render pass overlays pheromone intensity as a translucent colour
wash.  Options (ordered by implementation simplicity):

**Option A — Per-vertex colour blend (simplest)**
- Track a `m_pheromoneDirty` flag on the renderer, set when `ApplyDelta()`
  is called (R6).  Only rebuild the overlay colour sub-buffer when the flag
  is set — typically at the CA tick rate (10 Hz), not every render frame
  (60 Hz).  Clear the flag after upload.
- On rebuild, iterate only the vertices whose chunk received deltas this
  tick (partial sub-buffer update).  Read `m_phHome` and `m_phFood` from
  the `TerrainWorld` for the cell under each vertex.
- Blend a colour (e.g. blue for home, green for food) with intensity
  proportional to pheromone value, into a second vertex colour channel
  or a dynamic overlay pass.
- Re-upload only the affected colour sub-buffer region.

**Option B — Screen-space texture (more scalable)**
- Maintain a small CPU-side texture (256×256 RGBA) mapping pheromone
  values to colour.
- Upload as a dynamic D3D12 texture each frame.
- Render a full-screen-quad or a second landscape pass sampling this texture
  with world-space UVs.

**Recommended: Option A** for initial implementation (matches existing
`BuildColourArray` pattern), with Option B as a future optimisation if the
per-vertex update becomes a bottleneck.

---

## 9. Integration Points

### 9.1 Location class (`location.h`)

```cpp
class Location
{
    // ...existing...
    Landscape m_landscape;   // ← already here; gains m_terrainWorld member

    // NEW: CA tick accumulator (server-only)
    float m_caAccumulator;

    void AdvanceCA(int _slice);   // new
    // ...
};
```

`Location::Init()` changes:

```cpp
void Location::Init(const char* _missionFilename, const char* _mapFilename)
{
    LoadLevel(_missionFilename, _mapFilename);
    darwiniaSeedRandom(1);

    InitLights();
    InitLandscape();

    // NEW: always generate terrain world (R3).
    // If terrainSeed >= 0, biome classification runs from the seed.
    // If terrainSeed < 0 (legacy map), all cells default to Earth with
    // zero pheromones.  GetTerrainWorld() is never null after this point.
    int terrainSeed = m_levelFile->m_landscape.m_terrainSeed;
    m_landscape.GenerateTerrainWorld(terrainSeed);

    m_water = new Water();
    // ...rest unchanged...
}
```

`Location::Advance()` calls `AdvanceCA()` which runs the accumulator
logic from §6.3, but only `#ifndef` client-only or guarded by a
`g_context->IsServer()` check.

### 9.2 LevelFile / LandscapeDef

`LandscapeDef` gains an optional `int m_terrainSeed` field.  When present
in the map file, it is used instead of deriving the seed from tile data.
This keeps the existing map format backward-compatible.

### 9.3 Existing gameplay code

All 272 references to `m_landscape.m_heightMap->GetValue()` etc. are
**unchanged**.  The heightmap and normal map continue to exist and function
identically.  Gameplay code that needs terrain type or food amount accesses:

```cpp
TerrainWorld* world = g_context->m_location->m_landscape.GetTerrainWorld();
TerrainType type = world->GetTerrainType(cellX, cellZ);
int food = world->GetCell(cellX, cellZ).m_foodAmount;
```

Gameplay code that deposits pheromone (e.g. ArmyAnt returning to base):

```cpp
// Server-side only, inside ArmyAnt::Advance or similar
TerrainWorld* world = g_context->m_location->m_landscape.GetTerrainWorld();
int cx = static_cast<int>(m_pos.x / cellSize);
int cz = static_cast<int>(m_pos.z / cellSize);
world->GetCell(cx, cz).m_phHome += DEPOSIT_AMOUNT;
```

### 9.4 ArmyAnt / Darwinian AI

The existing `ArmyAnt` already navigates via `m_wayPoint` and height
lookups.  To make ants stigmergic:

- **Scouting**: when wandering, bias direction towards cells with low
  `m_phHome` (explore unvisited territory).
- **Foraging**: when food is found, deposit `m_phFood` along the return
  path.  Other ants bias direction towards high `m_phFood`.
- **Homing**: deposit `m_phHome` on every step; lost ants follow the
  `m_phHome` gradient back to the colony.

This is a **gameplay change** to `ArmyAnt::SearchForRandomPosition()` and
`ArmyAnt::AdvanceToTargetPosition()`, not an engine change.  It reads from
`TerrainGrid` and writes pheromone deposits server-side.

### 9.5 GlobalWorld — Location Registration & Selection

#### 9.5.1 How the existing system works

The game's world-selection screen is driven by `GlobalWorld`
(`Starstrike/global_world.h/.cpp`).  The full lifecycle:

1. **Data files** define the location catalogue:
   - `game.txt` (or `game_unlockall.txt`) — `Locations_StartDefinition`
     block, one row per location: `{id, avail, mapFile, missionFile}`.
   - `locations.txt` — sphere-world 3D positions: `{id, x, y, z}`.

2. **Load**: `GameApp::LoadCampaign()` sets `m_gameDataFile = "game.txt"`,
   calls `LoadProfile()`, which calls `GlobalWorld::LoadGame(game.txt)`.
   That parses `Locations_StartDefinition` → `ParseLocations()` → creates
   `GlobalLocation*` objects.  Then `LoadLocations("locations.txt")` assigns
   3D positions.  The location name is derived automatically: strip `map_`
   prefix and `.txt` extension from the map filename (e.g.
   `map_colony.txt` → name `colony`).

3. **Selection**: `GlobalWorld::Advance()` checks for user click →
   `LocationHit()` → if `loc->m_available && loc->m_missionFilename != "null"`
   → starts fade-out → on fade complete, copies `loc->m_mapFilename` and
   `loc->m_missionFilename` into `g_context->m_requestedMap` and
   `g_context->m_requestedMission`, sets `g_context->m_requestedLocationId`.

4. **Transition**: `RunTheGame()` main loop sees `m_requestedLocationId != -1`
   → `EnterLocation()` → `new Location()` →
   `Location::Init(m_requestedMission, m_requestedMap)` →
   `LoadLevel()` → `LevelFile(mission, map)` → `ParseMapFile()` →
   `ParseLandscapeData()` + `ParseLandscapeTiles()` → heightmap generation.

5. **Rendering on sphere**: `SphereWorld::RenderIslands()` iterates
   `m_locations` and renders a starburst quad for each where
   `loc->m_available == true`.  Names are drawn below via
   `GetLocationNameTranslated()`.

6. **Events**: `GlobalEventAction::MakeAvailable` sets
   `loc->m_available = true` at runtime, gated by conditions
   (e.g. `BuildingOnline`, `ResearchOwned`).

#### 9.5.2 Colony location registration  ✅ IMPLEMENTED

The colony (procedural terrain) location is registered as location **ID 12**
via data files — no code changes required for registration itself:

**`Assets/game.txt`** — new row in `Locations_StartDefinition`:
```
   12   1       map_colony.txt                  mission_colony_explore.txt
```

**`Assets/game_unlockall.txt`** — same, with `avail=0` (unlocked at
runtime by the `LoadProfile` code that sets all locations available):
```
   12   0       map_colony.txt                  mission_colony_explore.txt
```

**`Assets/locations.txt`** — sphere-world position:
```
12     -10.50   -275.00  -50.00
```

This causes `GlobalWorld::ParseLocations()` to create a `GlobalLocation`
with `m_id=12`, `m_name="colony"`, `m_available=true`, pointing at the
new map and mission files.  The location appears on the sphere world and is
selectable by the player through the standard click → fade → enter flow.

#### 9.5.3 Map file: `Assets/Levels/map_colony.txt`  ✅ CREATED

The map file uses the existing `Landscape_StartDefinition` format with one
addition — the `terrainSeed` keyword (parsed by the new code in
`LevelFile::ParseLandscapeData()`):

```
Landscape_StartDefinition
    worldSizeX 5000
    worldSizeZ 5000
    cellSize 19.53
    outsideHeight -12.00
    landColourFile landscape_default.bmp
    wavesColourFile waves_default.bmp
    waterColourFile water_default.bmp
    terrainSeed 42
Landscape_EndDefinition
```

Key points:
- `terrainSeed 42` — triggers `TerrainWorld` generation when implemented.
  Parsed into `LandscapeDef::m_terrainSeed` (default `-1` = legacy, no
  terrain grid).
- Four overlapping `LandscapeTile` entries with varied fractal parameters
  and seeds provide terrain height variation across the map (R8).  The
  `TerrainWorld` biome overlay is layered on top from the same seed.
- A starter `ControlTower` building is included.
- The `location_colony` language string is registered in `english.txt` (R9).

#### 9.5.4 Mission file: `Assets/Levels/mission_colony_explore.txt`  ✅ CREATED

A minimal mission stub with one primary objective (bring the control tower
online).  Can be expanded as gameplay features land.

#### 9.5.5 LandscapeDef `m_terrainSeed` field  ✅ IMPLEMENTED

`LandscapeDef` in `level_file.h` now has:

```cpp
int m_terrainSeed;  // -1 = no terrain grid (legacy); >= 0 = generate TerrainGrid
```

Initialized to `-1` in the constructor.  `ParseLandscapeData()` recognises
the `terrainSeed` keyword; `WriteLandscapeData()` writes it only when
`m_terrainSeed >= 0` (backward-compatible with all existing map files).

#### 9.5.6 Updated `Location::Init()` seed resolution

With the `m_terrainSeed` field available, the seed resolution in
`Location::Init()` is unconditional (R3):

```cpp
// After InitLandscape():
int terrainSeed = m_levelFile->m_landscape.m_terrainSeed;
m_landscape.GenerateTerrainWorld(terrainSeed);
// Legacy maps (terrainSeed == -1) get an all-Earth grid with zero
// pheromones.  GetTerrainWorld() is never null.
```

This replaces the earlier proposal of a conditional allocation.
The explicit `terrainSeed` field in the map file is cleaner: legacy maps
are unaffected (default `-1`), and new procedural maps declare their seed
explicitly.

#### 9.5.7 Making the location available via events (optional)

If the colony should be locked initially and unlocked through gameplay
progression (rather than always available with `avail=1`), add an event
to `game.txt`:

```
Events_StartDefinition
    Event BuildingOnline:garden,8
        Action MakeAvailable colony
        End
Events_EndDefinition
```

This unlocks the colony location when building 8 at Garden comes online.
For development, `avail=1` makes it immediately selectable.

#### 9.5.8 Script access

The `Script` system can also enter the colony programmatically:

```
EnterLocation colony
```

This calls `Script::RunCommand_EnterLocation("colony")` which looks up
`GlobalWorld::GetLocationId("colony")` → returns 12, copies the map/mission
filenames into `g_context`, and triggers the location transition.

---

## 10. File Plan

### 10.1 New files

| File | Project | Purpose |
|------|---------|---------|
| `GameLogic/TerrainCell.h` | GameLogic | `TerrainType` enum + `TerrainCell` struct |
| `GameLogic/TerrainChunk.h` | GameLogic | `TerrainChunk` class declaration |
| `GameLogic/TerrainChunk.cpp` | GameLogic | Per-chunk generation, CA tick, halo exchange, delta sync |
| `GameLogic/TerrainWorld.h` | GameLogic | `TerrainWorld` facade (chunk-agnostic API) |
| `GameLogic/TerrainWorld.cpp` | GameLogic | Chunk management, parallel tick, persistence |
| `GameLogic/PerlinNoise.h` | GameLogic | Deterministic Perlin noise (header-only or .cpp) |
| `GameLogic/PerlinNoise.cpp` | GameLogic | Perlin noise implementation |
| `Assets/Levels/map_colony.txt` | Starstrike (data) | Map file for colony location — `terrainSeed 42` |
| `Assets/Levels/mission_colony_explore.txt` | Starstrike (data) | Mission stub for colony location |

### 10.2 Modified files

| File | Change |
|------|--------|
| `Starstrike/landscape.h` | Add `TerrainWorld* m_terrainWorld`, new methods |
| `Starstrike/landscape.cpp` | `GenerateTerrainWorld()`, `TickCA()`, update `Empty()` constructor/destructor |
| `Starstrike/landscape_renderer.h` | Array of `BitmapRGBA*` per terrain type; updated `GetLandscapeColour` signature; `m_pheromoneDirty` flag |
| `Starstrike/landscape_renderer.cpp` | Load per-biome BMPs; pass `TerrainType` to colour lookup; dirty-flag pheromone overlay pass |
| `Starstrike/location.h` | Add `m_caAccumulator`, `AdvanceCA()` |
| `Starstrike/location.cpp` | Call `GenerateTerrainWorld()` unconditionally in `Init()`; call `AdvanceCA()` in `Advance()` |
| `Starstrike/level_file.h` | Add `m_terrainSeed` to `LandscapeDef` ✅ |
| `Starstrike/level_file.cpp` | Parse/write `terrainSeed` keyword ✅ |
| `Assets/game.txt` | Add colony location entry (ID 12, avail=1) ✅ |
| `Assets/game_unlockall.txt` | Add colony location entry (ID 12, avail=0) ✅ |
| `Assets/locations.txt` | Add sphere-world position for ID 12 ✅ |
| `Assets/Language/english.txt` | Add `location_colony` string ✅ |
| `NeuronClient/servertoclientletter.h` | Add `ChunkPheromoneUpdate` letter type + bulk payload fields (§A.5) |
| `NeuronClient/servertoclientletter.cpp` | Serialize/deserialize bulk pheromone payloads |
| `NeuronClient/server.cpp` | Send delta packets per-chunk to subscribers after CA ticks |
| `NeuronClient/server.h` | Per-client chunk subscription tracking (`ClientSession`) |
| `GameLogic/armyant.cpp` | (Future) Stigmergic navigation using pheromone gradients |

### 10.3 Header dependency chain

```
TerrainCell.h          ← no includes (standalone enum + struct)
TerrainChunk.h         ← includes TerrainCell.h only
TerrainWorld.h         ← includes TerrainChunk.h
landscape.h            ← adds forward declaration: class TerrainWorld
                          (no #include TerrainWorld.h — keeps header light)
landscape.cpp          ← #include "TerrainWorld.h"
```

This means modifying `TerrainChunk.h` or `TerrainWorld.h` does **not**
trigger a rebuild of the ~50 files that include `landscape.h` transitively.

---

## 11. Implementation Order

### Phase 1a — Static data layer (biome only, no pheromones)

1. Create `TerrainCell.h` + `TerrainChunk.h/.cpp` + `TerrainWorld.h/.cpp`
   + `PerlinNoise.h/.cpp`.
2. `TerrainCell` initially has only `m_type` and `m_foodAmount`.  The
   `m_phHome`/`m_phFood` fields and all scratch/snapshot buffers are
   **not yet added** — keeps the struct at 8 bytes and avoids allocating
   unused memory on clients that will never run pheromone logic.
3. `TerrainChunk::Generate()` takes `(seed, heightData, stride)` (R2)
   and fills `m_type` + `m_foodAmount` using integer-quantised noise (§A.8).
4. `TerrainWorld` manages a 2D array of `TerrainChunk*`; exposes
   chunk-agnostic `GetCell(x,z)` / `GetTerrainType(x,z)` / `IsPassable(x,z)`.
5. Add `TerrainWorld* m_terrainWorld` to `Landscape`; implement
   `GenerateTerrainWorld()` (extracts `const float*` from SurfaceMap2D — R2).
6. Call `GenerateTerrainWorld()` unconditionally from `Location::Init()`
   (R3).  Legacy maps get an all-Earth grid.
7. Verify build.  Unit test: generate two worlds from same seed, compare
   chunk-by-chunk (memcmp).

### Phase 1b — Pheromone fields + scratch buffers

8. Add `m_phHome`, `m_phFood` to `TerrainCell` (now 16 bytes).
9. Add per-chunk scratch, snapshot, and halo buffers.
10. Verify build.  Unit test: deposit pheromone at a cell, read it back.

### Phase 2 — CA tick (chunked, parallel)

11. Implement `TerrainChunk::TickPheromones()` with halo edge exchange
    and reflective boundary (§A.7).
12. Implement `TerrainWorld::TickActiveChunks()` with OMP parallel.
13. Wire into `Location::AdvanceCA()` (entity deposits run first — R4).
14. Gate behind `g_context->IsServer()` or `#ifndef SERVER_BUILD`.
15. Unit test: deposit pheromone at a chunk border, tick N times, verify
    diffusion crosses into the neighbouring chunk correctly and
    evaporation conserves total pheromone within epsilon.

### Phase 3 — Rendering

16. Load per-biome BMP colour ramps in `LandscapeRenderer`.
17. Update `GetLandscapeColour()` to accept `TerrainType`; `BuildColourArray`
    looks up the `TerrainWorld` chunk for each vertex's world position.
18. Add dirty-flag pheromone overlay pass (Option A: per-vertex colour
    blend, rebuild only when deltas arrive — R6).
19. Visual verification: run client, confirm biome colours and pheromone
    heat-map render correctly.

### Phase 4 — Networking (AoI-aware)

20. Add `ChunkPheromoneUpdate` / `ChunkPheromoneFullSync` to
    `ServerToClientLetter` as bulk-data letter types (§A.5 — bypasses
    the 42-byte `NetworkUpdate` limit).
21. Add `ChunkSubscribe` / `ChunkUnsubscribe` client→server messages.
22. Server tracks per-client chunk subscriptions (`ClientSession`) (§A.2).
23. Implement per-chunk `BuildDelta()` / `ApplyDelta()` /
    `SnapshotPheromones()` with snapshot→tick→build→send ordering (R7).
24. Full sync on subscribe (not global connect) (§A.2).
25. Periodic heartbeat full-sync per chunk to cap drift (§A.6).
26. Integration test: run server + client, verify pheromone state converges.

### Phase 5 — Persistence + Gameplay

27. Implement `TerrainWorld::SaveSnapshot()` / `LoadSnapshot()` (§A.4).
28. Integrate with the existing save system: extend the save file format
    with a `TerrainPheromones` block written by `Location::Save()` and
    read by `Location::Load()` (R12).  Only dirty chunks are serialized.
29. Update `ArmyAnt` (and other agents) to read/write pheromone channels.
30. Add console commands to visualize pheromone overlay, adjust CA params.
31. Tune α, β, deposit rates, food distribution.

---

## 12. Trade-Off Analysis

### 12.1 Struct-of-arrays vs. array-of-structs for TerrainCell

| | AoS (chosen) | SoA |
|---|---|---|
| **Pros** | Simple addressing; single alloc; good for random access | Better cache utilisation for CA tick (only touches float channels) |
| **Cons** | CA tick touches 16 bytes per cell but only needs 8 | Multiple arrays to manage; more complex serialisation |
| **Decision** | AoS for now.  16-byte stride is 4 cells/cache-line — adequate for 128×128 chunks. |
| **Perf note** | Profile after Phase 2. If per-chunk CA tick > target budget, first optimisation: split pheromone channels into separate `float[]` arrays (SoA for hot channels only).  Use a ping-pong swap of the scratch pointer (`std::swap(m_phHome, m_phHomeScratch)`) rather than a `memcpy` write-back.  This eliminates the evaporation write-back pass entirely. |

### 12.2 Noise algorithm: Perlin vs. Simplex

| | Perlin | Simplex |
|---|---|---|
| **Pros** | Well-understood, many reference implementations | Fewer artifacts, faster in higher dimensions |
| **Cons** | Axis-aligned artifacts at low octaves | More complex implementation |
| **Decision** | Classic Perlin (2D only — artifacts acceptable for biome classification). |

### 12.3 Pheromone overlay: per-vertex vs. texture

| | Per-vertex (chosen) | Dynamic texture |
|---|---|---|
| **Pros** | Matches existing pattern; no new GPU resource types | Decoupled from mesh resolution; scalable |
| **Cons** | Re-upload vertex colours each frame if pheromones changed | Adds texture bind, extra shader pass |
| **Decision** | Per-vertex for Phase 3. Re-evaluate after profiling. |

### 12.4 Delta threshold

| Threshold | Typical deltas/tick | Bandwidth |
|-----------|-------------------|-----------|
| 0.01 | ~2000 | 24 KB |
| 0.05 | ~200 | 2.4 KB |
| 0.10 | ~50 | 0.6 KB |

**Decision**: 0.05 default.  Tunable at runtime.  Clients interpolate
between received values to mask the quantisation.

---

## 13. Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| CA tick exceeds budget on large grids | Server frame budget blown | SoA split; SIMD for diffusion; reduce tick rate; tick only active chunks (§A.3) |
| Floating-point non-determinism across compilers | Client/server biome mismatch | Integer-quantised biome classification (§A.8); pheromone is server-authoritative |
| 272 existing `m_landscape` references break | Full build failure | Heightmap/normalmap API is unchanged; `TerrainWorld` is additive only |
| Delta packets lost (UDP) | Client pheromone drift | Periodic per-chunk full-sync heartbeat every 30 s (§A.6) |
| Per-biome BMP files missing | Crash in `LandscapeRenderer` | Fallback to `landscape_earth.bmp` if file not found (assert in debug) |
| Chunk boundary pheromone discontinuity | Visual seams, incorrect agent behavior | Ghost-cell halo exchange before each tick (§A.7) |
| `NetworkUpdate` 42-byte limit blocks bulk data | Cannot ship deltas | Bulk-data `ServerToClientLetter` type; bypass `NetworkUpdate` (§A.5) |
| Server restart loses pheromone state | Gameplay disruption | Auto-save every 60 s; resume from snapshot on restart (§A.4) |
| Stale food display on clients | Visual mismatch | Accept as cosmetic; food is server-only (§A.9 / R11) |

---

## 14. Verification Checklist

- [ ] `TerrainChunk::Generate()` produces identical output for the same seed
      (unit test: generate two chunks, memcmp).
- [ ] `TerrainWorld::Generate()` produces identical output for the same seed
      (unit test: generate two worlds, compare chunk-by-chunk).
- [ ] Integer-quantised biome classification matches across Debug/Release.
- [ ] CA tick conserves total pheromone (minus evaporation) within floating-
      point epsilon (unit test).
- [ ] Pheromone diffuses correctly across chunk boundaries (ghost-cell
      halo exchange test).
- [ ] All 272 `m_landscape` references compile and behave identically
      (full build, run existing gameplay scenarios).
- [ ] `GetTerrainWorld()` never returns null (legacy and procedural maps).
- [ ] `BuildDelta()` correctly identifies changed cells above threshold
      (unit test).
- [ ] Client renders correct biome colours per `TerrainType`.
- [ ] Pheromone overlay updates only when dirty flag is set (not every frame).
- [ ] Server → client delta bandwidth < 3 KB/tick per chunk under normal
      gameplay.
- [ ] Full sync on subscribe restores pheromone state within one tick.
- [ ] Heartbeat re-sync caps drift to 30 s regardless of packet loss.
- [ ] `SaveSnapshot()` / `LoadSnapshot()` round-trips correctly.
- [ ] All build configurations pass: Debug/Release × Starstrike/NeuronServer.
- [ ] Frame time regression < 0.1 ms on client (profile with 256×256 grid).
- [ ] `location_colony` string present in english.txt.

---

# Appendix A — MMO-Readiness Analysis & Recommendations

The plan above is solid for a small-session game (4–8 players, one
`Location`, one `Server` instance).  Scaling it to an MMO — hundreds to
thousands of concurrent players in a shared persistent world — exposes
seven structural gaps that should be addressed **before** implementation
locks in the single-grid, broadcast-to-all design.

---

## A.1  Critical Issue: Single Monolithic Grid Does Not Scale

### Problem

The plan assumes a single `TerrainGrid` of fixed size (256×256) owned by
one `Landscape` instance inside one `Location`.  An MMO world is typically
orders of magnitude larger (tens of thousands of cells in each dimension).
A flat `65 536 × 16`-byte array works at 256², but at 4096² the cell array
alone is **256 MB**, the scratch buffers add another **256 MB**, and the CA
tick iterates 16M cells — well above the 0.5 ms budget.

### Recommendation: Chunk-based TerrainGrid

Split the world into fixed-size **chunks** (e.g. 64×64 or 128×128 cells).

```
TerrainWorld
 ├── TerrainChunk[0,0]   (128×128 TerrainCells)
 ├── TerrainChunk[1,0]
 ├── TerrainChunk[0,1]
 └── ...
```

| Concept | Current plan | Recommendation |
|---------|-------------|----------------|
| Grid class | `TerrainGrid` (one flat array) | `TerrainWorld` → array of `TerrainChunk*` |
| CA tick | Iterates all cells every tick | Each chunk ticks independently; only **active** chunks tick (see §A.3) |
| Delta sync | One delta buffer for the whole grid | Per-chunk delta buffer; clients subscribe to chunks they can see |
| Memory | Allocated up front for full world | Chunks allocated/freed on demand (streaming) |
| Heightmap coupling | 1:1 with `SurfaceMap2D` | Each chunk maps to a region of the heightmap; chunks near the player are loaded, far ones are dormant |

New structures:

```cpp
// GameLogic/TerrainChunk.h
class TerrainChunk
{
public:
    static constexpr int CHUNK_SIZE = 128; // cells per side

    int m_chunkX, m_chunkZ;                // chunk coordinates in the chunk grid
    TerrainCell m_cells[CHUNK_SIZE * CHUNK_SIZE];

    // Per-chunk scratch for CA diffusion
    float m_phHomeScratch[CHUNK_SIZE * CHUNK_SIZE];
    float m_phFoodScratch[CHUNK_SIZE * CHUNK_SIZE];

    // Per-chunk delta snapshot
    float m_phHomeSnapshot[CHUNK_SIZE * CHUNK_SIZE];
    float m_phFoodSnapshot[CHUNK_SIZE * CHUNK_SIZE];

    bool m_active;   // ticked by server when at least one client is subscribed
    bool m_dirty;    // any pheromone changed since last delta build

    void Generate(int _seed);
    void TickPheromones(float _alpha, float _beta, float _maxPh,
                        const TerrainChunk* _neighborN,
                        const TerrainChunk* _neighborS,
                        const TerrainChunk* _neighborE,
                        const TerrainChunk* _neighborW);

    int  BuildDelta(PhDelta* _out, int _max, float _threshold) const;
    void SnapshotPheromones();
    // ...
};

// GameLogic/TerrainWorld.h
class TerrainWorld
{
public:
    void Generate(int _worldW, int _worldH, int _seed);
    TerrainChunk* GetChunk(int _cx, int _cz);
    void TickActiveChunks(float _alpha, float _beta, float _maxPh);
    // ...
private:
    int m_chunksX, m_chunksZ;
    TerrainChunk** m_chunks;  // flat [m_chunksZ * m_chunksX], nullable for streaming
};
```

**Impact on plan phases**: Phase 1 needs to create `TerrainChunk` +
`TerrainWorld` instead of a single `TerrainGrid`.  The `Landscape` class
holds a `TerrainWorld*` instead of `TerrainGrid*`.  The API surface
(`GetCell`, `GetTerrainType`, `IsPassable`) remains identical — the
`TerrainWorld` just adds a chunk-lookup indirection.

---

## A.2  Critical Issue: Broadcast-to-All Delta Model

### Problem

The current plan calls `server->SendLetter(letter)` for pheromone deltas
with no filtering.  The existing `Server::Advance()` in `server.cpp:400`
puts each letter into `m_history` and then re-sends it to **every client**
(lines 409–433).  In an MMO:

- 1000 clients × 2.4 KB/tick × 10 ticks/s = **24 MB/s** outbound.
- Clients receive data for chunks they can't see — wasted bandwidth and
  free information for cheats.

### Recommendation: Area-of-Interest (AoI) Subscription

Each client tracks which chunks are in its **view radius** (a set of
`(chunkX, chunkZ)` pairs, typically 3×3 to 5×5 chunks around the camera).

```
ClientSession
{
    int m_clientId;
    std::unordered_set<uint32_t> m_subscribedChunks; // packed (cx << 16 | cz)
};
```

**Server flow per CA tick**:

```
for each active chunk:
    if chunk.m_dirty:
        deltas = chunk.BuildDelta(...)
        for each client subscribed to this chunk:
            send deltas to that client only
        chunk.SnapshotPheromones()
```

**Client flow on camera move**:

1. Compute new set of visible chunk coords.
2. Diff against previous set.
3. Send `SubscribeChunks` / `UnsubscribeChunks` messages to server.
4. Server responds with `PheromoneFullSync` for newly subscribed chunks
   (not the entire world — just the chunks the client entered).

**Full sync on connect** becomes: sync only the initial ~9–25 chunks
around spawn.  At 128×128 × 8 bytes × 25 chunks = **3.2 MB** — still
manageable as a burst, and can be compressed or streamed across frames.

### Recommendation: New NetworkUpdate types

```cpp
enum UpdateType
{
    // ...existing...
    ChunkSubscribe,         // Client → Server: subscribe to chunk(s)
    ChunkUnsubscribe,       // Client → Server: unsubscribe from chunk(s)
    ChunkPheromoneDeltas,   // Server → Client: sparse delta for one chunk
    ChunkPheromoneFullSync, // Server → Client: full pheromone state for one chunk
    ChunkFood Update,         // Server → Client: food changes for one chunk
};
```

Each delta message carries a `chunkId` header so the client knows which
chunk to apply it to.  This replaces the flat `m_cellIndex` with a
`(chunkId, localCellIndex)` pair.

---

## A.3  Critical Issue: CA Tick Cost at World Scale

### Problem

A single-threaded pass over all cells at world scale is too expensive.
A 2048×2048 world (256 chunks of 128²) has 4M cells.  Even at 10 ns/cell
the CA tick takes **40 ms** — 80× the budget.

### Recommendation: Tick Only Active Chunks + Parallelise

1. **Activity mask**: Only tick chunks where at least one client is
   subscribed OR an agent is present.  In a typical MMO, < 20% of chunks
   are active at any given time.

2. **Parallel tick**: The existing `Location::Advance()` already uses
   `#pragma omp parallel for` (location.cpp:729).  Apply the same pattern
   to the chunk tick loop:

   ```cpp
   void TerrainWorld::TickActiveChunks(float _alpha, float _beta, float _maxPh)
   {
       // Build a list of active chunk indices
       // (pre-allocated, rebuilt each tick or maintained incrementally)
       int numActive = m_activeChunkCount;

       #pragma omp parallel for schedule(dynamic)
       for (int i = 0; i < numActive; ++i)
       {
           TerrainChunk* chunk = m_activeChunks[i];
           chunk->TickPheromones(_alpha, _beta, _maxPh,
               GetNeighborN(chunk), GetNeighborS(chunk),
               GetNeighborE(chunk), GetNeighborW(chunk));
       }
   }
   ```

3. **Edge exchange**: Chunks share a 1-cell border halo for the 8-neighbor
   stencil.  Before ticking, copy edge pheromone values from neighbors into
   a halo buffer.  This is cheap (128 × 4 bytes × 4 sides = 2 KB per
   chunk) and makes each chunk tick independent — no locking needed during
   parallel iteration.

4. **Budget**: 50 active chunks × 128² cells × 10 ns/cell = **8.2 ms** on
   one core.  With 4 OMP threads on a server box: **~2 ms**.  Acceptable.
   Reduce tick rate to 5 Hz for further savings if needed.

---

## A.4  Moderate Issue: No Pheromone Persistence

### Problem

The plan makes no mention of persisting pheromone state.  In an MMO, the
server process may restart (patches, crashes, scaling).  All pheromone
trails built up over hours of gameplay would be lost.

### Recommendation: Periodic Snapshot to Disk

- Every N seconds (e.g. 60), serialize the pheromone state of all dirty
  chunks to a binary file or database.  Only dirty chunks need writing.
- On server restart, load the snapshot and resume.  Clients reconnecting
  receive a full sync of their visible chunks as normal.
- File format: chunk header (chunkX, chunkZ, cellCount) followed by raw
  float pairs.  128² × 8 bytes = 128 KB per chunk; at 256 dirty chunks
  that's 32 MB — disk write in < 100 ms.

```cpp
// TerrainWorld persistence API
void SaveSnapshot(const char* _filename) const;
void LoadSnapshot(const char* _filename);
```

---

## A.5  Moderate Issue: NetworkUpdate Fixed-Size Byte Stream

### Problem

`NetworkUpdate` uses a fixed `42`-byte byte stream
(`NETWORKUPDATE_BYTESTREAMSIZE`).  A single pheromone delta is 12 bytes,
and a realistic tick produces 50–200 per chunk.  Stuffing these into 42-byte
`NetworkUpdate` objects means either:

- One `NetworkUpdate` per delta → 200 heap allocations per tick per chunk,
  each serialized individually into the `ServerToClientLetter`'s 1024-byte
  stream.  At 200 deltas this overflows.
- Cramming multiple deltas into one `NetworkUpdate` → doesn't fit at
  42 bytes.

### Recommendation: Variable-Length Bulk Message

Do **not** use `NetworkUpdate` for pheromone data.  Instead, add a new
bulk-data path to `ServerToClientLetter`:

```cpp
class ServerToClientLetter
{
public:
    enum LetterType
    {
        // ...existing...
        ChunkPheromoneUpdate,   // NEW: bulk pheromone payload
    };

    // NEW: variable-length bulk payload (for pheromone, full sync, etc.)
    char*  m_bulkData;       // heap-allocated, nullptr if unused
    int    m_bulkDataSize;   // bytes

    // ...existing...
};
```

Alternatively, increase `SERVERTOCLIENTLETTER_BYTESTREAMSIZE` for
pheromone letters, or use a separate reliable channel.  The key point:
**pheromone deltas must bypass the 42-byte `NetworkUpdate` design**.

The plan's §7.2 message format (2-byte header + 12 bytes/delta) is correct,
but it should be packed into a bulk payload rather than a list of
`NetworkUpdate` objects.

---

## A.6  Moderate Issue: Per-Client Delta Snapshot State

### Problem

The plan stores one `m_phHomeSnapshot` / `m_phFoodSnapshot` per grid — the
"last sent" baseline.  With one client this is fine.  With N clients that
may be subscribed to different chunks at different times, a single snapshot
means:

- If client A receives the delta and client B doesn't (different subscription
  set), the snapshot is advanced but B never got the data.
- If you send the same delta to all subscribers and then snapshot, it works
  — but only if all subscribers received the same data in the same tick.

### Recommendation: Two-Level Snapshot

**Option 1 — Server-global snapshot (simpler, recommended for initial
implementation):**

Maintain one snapshot per chunk.  After building deltas, broadcast to ALL
subscribers of that chunk, then advance the snapshot.  Clients that miss a
delta (packet loss) will drift.  Fix with periodic full-sync heartbeats
per chunk (e.g. every 30 s, staggered across chunks to spread bandwidth).

**Option 2 — Per-client snapshot (correct but expensive):**

Each `ClientSession` tracks its own snapshot per subscribed chunk.  Deltas
are built per-client.  This guarantees no drift but costs O(clients × chunks)
memory and CPU for delta computation.  Appropriate only at very large scale
with a reliable transport layer.

**Recommendation**: Use Option 1.  Add a `m_lastFullSyncTick` counter per
chunk.  If `currentTick - m_lastFullSyncTick > HEARTBEAT_INTERVAL`, send a
full sync to all subscribers and reset.  This caps maximum drift to
`HEARTBEAT_INTERVAL` ticks regardless of packet loss.

---

## A.7  Minor Issue: Diffusion Across Terrain Boundaries

### Problem

The CA diffusion algorithm in §6.1 uses `if in bounds` to skip neighbors
outside the grid.  Edge and corner cells have fewer neighbors (5 or 3
instead of 8), which creates a **pheromone sink at world edges** — trails
near the border evaporate faster because diffusion spreads outward into the
void.

In a single small map this is barely noticeable.  In an MMO with a large
contiguous world, chunk boundaries introduce the same artifact if chunks
are ticked independently without edge exchange.

### Recommendation

1. **World edges**: Use reflective boundary (mirror the cell's own value
   for missing neighbors) instead of skipping.  This makes edge cells
   behave identically to interior cells.

2. **Chunk edges**: Before each chunk tick, copy the outermost row/column
   of pheromone values from the neighboring chunk into a 1-cell halo
   buffer.  The stencil reads from the halo for cross-chunk neighbors.
   This is the same "ghost cell" technique used in parallel PDE solvers.

```cpp
// Before ticking chunk at (cx, cz):
CopyEdgeToHalo(chunk, GetChunk(cx, cz-1), North);
CopyEdgeToHalo(chunk, GetChunk(cx, cz+1), South);
CopyEdgeToHalo(chunk, GetChunk(cx-1, cz), West);
CopyEdgeToHalo(chunk, GetChunk(cx+1, cz), East);
// Corners come from diagonal neighbors
```

Cost: 128 × 2 floats × 4 sides = 1 KB per chunk.  Negligible.

---

## A.8  Minor Issue: Deterministic Generation & Floating-Point

### Problem

§5.1 relies on `float` Perlin noise being bit-identical across client and
server.  This is safe **today** because both compile with MSVC `/fp:precise`
on x86-64.  But:

- A future server running Linux (GCC/Clang) may produce different float
  rounding.
- ARM servers (Azure ARM VMs) use different FPU behavior.

Biome classification is static and only happens once at load, so a mismatch
means a client sees "Earth" where the server says "Desert" — broken.

### Recommendation

Use **integer-only classification** for `TerrainType`.  Quantize the Perlin
output to a fixed-point integer (e.g. multiply by 1000 and round), then
classify using integer thresholds.  This is trivially deterministic across
all platforms.

```cpp
int baseNoise1000 = static_cast<int>(SampleNoise(x, z, seed) * 1000.0f + 0.5f);
if (baseNoise1000 > 700) type = TerrainType::Rock;
// ...
```

The `float→int` conversion is the only platform-sensitive step.  Adding
`0.5f` before truncation makes the rounding mode irrelevant for values not
exactly on the boundary.

Alternatively, ship a pre-generated biome map as compressed binary data
alongside the seed — eliminates float concerns entirely and costs < 64 KB
for 256² (1 byte/cell, zlib-compressed).

---

## A.9  Design Issue: `foodAmount` Mutation Without Delta Sync

### Problem

§2.1/F6 says `foodAmount` mutates (agents collect food), but the delta
message in §7.2 only carries `{m_cellIndex, m_phHome, m_phFood}`.
`foodAmount` changes are never transmitted to clients.

If clients need to render food (e.g. visual density of food patches), they
will be stale.  If food is server-only state, clients don't need it — but
this should be explicit.

### Recommendation

Either:

1. **Transmit food changes**: Add `m_foodAmount` to the delta struct (now
   16 bytes per delta instead of 12).  Only include food when it changes
   (most ticks it doesn't — only when an agent collects).

2. **Keep food server-only**: Document that `foodAmount` is authoritative
   server state never transmitted.  Clients render food based on the
   initial seed-generated distribution, updated only on full sync.

Option 2 is simpler and sufficient if food depletion is slow.

---

## A.10  Summary: Updated Architecture for MMO

```
                        ┌─────────────────┐
                        │   TerrainWorld   │  (GameLogic, standalone)
                        │                  │
                        │  TerrainChunk[]  │  128×128 cells each
                        │  activeChunks[]  │  only ticked if subscribed
                        │  Generate(seed)  │  deterministic per-chunk
                        │  TickActive()    │  OMP parallel
                        └────────┬────────┘
                                 │ owned by
                        ┌────────┴────────┐
                        │    Landscape     │  (Starstrike)
                        │  m_heightMap     │  unchanged
                        │  m_normalMap     │  unchanged
                        │  m_terrainWorld* │  replaces m_terrainGrid*
                        └────────┬────────┘
                                 │ owned by
                        ┌────────┴────────┐
                        │    Location      │  (Starstrike)
                        │  m_caAccumulator │
                        │  AdvanceCA()     │  server-only
                        └─────────────────┘

Server:                              Client:
  TickActiveChunks()                   Camera move →
    ↓                                    ChunkSubscribe/Unsubscribe
  BuildDelta per dirty chunk             ↓
    ↓                                  Receive ChunkPheromoneDeltas
  Send to subscribed clients only        ↓
    ↓                                  ApplyDelta per chunk
  SnapshotPheromones per chunk           ↓
                                       Render biome + overlay
```

---

## A.11  Revised Implementation Phases (MMO-Ready)

### Phase 1 — Data layer (chunked)

1. Create `TerrainCell.h`, `TerrainChunk.h/.cpp`, `TerrainWorld.h/.cpp`,
   `PerlinNoise.h/.cpp`.
2. `TerrainChunk` is self-contained: `Generate(seed)`, accessors, halo
   buffers.
3. `TerrainWorld` manages a 2D array of `TerrainChunk*` with lazy
   allocation.
4. Add `TerrainWorld* m_terrainWorld` to `Landscape` (forward-declared).
5. `Landscape::GenerateTerrainGrid(seed)` populates the initial chunks
   around the starting area.
6. Verify build.  Unit test: generate two worlds from same seed, compare
   chunk-by-chunk.

### Phase 2 — CA tick (chunked, parallel)

7. Implement `TerrainChunk::TickPheromones()` with halo edge exchange.
8. Implement `TerrainWorld::TickActiveChunks()` with OMP.
9. Wire into `Location::AdvanceCA()`.
10. Unit test: deposit pheromone at a chunk border, verify diffusion crosses
    into the neighboring chunk correctly.

### Phase 3 — Rendering

11. Same as original Phase 3, but `BuildColourArray` looks up the chunk for
    each vertex's world position.
12. Pheromone overlay: only rebuild colour sub-buffer for chunks whose
    deltas were received this frame.

### Phase 4 — Networking (AoI-aware)

13. Add `ChunkSubscribe`, `ChunkUnsubscribe`, `ChunkPheromoneDeltas`,
    `ChunkPheromoneFullSync` to `NetworkUpdate` (or a new bulk-message
    type that bypasses the 42-byte limit).
14. Server tracks per-client chunk subscriptions.
15. Delta build + send is per-chunk, per-subscriber.
16. Full sync on subscribe (not on global connect).
17. Periodic heartbeat full-sync per chunk to cap drift.

### Phase 5 — Persistence + Gameplay

18. `TerrainWorld::SaveSnapshot()` / `LoadSnapshot()`.
19. ArmyAnt stigmergic navigation (unchanged from original plan).
20. Console commands, tuning.

---

## A.12  Revised Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| Chunk boundary pheromone discontinuity | Visual seams, incorrect agent behavior | Halo/ghost-cell exchange before each tick |
| Per-client subscription tracking memory | O(clients × chunks) | Cap view radius; use bitset per client (256 chunks = 32 bytes/client) |
| Chunk activation thundering herd (mass player movement) | Server load spike | Rate-limit chunk activation; pre-activate chunks ahead of player movement direction |
| Stale food display on clients | Visual mismatch | Accept as cosmetic; fix with periodic food-sync if needed |
| Server restart loses pheromone state | Gameplay disruption | Auto-save every 60 s; resume from snapshot on restart |
| Parallel CA tick non-determinism (OMP thread scheduling) | Divergent pheromone state between server restarts | Pheromone is server-authoritative; determinism not required — clients receive deltas |
| UDP packet loss causes pheromone drift | Visual artifacts | Heartbeat full-sync per chunk every 30 s, staggered |
| `NetworkUpdate` 42-byte limit blocks bulk data | Cannot ship deltas | Use separate bulk-data letter type; bypass `NetworkUpdate` for pheromone traffic |

---

## A.13  Bandwidth Estimates (MMO Scale)

| Scenario | Clients | Active chunks | Deltas/chunk/tick | Tick rate | Bandwidth (server out) |
|----------|---------|---------------|-------------------|-----------|----------------------|
| Small session (current plan) | 4 | 1 (whole grid) | 200 | 10 Hz | 96 KB/s |
| Medium MMO (100 players) | 100 | ~50 | 50 | 5 Hz | 100 × 25 chunks × 50 × 12 B × 5 = **7.5 MB/s** |
| Large MMO (1000 players) | 1000 | ~200 | 50 | 5 Hz | ≫ capacity |

The medium scenario is feasible.  For the large scenario, additional
measures are needed:

1. **Quantize pheromone** to `uint16_t` in deltas (2 bytes instead of 4):
   cuts delta size from 12 to 8 bytes.
2. **Aggregate deltas**: don't send every tick; batch 2–3 ticks of deltas
   and send once per 200–300 ms.
3. **LOD for distant chunks**: clients subscribed to far-away chunks
   receive deltas at 1/4 rate or only significant changes (higher
   threshold).
4. **Run-length encoding**: pheromone changes are spatially coherent
   (diffusion creates blobs).  RLE on the sorted delta list can compress
   2–5×.

---

## A.14  Final Recommendation Checklist

- [x] **Use chunked `TerrainWorld`** instead of flat `TerrainGrid` (§A.1) — incorporated into §3.3–3.4, §4, §10, §11
- [x] **AoI subscription** instead of broadcast-to-all (§A.2) — incorporated into §7.3–7.5, §10.2
- [x] **Parallel chunk ticking** with halo exchange (§A.3) — incorporated into §6.1, §3.3
- [x] **Periodic persistence** to survive server restarts (§A.4) — incorporated into §3.4, §11 Phase 5
- [x] **Bulk message path** for pheromone data (§A.5) — incorporated into §7.3, §10.2
- [x] **Server-global snapshot** with heartbeat re-sync (§A.6) — incorporated into §7.5
- [x] **Reflective boundary + ghost cells** for diffusion (§A.7) — incorporated into §3.3, §6.1
- [x] **Integer-quantized biome classification** (§A.8) — incorporated into §5.1
- [x] **Decide `foodAmount` sync policy** and document it (§A.9) — server-only, documented in §2.1 F6
- [x] **Bandwidth LOD** for distant chunk subscriptions (§A.13) — documented; deferred to post-Phase 4 tuning
