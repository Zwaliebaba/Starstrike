#include "pch.h"
#include "2d_surface_map.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "math_utils.h"
#include "opengl_directx.h"
#include "opengl_directx_internals.h"
#include "UploadRingBuffer.h"
#include "profiler.h"
#include "resource.h"
#include "rgb_colour.h"
#include "texture_uv.h"
#include "LegacyVector3.h"
#include "GameApp.h"
#include "landscape_renderer.h"
#include "location.h"	
#include "level_file.h"

//*****************************************************************************
// Protected Functions
//*****************************************************************************

void LandscapeRenderer::BuildVertArrayAndTriStrip(SurfaceMap2D<float>* _heightMap)
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

void LandscapeRenderer::BuildUVArray(SurfaceMap2D<float>* _heightMap)
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

  if (g_context->m_negativeRenderer)
    _colour->a = 0;
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
{
  char fullFilname[256];
  snprintf(fullFilname, sizeof(fullFilname), "terrain\\%s", g_context->m_location->m_levelFile->m_landscapeColourFilename);

  if (Location::ChristmasModEnabled() == 1)
    strncpy(fullFilname, "terrain\\landscape_icecaps.bmp", sizeof(fullFilname));
    fullFilname[sizeof(fullFilname) - 1] = '\0';

  BinaryReader* reader = Resource::GetBinaryReader(fullFilname);
  ASSERT_TEXT(reader != nullptr, "Failed to get resource %s", fullFilname);
  m_landscapeColour = new BitmapRGBA(reader, "bmp");
  delete reader;

  BuildOpenGlState(_heightMap);
}

LandscapeRenderer::~LandscapeRenderer()
{
  m_verts.Empty();
}

void LandscapeRenderer::BuildOpenGlState(SurfaceMap2D<float>* _heightMap)
{
  BuildVertArrayAndTriStrip(_heightMap);
  BuildNormArray();
  BuildColourArray();
  BuildUVArray(_heightMap);
}

void LandscapeRenderer::UploadToGPU()
{
  if (m_gpuUploaded || m_verts.Size() <= 0)
    return;

  const int count = m_verts.NumUsed();
  const SIZE_T bufferSize = static_cast<SIZE_T>(count) * sizeof(OpenGLD3D::CustomVertex);

  // Convert LandVertex → CustomVertex (done once, matching glDrawArrays behavior)
  std::vector<OpenGLD3D::CustomVertex> vertices(count);
  for (int i = 0; i < count; ++i)
  {
    const LandVertex& src = m_verts[i];
    OpenGLD3D::CustomVertex& dst = vertices[i];
    dst.x  = src.m_pos.x;
    dst.y  = src.m_pos.y;
    dst.z  = src.m_pos.z;
    // glDrawArrays does NOT negate normals (unlike glNormal3f_impl) — store as-is
    dst.nx = src.m_norm.x;
    dst.ny = src.m_norm.y;
    dst.nz = src.m_norm.z;
    // RGBAColour(r,g,b,a) → BGRA diffuse via named union members
    dst.r8 = src.m_col.r;
    dst.g8 = src.m_col.g;
    dst.b8 = src.m_col.b;
    dst.a8 = src.m_col.a;
    dst.u  = src.m_uv.u;
    dst.v  = src.m_uv.v;
    dst.u2 = 0.0f;
    dst.v2 = 0.0f;
  }

  // Create default-heap vertex buffer
  auto* device  = Graphics::Core::Get().GetD3DDevice();
  auto* cmdList = Graphics::Core::Get().GetCommandList();

  D3D12_HEAP_PROPERTIES defaultHeap{};
  defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC bufferDesc{};
  bufferDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufferDesc.Width            = bufferSize;
  bufferDesc.Height           = 1;
  bufferDesc.DepthOrArraySize = 1;
  bufferDesc.MipLevels        = 1;
  bufferDesc.SampleDesc.Count = 1;
  bufferDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  check_hresult(device->CreateCommittedResource(
      &defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_GRAPHICS_PPV_ARGS(m_gpuVertexBuffer)));

  // Upload via ring buffer
  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(bufferSize, sizeof(float));
  memcpy(alloc.cpuPtr, vertices.data(), bufferSize);

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource   = m_gpuVertexBuffer.get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  cmdList->CopyBufferRegion(m_gpuVertexBuffer.get(), 0,
                            Graphics::GetCurrentUploadBuffer().GetResource(),
                            alloc.offset, bufferSize);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  cmdList->ResourceBarrier(1, &barrier);

  m_gpuVBView.BufferLocation = m_gpuVertexBuffer->GetGPUVirtualAddress();
  m_gpuVBView.SizeInBytes    = static_cast<UINT>(bufferSize);
  m_gpuVBView.StrideInBytes  = sizeof(OpenGLD3D::CustomVertex);

  m_gpuUploaded = true;

  DebugTrace("LandscapeRenderer: uploaded {} vertices, {} bytes\n",
             count, static_cast<unsigned int>(bufferSize));
}

