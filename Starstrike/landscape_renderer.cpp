#include "pch.h"
#include "landscape_renderer.h"
#include "2d_surface_map.h"
#include "GameApp.h"
#include "LegacyVector3.h"
#include "TerrainCell.h"
#include "TerrainWorld.h"
#include "UploadRingBuffer.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "level_file.h"
#include "location.h"
#include "math_utils.h"
#include "opengl_directx.h"
#include "opengl_directx_internals.h"
#include "profiler.h"
#include "resource.h"
#include "rgb_colour.h"
#include "texture_uv.h"

#include "CompiledShaders/LandscapeVS.h"
#include "CompiledShaders/LandscapePS.h"

//*****************************************************************************
// Protected Functions
//*****************************************************************************

void LandscapeRenderer::BuildVertArrayAndTriStrip(const SurfaceMap2D<float>* _heightMap)
{
  m_highest = _heightMap->GetHighestValue();

  const int rows = _heightMap->GetNumColumns();
  const int cols = _heightMap->GetNumRows();

  m_verts.SetStepDouble();
  auto strip = new LandTriangleStrip();
  strip->m_firstVertIndex = 0;
  int degen = 0;
  LandVertex vertex1, vertex2;

  for (int z = 0; z < cols; ++z)
  {
    float fz = static_cast<float>(z) * _heightMap->m_cellSizeY;

    // Break the strip between rows to avoid ghost triangles
    if (degen == 2)
    {
      m_verts.PutData(vertex2);
      m_verts.PutData(vertex2);
      degen = 1;
    }

    for (int x = 0; x < rows; ++x)
    {
      float heighta = _heightMap->GetData(x - 1, z);
      float heightb = _heightMap->GetData(x - 1, z + 1);
      float height1 = _heightMap->GetData(x, z);
      float height2 = _heightMap->GetData(x, z + 1);
      float height3 = _heightMap->GetData(x + 1, z);
      float height4 = _heightMap->GetData(x + 1, z + 1);
      // Is this quad at least partly above water?
      if (heighta > 0.0f || heightb > 0.0f || height1 > 0.0f || height2 > 0.0f || height3 > 0.0f || height4 > 0.0f)
      {
        // Yes it is, add it to the strip
        float fx = static_cast<float>(x) * _heightMap->m_cellSizeX;
        vertex1.m_pos.Set(fx, height1, fz);
        vertex2.m_pos.Set(fx, height2, fz + _heightMap->m_cellSizeY);
        if (vertex1.m_pos.y < 0.3f)
          vertex1.m_pos.y = -10.0f;
        if (vertex2.m_pos.y < 0.3f)
          vertex2.m_pos.y = -10.0f;
        if (degen == 1)
        {
          m_verts.PutData(vertex1);
          m_verts.PutData(vertex1);
        }
        degen = 2;
        m_verts.PutData(vertex1);
        m_verts.PutData(vertex2);
      }
      else
      {
        // No, quad is entirely below water, add degenerated joint.
        if (degen == 2)
        {
          m_verts.PutData(vertex2);
          m_verts.PutData(vertex2);
          degen = 1;
        }
      }
    }
  }

  // end strip
  strip->m_numVerts = m_verts.NumUsed();
  m_strips.PutData(strip);
  m_numTriangles = strip->m_numVerts - 2;
}

void LandscapeRenderer::BuildNormArray()
{
  if (m_verts.Size() <= 0)
    return;

  int nextNormId = 0;

  // Go through all the strips...
  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    m_verts[nextNormId++].m_norm = g_upVector;
    m_verts[nextNormId++].m_norm = g_upVector;
    const int maxJ = strip->m_numVerts - 2;

    // For each vertex in strip
    for (int j = 0; j < maxJ; j += 2)
    {
      const LegacyVector3& v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
      const LegacyVector3& v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
      const LegacyVector3& v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
      const LegacyVector3& v4 = m_verts[strip->m_firstVertIndex + j + 3].m_pos;

      LegacyVector3 north(v1 - v2);
      LegacyVector3 northEast(v3 - v2);
      LegacyVector3 east(v4 - v2);

      LegacyVector3 norm1(northEast ^ north);
      norm1.Normalise();
      LegacyVector3 norm2(east ^ northEast);
      norm2.Normalise();

      m_verts[nextNormId++].m_norm = norm1;
      m_verts[nextNormId++].m_norm = norm2;

      int vertIndex = strip->m_firstVertIndex + j + 2;
      DEBUG_ASSERT(nextNormId - 2 == vertIndex);
    }
  }

  int vertIndex = m_verts.NumUsed();
  DEBUG_ASSERT(nextNormId == vertIndex);
}

