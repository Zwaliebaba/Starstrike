#pragma once

#include "2d_surface_map.h"
#include "fast_darray.h"
#include "rgb_colour.h"
#include "texture_uv.h"
#include "LegacyVector3.h"

// Forward declarations — avoid pulling TerrainCell.h into this header.
enum class TerrainType : unsigned char;
class TerrainWorld;

class BitmapRGBA;

namespace OpenGLD3D
{
  struct CustomVertex;
}

class LandVertex
{
  public:
    LegacyVector3 m_pos;
    LegacyVector3 m_norm;
    RGBAColour m_col;
    TextureUV m_uv;
};

//*****************************************************************************
// Class LandTriangleStrip
//*****************************************************************************

class LandTriangleStrip
{
  public:
    int m_firstVertIndex;
    int m_numVerts;

    LandTriangleStrip()
      : m_firstVertIndex(-1),
        m_numVerts(-2) {}
};

//*****************************************************************************
// Class LandscapeRenderer
//*****************************************************************************

class LandscapeRenderer
{
  protected:
    BitmapRGBA* m_landscapeColour;
    float m_highest;

    // Per-biome colour ramps (indexed by TerrainType, §8.2)
    static constexpr int BIOME_COLOUR_COUNT = 6;
    BitmapRGBA* m_biomeColours[BIOME_COLOUR_COUNT];

#ifdef PHEROMONE_ACTIVE
    // Saved base (biome) vertex colours — pheromone overlay blends on top.
    FastDArray<RGBAColour> m_baseColours;

    // Pointer to the terrain world (not owned — set by RebuildColours).
    const TerrainWorld* m_terrainWorld;

    // Dirty flag: set when pheromone deltas arrive; cleared after overlay update.
    bool m_pheromoneDirty;
#endif

    FastDArray<LandVertex> m_verts;

    // Frame-buffered GPU vertex buffers — one per in-flight frame to avoid
    // stalls when the GPU is still reading a previous frame's buffer.
    com_ptr<ID3D12Resource> m_gpuVertexBuffers[Graphics::Core::GetMaxBackBufferCount()];
    D3D12_VERTEX_BUFFER_VIEW m_gpuVBViews[Graphics::Core::GetMaxBackBufferCount()]{};
    SIZE_T m_gpuBufferSizes[Graphics::Core::GetMaxBackBufferCount()]{};
    bool m_gpuBufferInVBState[Graphics::Core::GetMaxBackBufferCount()]{}; // true = VCB state, false = COMMON (new)

    // Version-based dirty tracking — avoids re-uploading when data hasn't changed.
    int m_cpuDataVersion = 1; // Bumped by InvalidateGPU when CPU data changes
    int m_gpuFrameVersions[Graphics::Core::GetMaxBackBufferCount()]{}; // Last uploaded version per frame slot
    int m_mirrorVersion = 0; // Version when mirror was last synced from m_verts

    // Persistent CPU mirror of GPU vertex data — avoids full
    // LandVertex→CustomVertex conversion when only colours change.
    OpenGLD3D::CustomVertex* m_gpuMirror = nullptr;
    int m_gpuMirrorCount = 0;

    #ifdef PHEROMONE_ACTIVE
        // Cached per-vertex
    // reused by UpdatePheromoneOverlay to skip centroid recomputation).
    int* m_cachedCellX = nullptr;
    int* m_cachedCellZ = nullptr;
    int m_cachedCellCount = 0;
#endif

    FastDArray<LandTriangleStrip*> m_strips;

    // Compiled PSOs — created once at init, no runtime PSO cache lookup.
    com_ptr<ID3D12PipelineState> m_mainPSO;         // Opaque base terrain
    com_ptr<ID3D12PipelineState> m_overlayPSO;      // Triangle-outline overlay (additive blend)

    void BuildVertArrayAndTriStrip(const SurfaceMap2D<float>* _heightMap);
    void BuildNormArray();
    void BuildUVArray(const SurfaceMap2D<float>* _heightMap);
    void GetLandscapeColour(float _height, float _gradient, unsigned int _x, unsigned int _y, RGBAColour* _colour);
    void GetBiomeColour(float _height, float _gradient, unsigned int _x, unsigned int _y, TerrainType _type, RGBAColour* _colour);
    void BuildColourArray();

    void InitPipeline();
    void InvalidateGPU();
    void UploadToGPU();
    void RenderMainPass(ID3D12GraphicsCommandList* _cmdList);
    void RenderOverlayPass(ID3D12GraphicsCommandList* _cmdList);
    void DrawStrips(ID3D12GraphicsCommandList* _cmdList) const;

    int m_overlayTextureId = -1;

  public:
    static const unsigned int m_posOffset;
    static const unsigned int m_normOffset;
    static const unsigned int m_colOffset;
    static const unsigned int m_uvOffset;

    int m_numTriangles;

    LandscapeRenderer(SurfaceMap2D<float>* _heightMap);
    ~LandscapeRenderer();

    void BuildMeshData(SurfaceMap2D<float>* _heightMap);

    void Initialise();

    void Render();

    #ifdef PHEROMONE_ACTIVE
        // Rebuild all vertex
    // Called once after Landscape::GenerateTerrainWorld() for procedural maps.
    void RebuildColours(const TerrainWorld* _world);

    // Called by client networking when pheromone deltas arrive.
    void MarkPheromoneDirty();

    // Recompute pheromone overlay tint on all vertices from m_baseColours.
    void UpdatePheromoneOverlay();
#endif
};