void LandscapeRenderer::RenderMainSlow()
{
  // GL state setup — needed for PrepareDrawState PSO selection
  GLfloat materialSpecular[] = {0.0f, 0.0f, 0.0f, 0.0f};
  GLfloat materialDiffuse[] = {1.0f, 1.0f, 1.0f, 0.0f};
  GLfloat materialShininess[] = {100.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHTING);

  if (g_context->m_negativeRenderer)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
  }

  // Native D3D12 draw — replaces glEnableClientState + glVertexPointer + glDrawArrays
  auto* cmdList = Graphics::Core::Get().GetCommandList();
  OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  cmdList->IASetVertexBuffers(0, 1, &m_gpuVBView);

  for (int z = 0; z < m_strips.Size(); ++z)
  {
    LandTriangleStrip* strip = m_strips[z];
    cmdList->DrawInstanced(strip->m_numVerts, 1, strip->m_firstVertIndex, 0);
  }

  OpenGLD3D::RecordBatchedDraw(0); // GPU-resident VB — no per-frame upload

  if (g_context->m_negativeRenderer)
    glDisable(GL_BLEND);

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
}

void LandscapeRenderer::RenderOverlaySlow()
{
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glDepthMask(false);
  glDepthFunc(GL_LEQUAL);

  if (!g_context->m_negativeRenderer)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

  int outlineTextureId = Resource::GetTexture("textures/triangleOutline.bmp", true, false);
  glBindTexture(GL_TEXTURE_2D, outlineTextureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  GLfloat materialSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
  GLfloat materialDiffuse[] = {1.2f, 1.2f, 1.2f, 0.0f};
  GLfloat materialShininess[] = {40.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);

  // Native D3D12 draw — replaces glEnableClientState + glVertexPointer + glDrawArrays
  auto* cmdList = Graphics::Core::Get().GetCommandList();
  OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  cmdList->IASetVertexBuffers(0, 1, &m_gpuVBView);

  for (int z = 0; z < m_strips.Size(); ++z)
  {
    LandTriangleStrip* strip = m_strips[z];
    cmdList->DrawInstanced(strip->m_numVerts, 1, strip->m_firstVertIndex, 0);
  }

  OpenGLD3D::RecordBatchedDraw(0); // GPU-resident VB — no per-frame upload

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDepthMask(true);
  glDepthFunc(GL_LEQUAL);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void LandscapeRenderer::Render()
{
  if (m_verts.Size() <= 0)
    return;

  // Lazy upload to GPU on first render (constructor runs during level load,
  // before the D3D12 command list is available for copy operations)
  UploadToGPU();

  g_context->m_location->SetupFog();
  glEnable(GL_FOG);

  START_PROFILE(g_context->m_profiler, "Render Landscape");
  RenderMainSlow();
  RenderOverlaySlow();
  END_PROFILE(g_context->m_profiler, "Render Landscape");

  glDisable(GL_FOG);
}