void LandscapeRenderer::BuildUVArray(const SurfaceMap2D<float>* _heightMap)
{
  if (m_verts.Size() <= 0)
    return;

  int nextUVId = 0;

  float factorX = 1.0f / _heightMap->m_cellSizeX;
  float factorZ = 1.0f / _heightMap->m_cellSizeY;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    const int numVerts = strip->m_numVerts;
    for (int j = 0; j < numVerts; ++j)
    {
      const LegacyVector3& vert = m_verts[strip->m_firstVertIndex + j].m_pos;
      float u = vert.x * factorX;
      float v = vert.z * factorZ;
      m_verts[nextUVId++].m_uv = TextureUV(u, v);
    }
  }

  DEBUG_ASSERT(nextUVId == m_verts.NumUsed());
}

// _gradient=1 means flat, _gradient=0 means vertical
// x and z are only passed in as a means to get predicatable noise
void LandscapeRenderer::GetLandscapeColour(float _height, float _gradient, unsigned int _x, unsigned int _y, RGBAColour* _colour)
{
  float heightAboveSea = _height;
  float u = powf(1.0f - _gradient, 0.4f);
  float v = 1.0f - heightAboveSea / m_highest;
  darwiniaSeedRandom(_x | _y + darwiniaRandom());
  if (heightAboveSea < 0.0f)
    heightAboveSea = 0.0f;
  v += sfrand(0.45f / (heightAboveSea + 2.0f));

  int x = u * m_landscapeColour->m_width;
  int y = v * m_landscapeColour->m_height;

  if (x < 0)
    x = 0;
  if (x > m_landscapeColour->m_width - 1)
    x = m_landscapeColour->m_width - 1;
  if (y < 0)
    y = 0;
  if (y > m_landscapeColour->m_height - 1)
    y = m_landscapeColour->m_height - 1;

  *_colour = m_landscapeColour->GetPixel(x, y);
}

// Biome-aware colour lookup — selects the ramp based on TerrainType.
void LandscapeRenderer::GetBiomeColour(float _height, float _gradient, unsigned int _x, unsigned int _y, TerrainType _type,
                                       RGBAColour* _colour)
{
  int biomeIdx = static_cast<int>(_type);
  BitmapRGBA* ramp = nullptr;
  if (biomeIdx >= 0 && biomeIdx < BIOME_COLOUR_COUNT)
    ramp = m_biomeColours[biomeIdx];
  if (!ramp)
    ramp = m_landscapeColour;

  float heightAboveSea = _height;
  float u = powf(1.0f - _gradient, 0.4f);
  float v = 1.0f - heightAboveSea / m_highest;
  darwiniaSeedRandom(_x | _y + darwiniaRandom());
  if (heightAboveSea < 0.0f)
    heightAboveSea = 0.0f;
  v += sfrand(0.45f / (heightAboveSea + 2.0f));

  int px = static_cast<int>(u * ramp->m_width);
  int py = static_cast<int>(v * ramp->m_height);

  if (px < 0)
    px = 0;
  if (px > ramp->m_width - 1)
    px = ramp->m_width - 1;
  if (py < 0)
    py = 0;
  if (py > ramp->m_height - 1)
    py = ramp->m_height - 1;

  *_colour = ramp->GetPixel(px, py);
}

void LandscapeRenderer::BuildColourArray()
{
  if (m_verts.Size() <= 0)
    return;

  int nextColId = 0;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    m_verts[nextColId++].m_col.Set(255, 0, 255);
    m_verts[nextColId++].m_col.Set(255, 0, 255);

    const int numVerts = strip->m_numVerts - 2;
    for (int j = 0; j < numVerts; ++j)
    {
      const LegacyVector3& v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
      const LegacyVector3& v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
      const LegacyVector3& v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
      LegacyVector3 center = (v1 + v2 + v3) * 0.33333f;
      const LegacyVector3& norm = m_verts[strip->m_firstVertIndex + j + 2].m_norm;
      RGBAColour col;
      GetLandscapeColour(center.y, norm.y, static_cast<unsigned int>(center.x), static_cast<unsigned int>(center.z), &col);
      m_verts[nextColId++].m_col = col;
    }
  }

  DEBUG_ASSERT(nextColId == m_verts.NumUsed());
}

