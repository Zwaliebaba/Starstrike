#include "pch.h"
#include "2d_array.h"
#include "binary_stream_readers.h"
#include "bitmap.h"

#include "hi_res_time.h"
#include "math_utils.h"
#include "ogl_extensions.h"
#include "opengl_directx.h"
#include "opengl_directx_internals.h"
#include "UploadRingBuffer.h"
#include "profiler.h"
#include "preferences.h"
#include "resource.h"
#include "GameApp.h"
#include "main.h"
#include "renderer.h"
#include "water.h"
#include "location.h"
#include "level_file.h"

#define LIGHTMAP_TEXTURE_NAME "water_lightmap"

constexpr float waveBrightnessScale = 4.0f;
constexpr float shoreBrighteningFactor = 250.0f;
constexpr float shoreNoiseFactor = 0.25f;

// ****************************************************************************
// Class Water
// ****************************************************************************

Water::Water()
  : m_waterDepthMap(nullptr),
    m_renderWaterEffect(false)
{
  if (!g_context->m_editing)
  {
    Landscape* land = &g_context->m_location->m_landscape;

    GenerateLightMap();

    int detail = g_prefsManager->GetInt("RenderWaterDetail");

    if (detail > 0)
    {
      float worldSize = std::max(g_context->m_location->m_landscape.GetWorldSizeX(), g_context->m_location->m_landscape.GetWorldSizeZ());
      worldSize /= 100.0f;

      m_cellSize = static_cast<float>(detail) * worldSize;

      int alpha = (g_context->m_negativeRenderer ? 0 : 255);

      // Load colour information from a bitmap
      {
        char fullFilename[256];
        snprintf(fullFilename, sizeof(fullFilename), "terrain/%s", g_context->m_location->m_levelFile->m_wavesColourFilename);

        if (Location::ChristmasModEnabled() == 1)
          strncpy(fullFilename, "terrain/waves_earth.bmp", sizeof(fullFilename));
          fullFilename[sizeof(fullFilename) - 1] = '\0';

        BinaryReader* in = Resource::GetBinaryReader(fullFilename);
        BitmapRGBA bmp(in, "bmp");
        m_colourTable.resize(bmp.m_width);
        for (int x = 0; x < bmp.m_width; ++x)
        {
          m_colourTable[x] = bmp.GetPixel(x, 1);
          m_colourTable[x].a = alpha;
        }
        delete in;
      }

      BuildTriangleStrips();

      m_waveTableX.resize((int)((2.0f * land->GetWorldSizeX()) / m_cellSize + 2));
      m_waveTableZ.resize((int)((2.0f * land->GetWorldSizeZ()) / m_cellSize + 2));

      BuildOpenGlState();
    }
  }
}

Water::~Water()
{
  m_renderVerts.Empty();
}