//*****************************************************************************
// PublicFunctions
//*****************************************************************************

const unsigned LandscapeRenderer::m_posOffset(0);
const unsigned LandscapeRenderer::m_normOffset(sizeof(LegacyVector3));
const unsigned LandscapeRenderer::m_colOffset(sizeof(LegacyVector3) * 2);
const unsigned LandscapeRenderer::m_uvOffset(sizeof(LegacyVector3) * 2 + sizeof(RGBAColour));

LandscapeRenderer::LandscapeRenderer(SurfaceMap2D<float>* _heightMap)
#ifdef PHEROMONE_ACTIVE
  : m_terrainWorld(nullptr),
    m_pheromoneDirty(false)
#endif
{
  std::memset(m_biomeColours, 0, sizeof(m_biomeColours));

  char fullFilname[256];
  snprintf(fullFilname, sizeof(fullFilname), "terrain\\%s", g_context->m_location->m_levelFile->m_landscapeColourFilename);

  if (Location::ChristmasModEnabled() == 1)
    strncpy(fullFilname, "terrain\\landscape_icecaps.bmp", sizeof(fullFilname));
  fullFilname[sizeof(fullFilname) - 1] = '\0';

  BinaryReader* reader = Resource::GetBinaryReader(fullFilname);
  ASSERT_TEXT(reader != nullptr, "Failed to get resource %s", fullFilname);
  m_landscapeColour = new BitmapRGBA(reader, "bmp");
  delete reader;

  // Load per-biome colour ramps.  Missing files fall back to nullptr;
  // GetBiomeColour() will use m_landscapeColour instead.
  static const char* s_biomeFiles[BIOME_COLOUR_COUNT] = {
    "terrain\\landscape_earth.bmp", // Empty  (fallback)
    "terrain\\landscape_earth.bmp", // Earth
    "terrain\\landscape_desert.bmp", // Desert
    "terrain\\landscape_icecaps.bmp", // Ice
    "terrain\\landscape_default.bmp", // Rock   (no dedicated BMP)
    nullptr // Water  (handled by water renderer)
  };

  for (int i = 0; i < BIOME_COLOUR_COUNT; ++i)
  {
    if (s_biomeFiles[i])
    {
      BinaryReader* biomeReader = Resource::GetBinaryReader(s_biomeFiles[i]);
      if (biomeReader)
      {
        m_biomeColours[i] = new BitmapRGBA(biomeReader, "bmp");
        delete biomeReader;
      }
    }
  }

  BuildMeshData(_heightMap);
  InitPipeline();
}

void LandscapeRenderer::InitPipeline()
{
  auto* device = Graphics::Core::Get().GetD3DDevice();

  // Input layout matching CustomVertex (pos, normal, diffuse, uv0, uv1).
  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  // Single opaque PSO using the dedicated landscape shaders.
  // The pixel shader combines base vertex colour + overlay texture in one pass.
  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
  desc.pRootSignature = OpenGLD3D::GetSharedRootSignature();
  desc.VS = {g_pLandscapeVS, sizeof(g_pLandscapeVS)};
  desc.PS = {g_pLandscapePS, sizeof(g_pLandscapePS)};
  desc.InputLayout = {inputLayout, _countof(inputLayout)};
  desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  desc.RasterizerState.FrontCounterClockwise = TRUE;
  desc.RasterizerState.DepthClipEnable = TRUE;
  desc.DepthStencilState.DepthEnable = TRUE;
  desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  desc.DepthStencilState.StencilEnable = FALSE;
  desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
  desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
  desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
  desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
  desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  desc.SampleMask = UINT_MAX;
  desc.SampleDesc.Count = 1;
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc.NumRenderTargets = 1;
  desc.RTVFormats[0] = Graphics::Core::Get().GetBackBufferFormat();
  desc.DSVFormat = Graphics::Core::Get().GetDepthBufferFormat();

  check_hresult(device->CreateGraphicsPipelineState(&desc, IID_GRAPHICS_PPV_ARGS(m_pso)));
}

LandscapeRenderer::~LandscapeRenderer()
{
  m_verts.Empty();
  delete[] m_gpuMirror;
  m_gpuMirror = nullptr;
#ifdef PHEROMONE_ACTIVE
  m_baseColours.Empty();
  delete[] m_cachedCellX;
  m_cachedCellX = nullptr;
  delete[] m_cachedCellZ;
  m_cachedCellZ = nullptr;
#endif
  for (int i = 0; i < BIOME_COLOUR_COUNT; ++i)
  {
    delete m_biomeColours[i];
    m_biomeColours[i] = nullptr;
  }
}

void LandscapeRenderer::BuildMeshData(SurfaceMap2D<float>* _heightMap)
{
  m_gpuMirrorCount = 0;  // Force full mirror rebuild on next upload
  m_mirrorVersion = 0;
#ifdef PHEROMONE_ACTIVE
  m_cachedCellCount = 0; // Invalidate cell cache
#endif

  // Release all frame buffers
  for (int i = 0; i < Graphics::Core::GetMaxBackBufferCount(); ++i)
  {
    m_gpuVertexBuffers[i] = nullptr;
    m_gpuVBViews[i] = {};
    m_gpuBufferSizes[i] = 0;
    m_gpuBufferInVBState[i] = false;
    m_gpuFrameVersions[i] = 0;
  }
  ++m_cpuDataVersion;

  BuildVertArrayAndTriStrip(_heightMap);
  BuildNormArray();
  BuildColourArray();
  BuildUVArray(_heightMap);
}

void LandscapeRenderer::UploadToGPU()
{
  if (m_verts.Size() <= 0)
    return;

  const int frameIdx = static_cast<int>(Graphics::Core::Get().GetCurrentFrameIndex());

  // Already up-to-date for this frame slot?
  if (m_gpuFrameVersions[frameIdx] == m_cpuDataVersion)
    return;

  const int count = m_verts.NumUsed();
  const SIZE_T bufferSize = static_cast<SIZE_T>(count) * sizeof(OpenGLD3D::CustomVertex);

  // Build or update the persistent CPU mirror of GPU vertex data.
  if (m_gpuMirrorCount != count)
  {
    // Vertex count changed — full rebuild (positions, normals, UVs, colours).
    delete[] m_gpuMirror;
    m_gpuMirror = new OpenGLD3D::CustomVertex[count];
    m_gpuMirrorCount = count;
    m_mirrorVersion = 0; // Force full conversion below
  }

  if (m_mirrorVersion != m_cpuDataVersion)
  {
    if (m_mirrorVersion == 0)
    {
      // Full conversion — geometry + colours.
      for (int i = 0; i < count; ++i)
      {
        const LandVertex& src = m_verts[i];
        OpenGLD3D::CustomVertex& dst = m_gpuMirror[i];
        dst.x = src.m_pos.x;
        dst.y = src.m_pos.y;
        dst.z = src.m_pos.z;
        // glDrawArrays does NOT negate normals (unlike glNormal3f_impl) — store as-is
        dst.nx = src.m_norm.x;
        dst.ny = src.m_norm.y;
        dst.nz = src.m_norm.z;
        // RGBAColour(r,g,b,a) → BGRA diffuse via named union members
        dst.r8 = src.m_col.r;
        dst.g8 = src.m_col.g;
        dst.b8 = src.m_col.b;
        dst.a8 = src.m_col.a;
        dst.u = src.m_uv.u;
        dst.v = src.m_uv.v;
        dst.u2 = 0.0f;
        dst.v2 = 0.0f;
      }
    }
    else
    {
      // Colour-only refresh — positions, normals, UVs unchanged in the mirror.
      for (int i = 0; i < count; ++i)
      {
        m_gpuMirror[i].r8 = m_verts[i].m_col.r;
        m_gpuMirror[i].g8 = m_verts[i].m_col.g;
        m_gpuMirror[i].b8 = m_verts[i].m_col.b;
        m_gpuMirror[i].a8 = m_verts[i].m_col.a;
      }
    }
    m_mirrorVersion = m_cpuDataVersion;
  }

  // Allocate or reuse the frame's default-heap vertex buffer.
  // MoveToNextFrame() guarantees the GPU has finished reading this slot's
  // previous buffer before we reach here, so reuse is safe.
  auto* device = Graphics::Core::Get().GetD3DDevice();
  auto* cmdList = Graphics::Core::Get().GetCommandList();

  if (m_gpuBufferSizes[frameIdx] != bufferSize)
  {
    // Size mismatch — allocate a new buffer.
    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    m_gpuVertexBuffers[frameIdx] = nullptr;
    check_hresult(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                  D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                  IID_GRAPHICS_PPV_ARGS(m_gpuVertexBuffers[frameIdx])));
    m_gpuBufferSizes[frameIdx] = bufferSize;
    m_gpuBufferInVBState[frameIdx] = false; // New buffer starts in COMMON
  }

  // Upload mirror to ring buffer and copy to default-heap VB.
  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(bufferSize, sizeof(float));
  memcpy(alloc.cpuPtr, m_gpuMirror, bufferSize);

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_gpuVertexBuffers[frameIdx].get();
  barrier.Transition.StateBefore = m_gpuBufferInVBState[frameIdx]
                                     ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
                                     : D3D12_RESOURCE_STATE_COMMON;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  cmdList->CopyBufferRegion(m_gpuVertexBuffers[frameIdx].get(), 0,
                            Graphics::GetCurrentUploadBuffer().GetResource(), alloc.offset, bufferSize);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  cmdList->ResourceBarrier(1, &barrier);

  m_gpuBufferInVBState[frameIdx] = true;

  m_gpuVBViews[frameIdx].BufferLocation = m_gpuVertexBuffers[frameIdx]->GetGPUVirtualAddress();
  m_gpuVBViews[frameIdx].SizeInBytes = static_cast<UINT>(bufferSize);
  m_gpuVBViews[frameIdx].StrideInBytes = sizeof(OpenGLD3D::CustomVertex);

  m_gpuFrameVersions[frameIdx] = m_cpuDataVersion;

  DebugTrace("LandscapeRenderer: uploaded {} vertices, {} bytes (frame {})\n", count,
             static_cast<unsigned int>(bufferSize), frameIdx);
}