void Water::GenerateLightMap()
{
  double startTime = GetHighResTime();

#define MASK_SIZE 128

  float landSizeX = g_context->m_location->m_landscape.GetWorldSizeX();
  float landSizeZ = g_context->m_location->m_landscape.GetWorldSizeZ();
  float scaleFactorX = 2.0f * landSizeX / static_cast<float>(MASK_SIZE);
  float scaleFactorZ = 2.0f * landSizeZ / static_cast<float>(MASK_SIZE);
  int low = MASK_SIZE / 4;
  int high = (MASK_SIZE / 4) * 3;
  float offX = low * scaleFactorX;
  float offZ = low * scaleFactorZ;

  //
  // Generate basic mask image data

  Array2D<float> landData;
  landData.Initialise(MASK_SIZE, MASK_SIZE, 0.0f);
  landData.SetAll(0.0f);

  for (int z = low; z < high; ++z)
  {
    for (int x = low; x < high; ++x)
    {
      float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue((0.0f + static_cast<float>(x)) * scaleFactorX - offX,
                                                                              (0.0f + static_cast<float>(z)) * scaleFactorZ - offZ);
      if (landHeight > 0.0f)
        landData.PutData(x, z, 1.0f);
    }
  }

  //
  // Horizontal blur

  constexpr int blurSize = 11;
  constexpr int halfBlurSize = 5;
  float m[blurSize] = {0.2f, 0.3f, 0.4f, 0.5f, 0.8f, 1.0f, 0.8f, 0.5f, 0.4f, 0.3f, 0.2f};
  for (int i = 0; i < blurSize; ++i)
    m[i] /= 5.4f;

  Array2D<float> tempData;
  tempData.Initialise(MASK_SIZE, MASK_SIZE, 0.0f);
  tempData.SetAll(0.0f);
  for (int z = 0; z < MASK_SIZE; ++z)
  {
    for (int x = 0; x < MASK_SIZE; ++x)
    {
      float val = landData.GetData(x, z);
      if (NearlyEquals(val, 0.0f))
        continue;

      for (int i = 0; i < blurSize; ++i)
        tempData.AddToData(x + i - halfBlurSize, z, val * m[i]);
    }
  }

  //
  // Vertical blur

  landData.SetAll(0.0f);
  for (int z = 0; z < MASK_SIZE; ++z)
  {
    for (int x = 0; x < MASK_SIZE; ++x)
    {
      float val = tempData.GetData(x, z);
      if (NearlyEquals(val, 0.0f))
        continue;

      for (int i = 0; i < blurSize; ++i)
        landData.AddToData(x, z + i - halfBlurSize, val * m[i]);
    }
  }

  //
  // Generate finished image and upload to openGL

  BitmapRGBA finalImage(MASK_SIZE, MASK_SIZE);
  for (int x = 0; x < MASK_SIZE; ++x)
  {
    for (int z = 0; z < MASK_SIZE; ++z)
    {
      float grayVal = static_cast<float>(landData.GetData(x, z)) * 855.0f;
      if (grayVal > 255.0f)
        grayVal = 255.0f;
      finalImage.PutPixel(x, MASK_SIZE - z - 1, RGBAColour(grayVal, grayVal, grayVal));
    }
  }

  if (Resource::GetBitmap(LIGHTMAP_TEXTURE_NAME) != nullptr)
    Resource::DeleteBitmap(LIGHTMAP_TEXTURE_NAME);

  if (Resource::DoesTextureExist(LIGHTMAP_TEXTURE_NAME))
    Resource::DeleteTexture(LIGHTMAP_TEXTURE_NAME);

  Resource::AddBitmap(LIGHTMAP_TEXTURE_NAME, finalImage);

  //
  // Create the water depth map

  float depthMapCellSize = (landSizeX * 2.0f) / static_cast<float>(finalImage.m_height);
  m_waterDepthMap = new SurfaceMap2D<float>(landSizeX * 2.0f, landSizeZ * 2.0f, -landSizeX / 2.0f, -landSizeZ / 2.0f, depthMapCellSize,
                                            depthMapCellSize, 1.0f);
  if (!g_context->m_editing)
  {
    for (int z = 0; z < finalImage.m_height; ++z)
    {
      for (int x = 0; x < finalImage.m_width; ++x)
      {
        RGBAColour pixel = finalImage.GetPixel(x, z);
        float depth = static_cast<float>(pixel.g) / 255.0f;
        depth = 1.0f - depth;
        m_waterDepthMap->PutData(x, finalImage.m_height - z - 1, depth);
      }
    }
  }

  //
  // Take a low res copy of the water depth map that the flat water renderer
  // can efficiently use to determine where under water polys are needed

  constexpr int flatWaterRatio = 4; // One flat water quad is the same size as 8x8 dynamic water quads
  scaleFactorX *= flatWaterRatio;
  scaleFactorZ *= flatWaterRatio;
  m_flatWaterTiles = new Array2D<bool>(m_waterDepthMap->GetNumColumns() / flatWaterRatio, m_waterDepthMap->GetNumRows() / flatWaterRatio,
                                       false);

  for (int z = 0; z < m_flatWaterTiles->GetNumRows(); ++z)
  {
    for (int x = 0; x < m_flatWaterTiles->GetNumColumns(); ++x)
    {
      int currentVal = 0;
      for (int dz = 0; dz < flatWaterRatio; ++dz)
      {
        for (int dx = 0; dx < flatWaterRatio; ++dx)
        {
          float depth = m_waterDepthMap->GetData(x * flatWaterRatio + dx, z * flatWaterRatio + dz);
          if (depth >= 1.0f) // Very deep
            ++currentVal;
        }
      }

      constexpr int topScore = flatWaterRatio * flatWaterRatio;
      if (currentVal == topScore)
        m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, false);
      else
        m_flatWaterTiles->PutData(x, m_flatWaterTiles->GetNumRows() - z - 1, true);
    }
  }

  double totalTime = GetHighResTime() - startTime;
  DebugTrace("Water lightmap generation took {}ms\n", static_cast<int>(totalTime * 1000));
}

void Water::BuildOpenGlState()
{
}

bool Water::IsVertNeeded(float x, float z)
{
  float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(x, z);
  if (landHeight > 4.0f)
    return false;

  float waterDepth = m_waterDepthMap->GetValue(x, z);
  if (waterDepth > 0.999f)
    return false;

  return true;
}

void Water::BuildTriangleStrips()
{
  m_renderVerts.SetStepDouble();
  m_strips.SetStepDouble();

  const float landSizeX = g_context->m_location->m_landscape.GetWorldSizeX();
  const float landSizeZ = g_context->m_location->m_landscape.GetWorldSizeZ();

  const float lowX = -landSizeX * 0.5f;
  const float lowZ = -landSizeZ * 0.5f;
  const float highX = landSizeX * 1.5f;
  const float highZ = landSizeZ * 1.5f;
  const int maxXb = (highX - lowX) / m_cellSize;
  const int maxZb = (highZ - lowZ) / m_cellSize;

  auto strip = new WaterTriangleStrip;
  strip->m_startRenderVertIndex = 0;
  int degen = 0;
  WaterVertex vertex1, vertex2;

  for (int zb = 0; zb < maxZb; ++zb)
  {
    float fz = lowZ + static_cast<float>(zb) * m_cellSize;

    for (int xb = 0; xb < maxXb; ++xb)
    {
      float fx = lowX + static_cast<float>(xb) * m_cellSize;
      bool needed1 = IsVertNeeded(fx - m_cellSize, fz);
      bool needed2 = IsVertNeeded(fx - m_cellSize, fz + m_cellSize);
      bool needed3 = IsVertNeeded(fx, fz);
      bool needed4 = IsVertNeeded(fx, fz + m_cellSize);
      bool needed5 = IsVertNeeded(fx + m_cellSize, fz);
      bool needed6 = IsVertNeeded(fx + m_cellSize, fz + m_cellSize);

      if (needed1 || needed2 || needed3 || needed4 || needed5 || needed6)
      {
        // Is needed, so add it to the strip
        vertex1.m_pos.Set(fx, 0.0f, fz);
        vertex2.m_pos.Set(fx, 0.0f, fz + m_cellSize);
        if (degen == 1)
        {
          m_renderVerts.PutData(vertex1);
          m_renderVerts.PutData(vertex1);
        }
        degen = 2;
        m_renderVerts.PutData(vertex1);
        m_renderVerts.PutData(vertex2);
      }
      else
      {
        // Not needed, add degenerated joint.
        if (degen == 2)
        {
          m_renderVerts.PutData(vertex2);
          m_renderVerts.PutData(vertex2);
          degen = 1;
        }
      }
    }
  }

  strip->m_numVerts = m_renderVerts.NumUsed();
  m_strips.PutData(strip);

  // Down-size the finished FastDArrays to fit their data tightly
  m_renderVerts.SetSize(m_renderVerts.NumUsed());
  m_strips.SetSize(m_strips.NumUsed());

  // Up-size the empty FastDArrays to be the same size as the vertex array
  m_waterDepths.resize(m_renderVerts.NumUsed());
  m_shoreNoise.resize(m_renderVerts.NumUsed());
  m_gpuDynamicVerts.resize(m_renderVerts.NumUsed());

  // Create other per-vertex arrays
  for (int i = 0; i < m_renderVerts.Size(); ++i)
  {
    const LegacyVector3& pos = m_renderVerts[i].m_pos;
    float depth = m_waterDepthMap->GetValue(pos.x, pos.z);
    m_waterDepths[i] = depth;
    float shoreness = 1.0f - depth;
    float whiteness = shoreness + sfrand(shoreness) * shoreNoiseFactor;
    whiteness *= shoreBrighteningFactor;
    m_shoreNoise[i] = whiteness;
  }

  delete m_waterDepthMap;
  m_waterDepthMap = nullptr;
}