void LandscapeRenderer::DrawStrips(ID3D12GraphicsCommandList* _cmdList) const
{
  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];
    _cmdList->DrawInstanced(strip->m_numVerts, 1, strip->m_firstVertIndex, 0);
  }
}

void LandscapeRenderer::Render()
{
  if (m_verts.Size() <= 0)
    return;

#ifdef PHEROMONE_ACTIVE
  // Pheromone overlay — blend tints into vertex colours when dirty.
  if (m_pheromoneDirty && m_terrainWorld)
  {
    UpdatePheromoneOverlay();
    m_pheromoneDirty = false;
  }
#endif

  // Lazy upload to GPU
  UploadToGPU();

  START_PROFILE(g_context->m_profiler, "Render Landscape");

  auto& core = Graphics::Core::Get();
  auto* cmdList = core.GetCommandList();
  const int frameIdx = static_cast<int>(core.GetCurrentFrameIndex());

  // ---- Scene constants (b0) ----
  OpenGLD3D::EnsureSceneConstantsUploaded();
  cmdList->SetGraphicsRootConstantBufferView(0, OpenGLD3D::GetSceneConstantsGPUAddr());

  // ---- Draw constants (b1) ----
  // The dedicated landscape VS handles lighting directly; we still need
  // matrices, light params, and fog state from the DrawConstants cbuffer.
  OpenGLD3D::DrawConstants dc = OpenGLD3D::BuildDrawConstants();
  dc.LightingEnabled = 1;
  dc.Lights[0].Enabled = 1;
  dc.Lights[1].Enabled = 1;
  dc.ColorMaterialEnabled = 1;
  dc.ColorMaterialMode = 1; // AMBIENT_AND_DIFFUSE
  dc.FogEnabled = 1;

  auto cbAlloc = Graphics::GetCurrentUploadBuffer().Allocate(sizeof(OpenGLD3D::DrawConstants), 256);
  memcpy(cbAlloc.cpuPtr, &dc, sizeof(OpenGLD3D::DrawConstants));
  cmdList->SetGraphicsRootConstantBufferView(1, cbAlloc.gpuAddr);

  // ---- Overlay texture (t0) ----
  if (m_overlayTextureId < 0)
    m_overlayTextureId = Resource::GetTexture("textures/triangleOutline.bmp", true, false);
  cmdList->SetGraphicsRootDescriptorTable(2, OpenGLD3D::GetTextureSRVGPUHandle(m_overlayTextureId));
  cmdList->SetGraphicsRootDescriptorTable(3, OpenGLD3D::GetDefaultTextureSRVGPUHandle());

  // ---- Sampler (param 4) ----
  cmdList->SetGraphicsRootDescriptorTable(4, OpenGLD3D::GetSamplerBaseGPUHandle());

  // ---- Viewport / Scissor ----
  D3D12_VIEWPORT viewport = core.GetScreenViewport();
  cmdList->RSSetViewports(1, &viewport);
  D3D12_RECT scissor = core.GetScissorRect();
  cmdList->RSSetScissorRects(1, &scissor);

  // ---- VB / topology / PSO ----
  cmdList->IASetVertexBuffers(0, 1, &m_gpuVBViews[frameIdx]);
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  cmdList->SetPipelineState(m_pso.get());

  // ---- Single draw — base + overlay combined in pixel shader ----
  DrawStrips(cmdList);
  OpenGLD3D::RecordBatchedDraw(0);

  END_PROFILE(g_context->m_profiler, "Render Landscape");
}