void Water::RenderFlatWaterTiles(float posNorth, float posSouth, float posEast, float posWest, float height, float texNorth1,
                                 float texSouth1, float texEast1, float texWest1, float texNorth2, float texSouth2, float texEast2,
                                 float texWest2, int steps)
{
  float sizeX = posWest - posEast;
  float sizeZ = posSouth - posNorth;
  float posStepX = sizeX / static_cast<float>(steps);
  float posStepZ = sizeZ / static_cast<float>(steps);

  float texSizeX1 = texWest1 - texEast1;
  float texSizeZ1 = texSouth1 - texNorth1;
  float texStepX1 = texSizeX1 / static_cast<float>(steps);
  float texStepZ1 = texSizeZ1 / static_cast<float>(steps);

  float texSizeX2 = texWest2 - texEast2;
  float texSizeZ2 = texSouth2 - texNorth2;
  float texStepX2 = texSizeX2 / static_cast<float>(steps);
  float texStepZ2 = texSizeZ2 / static_cast<float>(steps);

  // Accumulate quads as CustomVertex triangles (6 verts per quad)
  // Read current color from GL state — set by RenderFlatWater before this call
  UINT32 color;
  {
    // Match glColor4ub state: use the currently set GL color.
    // RenderFlatWater calls glColor4ub before us — we read it from the
    // current attributes by constructing the BGRA value directly.
    // We don't have direct access to s_currentAttribs, so we replicate the
    // color value that RenderFlatWater set.
    unsigned char alpha = g_context->m_negativeRenderer ? 0 : 255;
    OpenGLD3D::CustomVertex tmpV{};
    tmpV.r8 = 255;
    tmpV.g8 = 255;
    tmpV.b8 = 255;
    tmpV.a8 = alpha;
    color = tmpV.diffuse;
  }

  std::vector<OpenGLD3D::CustomVertex> verts;
  verts.reserve(static_cast<size_t>(steps) * steps * 6); // worst case

  auto makeVtx = [&](float px, float py, float pz, float t1u, float t1v, float t2u, float t2v) -> OpenGLD3D::CustomVertex {
    OpenGLD3D::CustomVertex v{};
    v.x = px; v.y = py; v.z = pz;
    v.diffuse = color;
    v.u = t1u; v.v = t1v;
    v.u2 = t2u; v.v2 = t2v;
    return v;
  };

  for (int j = 0; j < steps; ++j)
  {
    float pz = posNorth + j * posStepZ;
    float tz1 = texNorth1 + j * texStepZ1;
    float tz2 = texNorth2 + j * texStepZ2;

    for (int i = 0; i < steps; ++i)
    {
      float px = posEast + i * posStepX;

      if (m_flatWaterTiles->GetData(i, j) == false)
        continue;

      float tx1 = texEast1 + i * texStepX1;
      float tx2 = texEast2 + i * texStepX2;

      // Original quad winding: (px+step,pz), (px+step,pz+step), (px,pz+step), (px,pz)
      auto v0 = makeVtx(px + posStepX, height, pz,           tx1 + texStepX1, tz1,               tx2 + texStepX2, tz2);
      auto v1 = makeVtx(px + posStepX, height, pz + posStepZ, tx1 + texStepX1, tz1 + texStepZ1,   tx2 + texStepX2, tz2 + texStepZ2);
      auto v2 = makeVtx(px,            height, pz + posStepZ, tx1,             tz1 + texStepZ1,   tx2,             tz2 + texStepZ2);
      auto v3 = makeVtx(px,            height, pz,            tx1,             tz1,               tx2,             tz2);

      // Decompose quad into 2 triangles (v0,v1,v2) (v0,v2,v3)
      verts.push_back(v0); verts.push_back(v1); verts.push_back(v2);
      verts.push_back(v0); verts.push_back(v2); verts.push_back(v3);
    }
  }

  if (verts.empty())
    return;

  // Upload to ring buffer and issue a single draw call
  const UINT vertexCount = static_cast<UINT>(verts.size());
  const UINT vbSize = vertexCount * sizeof(OpenGLD3D::CustomVertex);

  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(vbSize, sizeof(float));
  memcpy(alloc.cpuPtr, verts.data(), vbSize);

  D3D12_VERTEX_BUFFER_VIEW vbView{};
  vbView.BufferLocation = alloc.gpuAddr;
  vbView.SizeInBytes = vbSize;
  vbView.StrideInBytes = sizeof(OpenGLD3D::CustomVertex);

  OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  auto* cmdList = Graphics::Core::Get().GetCommandList();
  cmdList->IASetVertexBuffers(0, 1, &vbView);
  cmdList->DrawInstanced(vertexCount, 1, 0, 0);
  OpenGLD3D::RecordBatchedDraw(vbSize);
}

void Water::RenderFlatWater()
{
  Landscape* land = &g_context->m_location->m_landscape;

  glDisable(GL_CULL_FACE);
  glEnable(GL_FOG);
  glDisable(GL_BLEND);
  glDepthMask(false);

  if (g_context->m_negativeRenderer)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4ub(255, 255, 255, 0);
  }
  else
    glColor4ub(255, 255, 255, 255);

  char waterFilename[256];
  snprintf(waterFilename, sizeof(waterFilename), "terrain/%s", g_context->m_location->m_levelFile->m_waterColourFilename);

  if (Location::ChristmasModEnabled() == 1)
    strncpy(waterFilename, "terrain/water_icecaps.bmp", sizeof(waterFilename));
    waterFilename[sizeof(waterFilename) - 1] = '\0';

  gglActiveTextureARB(GL_TEXTURE0_ARB);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(waterFilename, true, true));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glEnable(GL_TEXTURE_2D);

  // JAK HACK (DISABLED)
  gglActiveTextureARB(GL_TEXTURE1_ARB);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(LIGHTMAP_TEXTURE_NAME));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
  glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);

  float landSizeX = land->GetWorldSizeX();
  float landSizeZ = land->GetWorldSizeZ();
  float borderX = landSizeX / 2.0f;
  float borderZ = landSizeZ / 2.0f;

  const float timeFactor = g_gameTime / 30.0f;

  RenderFlatWaterTiles(landSizeZ + borderZ, -borderZ, -borderX, landSizeX + borderX, -9.0f, timeFactor, timeFactor + 30.0f, timeFactor,
                       timeFactor + 30.0f, 0.0f, 1.0f, 0.0f, 1.0f, m_flatWaterTiles->GetNumColumns());

  gglActiveTextureARB(GL_TEXTURE1_ARB);
  glDisable(GL_TEXTURE_2D);

  gglActiveTextureARB(GL_TEXTURE0_ARB);
  glDisable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_FOG);
  glDepthMask(true);
}

bool isIdentical(const LegacyVector3& a, const LegacyVector3& b, const LegacyVector3& c)
{
  return a.x == b.x && a.x == c.x && a.z == b.z && a.z == c.z;
}