// ============================================================================
// InvalidateGPU — bump the CPU data version so that the next Render()
// call re-uploads from the (modified) CPU-side m_verts.  Per-frame
// buffers are reused (not released) — only the stale version triggers
// a new upload for each frame slot as it comes around.
// ============================================================================

void LandscapeRenderer::InvalidateGPU()
{
  ++m_cpuDataVersion;
}

#ifdef PHEROMONE_ACTIVE
// ============================================================================
// RebuildColours — rewrite all vertex colours using per-biome ramps.
// Called once after Landscape::GenerateTerrainWorld() for procedural maps.
// ============================================================================

void LandscapeRenderer::RebuildColours(const TerrainWorld* _world)
{
  if (!_world || m_verts.Size() <= 0)
    return;

  m_terrainWorld = _world;

  // Need heightmap cell size to map vertex world pos → cell coords.
  SurfaceMap2D<float>* hm = g_context->m_location->m_landscape.m_heightMap;
  if (!hm)
    return;
  const float invCellX = 1.0f / hm->m_cellSizeX;
  const float invCellZ = 1.0f / hm->m_cellSizeY;
  const int worldCellsX = _world->GetWidth();
  const int worldCellsZ = _world->GetHeight();

  int nextColId = 0;

  // Reset and re-allocate base-colour array to match vertices.
  // Empty() + SetSize() gives a sequential freelist so PutData()
  // returns indices 0, 1, 2 … matching nextColId.
  m_baseColours.Empty();
  m_baseColours.SetSize(m_verts.NumUsed());

  // Allocate cell coordinate cache for UpdatePheromoneOverlay.
  const int vertCount = m_verts.NumUsed();
  if (m_cachedCellCount != vertCount)
  {
    delete[] m_cachedCellX;
    delete[] m_cachedCellZ;
    m_cachedCellX = new int[vertCount];
    m_cachedCellZ = new int[vertCount];
    m_cachedCellCount = vertCount;
  }

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    // First two verts have no triangle context — magenta sentinel.
    m_verts[nextColId].m_col.Set(255, 0, 255);
    m_baseColours.PutData(m_verts[nextColId].m_col, nextColId);
    nextColId++;
    m_verts[nextColId].m_col.Set(255, 0, 255);
    m_baseColours.PutData(m_verts[nextColId].m_col, nextColId);
    nextColId++;

    const int numVerts = strip->m_numVerts - 2;
    for (int j = 0; j < numVerts; ++j)
    {
      const LegacyVector3& v1 = m_verts[strip->m_firstVertIndex + j].m_pos;
      const LegacyVector3& v2 = m_verts[strip->m_firstVertIndex + j + 1].m_pos;
      const LegacyVector3& v3 = m_verts[strip->m_firstVertIndex + j + 2].m_pos;
      LegacyVector3 center = (v1 + v2 + v3) * 0.33333f;
      const LegacyVector3& norm = m_verts[strip->m_firstVertIndex + j + 2].m_norm;

      // Map world position to cell coordinate.
      int cx = static_cast<int>(center.x * invCellX);
      int cz = static_cast<int>(center.z * invCellZ);
      if (cx < 0)
        cx = 0;
      if (cx >= worldCellsX)
        cx = worldCellsX - 1;
      if (cz < 0)
        cz = 0;
      if (cz >= worldCellsZ)
        cz = worldCellsZ - 1;

      TerrainType type = _world->GetTerrainType(cx, cz);

      RGBAColour col;
      GetBiomeColour(center.y, norm.y, static_cast<unsigned int>(center.x), static_cast<unsigned int>(center.z), type, &col);

      m_verts[nextColId].m_col = col;
      m_baseColours.PutData(col, nextColId);
      nextColId++;
    }
  }

  DEBUG_ASSERT(nextColId == m_verts.NumUsed());

  // Force GPU re-upload with the new colours.
  InvalidateGPU();
}