void Water::UpdateDynamicWater()
{
  constexpr float scaleFactor = 7.0f;

  //
  // Generate lookup tables

  for (int i = 0; i < (int)m_waveTableX.size(); ++i)
  {
    float x = static_cast<float>(i) * m_cellSize;
    float heightForX = sinf(x * 0.01f + g_gameTime * 0.65f) * 1.2f;
    heightForX += sinf(x * 0.03f + g_gameTime * 1.5f) * 0.9f;
    m_waveTableX[i] = heightForX * scaleFactor;
  }
  for (int i = 0; i < (int)m_waveTableZ.size(); ++i)
  {
    float z = static_cast<float>(i) * m_cellSize;
    float heightForZ = sinf(z * 0.02f + g_gameTime * 0.75f) * 0.9f;
    heightForZ += sinf(z * 0.03f + g_gameTime * 1.85f) * 0.65f;
    m_waveTableZ[i] = heightForZ * scaleFactor;
  }

  //
  // Go through all the strips, updating vertex heights and poly colours

  int totalNumVertices = 0;

  for (int i = 0; i < m_strips.Size(); ++i)
  {
    WaterTriangleStrip* strip = m_strips[i];

    float prevHeight1 = 0.0f;
    float prevHeight2 = 0.0f;

    totalNumVertices += strip->m_numVerts;

    //
    // Set through the triangle strip in pairs of vertices

    const int finalVertIndex = strip->m_startRenderVertIndex + strip->m_numVerts - 1;
    for (int j = strip->m_startRenderVertIndex; j < finalVertIndex; ++j)
    {
      WaterVertex* vertex1 = &m_renderVerts[j];
      WaterVertex* vertex2 = &m_renderVerts[j + 1];

      const float landSizeX = g_context->m_location->m_landscape.GetWorldSizeX();
      const float landSizeZ = g_context->m_location->m_landscape.GetWorldSizeZ();
      const float lowX = -landSizeX * 0.5f;
      const float lowZ = -landSizeZ * 0.5f;
      int indexX = static_cast<int>((vertex1->m_pos.x - lowX) / m_cellSize + 0.1f);
      int indexZ = static_cast<int>((vertex1->m_pos.z - lowZ) / m_cellSize + 0.1f);
      DEBUG_ASSERT(indexX < (int)m_waveTableX.size());
      DEBUG_ASSERT(indexZ + 1 < (int)m_waveTableZ.size());

      // Update the height and calc brightness for FIRST vertex of the pair
      vertex1->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ];
      vertex1->m_pos.y *= m_waterDepths[j];
      if (j >= 2 && isIdentical(m_renderVerts[j - 2].m_pos, m_renderVerts[j - 1].m_pos, vertex1->m_pos))
      {
        // end of degenerated joint
        m_renderVerts[j - 2].m_pos.y = vertex1->m_pos.y;
        m_renderVerts[j - 1].m_pos.y = vertex1->m_pos.y;
      }
      float brightness = (prevHeight1 + prevHeight2 + vertex1->m_pos.y) * waveBrightnessScale;
      float shoreness = 1.0f - m_waterDepths[j];
      brightness *= shoreness;
      brightness += m_shoreNoise[j];
      prevHeight1 = vertex1->m_pos.y;

      // Update the height and calc brightness for SECOND vertex of the pair
      ++j;
      vertex2->m_pos.y = m_waveTableX[indexX] + m_waveTableZ[indexZ + 1];
      vertex2->m_pos.y *= m_waterDepths[j];
      float brightness2 = (prevHeight2 + prevHeight1 + vertex2->m_pos.y) * waveBrightnessScale;
      shoreness = 1.0f - m_waterDepths[j];
      brightness2 *= shoreness;
      brightness2 += m_shoreNoise[j];
      prevHeight2 = vertex2->m_pos.y;

      // Now update the colours for the two vertices (and hence triangles), but
      // mix their colours together to reduce the sawtooth effect caused by too
      // much contrast between two triangles in the same quad.
      vertex1->m_col = GetColour(Round(brightness2 * 0.7f + brightness * 0.3f));
      vertex2->m_col = GetColour(Round(brightness * 0.7f + brightness2 * 0.3f));

      // Update vertex normals
      float dx1 = -(m_waveTableX[indexX + 1] - m_waveTableX[indexX - 1]) * m_waterDepths[j - 1];
      float dx2 = -(m_waveTableX[indexX + 1] - m_waveTableX[indexX - 1]) * m_waterDepths[j];
      float dz1 = -(m_waveTableZ[indexZ + 1] - m_waveTableZ[indexZ]) * m_waterDepths[j - 1];
      float dz2 = -(m_waveTableZ[indexZ + 2] - m_waveTableZ[indexZ + 1]) * m_waterDepths[j];
      // realistic, but with artifacts around islands in wild water
      vertex1->m_normal = LegacyVector3(dx1, vertex1->m_pos.y, dz1);
      vertex2->m_normal = LegacyVector3(dx2, vertex2->m_pos.y, dz2);
      // no artifacts around islands in wild water, but much less realistic
      //vertex1->m_normal = LegacyVector3(0,-vertex1->m_pos.y,0);
      //vertex2->m_normal = LegacyVector3(0,-vertex2->m_pos.y,0);

      if (j >= 2 && isIdentical(vertex1->m_pos, vertex2->m_pos, m_renderVerts[j - 2].m_pos))
      {
        // start of degenerated joint
        vertex1->m_pos.y = m_renderVerts[j - 2].m_pos.y;
        vertex2->m_pos.y = m_renderVerts[j - 2].m_pos.y;
      }
    }
  }
}