// ============================================================================
// Pheromone overlay
// ============================================================================

void LandscapeRenderer::MarkPheromoneDirty() { m_pheromoneDirty = true; }

void LandscapeRenderer::UpdatePheromoneOverlay()
{
  if (!m_terrainWorld || m_baseColours.Size() <= 0)
    return;

  // Require cached cell indices (built by RebuildColours).
  DEBUG_ASSERT(m_cachedCellCount == m_verts.NumUsed());
  if (m_cachedCellCount != m_verts.NumUsed())
    return;

  // Blue = home pheromone, Green = food pheromone.
  static constexpr float INV_MAX_PH = 1.0f / 100.0f;
  static constexpr unsigned char PH_HOME_R = 40;
  static constexpr unsigned char PH_HOME_G = 80;
  static constexpr unsigned char PH_HOME_B = 255;
  static constexpr unsigned char PH_FOOD_R = 40;
  static constexpr unsigned char PH_FOOD_G = 220;
  static constexpr unsigned char PH_FOOD_B = 40;

  // Threshold in raw cell units — cell.m_phHome * INV_MAX_PH must exceed 0.01
  // for visible contribution, i.e. cell.m_phHome > 1.0.
  static constexpr float PH_RAW_THRESHOLD = 1.0f;

  int vertIdx = 0;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    LandTriangleStrip* strip = m_strips[i];

    // First two sentinel verts — copy base colour as-is.
    m_verts[vertIdx].m_col = m_baseColours[vertIdx];
    vertIdx++;
    m_verts[vertIdx].m_col = m_baseColours[vertIdx];
    vertIdx++;

    const int numVerts = strip->m_numVerts - 2;
    for (int j = 0; j < numVerts; ++j, ++vertIdx)
    {
      // Use cached cell coordinates — avoids per-vertex centroid recomputation.
      const TerrainCell& cell = m_terrainWorld->GetCell(m_cachedCellX[vertIdx], m_cachedCellZ[vertIdx]);

      // Fast path: both pheromones negligible — restore base colour directly.
      if (cell.m_phHome <= PH_RAW_THRESHOLD && cell.m_phFood <= PH_RAW_THRESHOLD)
      {
        m_verts[vertIdx].m_col = m_baseColours[vertIdx];
        continue;
      }

      // Start from base (biome) colour.
      RGBAColour col = m_baseColours[vertIdx];

      // Additive blend for home pheromone (blue tint).
      float phHome = cell.m_phHome * INV_MAX_PH;
      if (phHome > 1.0f)
        phHome = 1.0f;
      if (phHome > 0.01f)
      {
        int r = col.r + static_cast<int>(PH_HOME_R * phHome);
        int g = col.g + static_cast<int>(PH_HOME_G * phHome);
        int b = col.b + static_cast<int>(PH_HOME_B * phHome);
        col.r = r > 255 ? 255 : static_cast<unsigned char>(r);
        col.g = g > 255 ? 255 : static_cast<unsigned char>(g);
        col.b = b > 255 ? 255 : static_cast<unsigned char>(b);
      }

      // Additive blend for food pheromone (green tint).
      float phFood = cell.m_phFood * INV_MAX_PH;
      if (phFood > 1.0f)
        phFood = 1.0f;
      if (phFood > 0.01f)
      {
        int r = col.r + static_cast<int>(PH_FOOD_R * phFood);
        int g = col.g + static_cast<int>(PH_FOOD_G * phFood);
        int b = col.b + static_cast<int>(PH_FOOD_B * phFood);
        col.r = r > 255 ? 255 : static_cast<unsigned char>(r);
        col.g = g > 255 ? 255 : static_cast<unsigned char>(g);
        col.b = b > 255 ? 255 : static_cast<unsigned char>(b);
      }

      m_verts[vertIdx].m_col = col;
    }
  }

  // Force GPU re-upload.
  InvalidateGPU();
}
#endif // PHEROMONE_ACTIVE