void Water::RenderDynamicWater()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_FOG);
  glEnable(GL_CULL_FACE);

  // Convert WaterVertex → CustomVertex (positions + colors only, no normals/texcoords)
  const int count = m_renderVerts.NumUsed();
  for (int i = 0; i < count; ++i)
  {
    const WaterVertex& src = m_renderVerts[i];
    OpenGLD3D::CustomVertex& dst = m_gpuDynamicVerts[i];
    dst.x  = src.m_pos.x;
    dst.y  = src.m_pos.y;
    dst.z  = src.m_pos.z;
    dst.nx = 0.0f;
    dst.ny = 0.0f;
    dst.nz = 0.0f;
    // RGBAColour(r,g,b,a) → BGRA diffuse via named union members
    dst.r8 = src.m_col.r;
    dst.g8 = src.m_col.g;
    dst.b8 = src.m_col.b;
    dst.a8 = src.m_col.a;
    dst.u  = 0.0f;
    dst.v  = 0.0f;
    dst.u2 = 0.0f;
    dst.v2 = 0.0f;
  }

  // Upload to ring buffer
  const UINT vertexCount = static_cast<UINT>(count);
  const UINT vbSize = vertexCount * sizeof(OpenGLD3D::CustomVertex);
  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(vbSize, sizeof(float));
  memcpy(alloc.cpuPtr, m_gpuDynamicVerts.data(), vbSize);

  D3D12_VERTEX_BUFFER_VIEW vbView{};
  vbView.BufferLocation = alloc.gpuAddr;
  vbView.SizeInBytes = vbSize;
  vbView.StrideInBytes = sizeof(OpenGLD3D::CustomVertex);

  OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  auto* cmdList = Graphics::Core::Get().GetCommandList();
  cmdList->IASetVertexBuffers(0, 1, &vbView);

  int numStrips = m_strips.Size();
  for (int i = 0; i < numStrips; ++i)
  {
    WaterTriangleStrip* strip = m_strips[i];
    cmdList->DrawInstanced(strip->m_numVerts, 1, strip->m_startRenderVertIndex, 0);
  }

  OpenGLD3D::RecordBatchedDraw(vbSize);

  glDisable(GL_FOG);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);
}

void Water::Render()
{
  m_renderWaterEffect = g_prefsManager->GetInt("RenderPixelShader", 2) == 1;
  if (g_context->m_editing)
  {
    START_PROFILE(g_context->m_profiler, "Render Water");

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/triangleOutline.bmp", true, false));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    Landscape* land = &g_context->m_location->m_landscape;
    glEnable(GL_BLEND);
    glColor4ub(250, 250, 250, 100);
    float size = 100.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0, 0.0f, 0);
    glTexCoord2f(size, 0.0f);
    glVertex3f(0, 0.0f, land->GetWorldSizeZ());
    glTexCoord2f(size, size);
    glVertex3f(land->GetWorldSizeX(), 0.0f, land->GetWorldSizeZ());
    glTexCoord2f(0.0f, size);
    glVertex3f(land->GetWorldSizeX(), 0.0f, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor4ub(250, 250, 250, 30);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0, 0.0f, 0);
    glTexCoord2f(size, 0.0f);
    glVertex3f(0, 0.0f, land->GetWorldSizeZ());
    glTexCoord2f(size, size);
    glVertex3f(land->GetWorldSizeX(), 0.0f, land->GetWorldSizeZ());
    glTexCoord2f(0.0f, size);
    glVertex3f(land->GetWorldSizeX(), 0.0f, 0);
    glEnd();
    glDisable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    END_PROFILE(g_context->m_profiler, "Render Water");
  }
  else
  {
    if (g_prefsManager->GetInt("RenderWaterDetail") > 0)
    {
      //Advance();
      START_PROFILE(g_context->m_profiler, "Render Water");
      RenderFlatWater();
      RenderDynamicWater();
      END_PROFILE(g_context->m_profiler, "Render Water");
    }
    else
    {
      START_PROFILE(g_context->m_profiler, "Render Water");
      RenderFlatWater();
      END_PROFILE(g_context->m_profiler, "Render Water");
    }
  }

  g_context->m_location->SetupFog();
}

void Water::Advance()
{
  if (!g_context->m_editing && g_prefsManager->GetInt("RenderWaterDetail") > 0)
  {
    START_PROFILE(g_context->m_profiler, "Advance Water");
    UpdateDynamicWater();
    END_PROFILE(g_context->m_profiler, "Advance Water");
  }
}
