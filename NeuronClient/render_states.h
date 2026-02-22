#ifndef INCLUDED_RENDER_STATES_H
#define INCLUDED_RENDER_STATES_H

#include <d3d11.h>

// Pre-built blend state identifiers
enum BlendStateId
{
  BLEND_DISABLED,                       // glDisable(GL_BLEND)
  BLEND_ALPHA,                          // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
  BLEND_ADDITIVE,                       // GL_SRC_ALPHA, GL_ONE
  BLEND_ADDITIVE_PURE,                  // GL_ONE, GL_ONE
  BLEND_SUBTRACTIVE_COLOR,              // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR
  BLEND_MULTIPLICATIVE,                 // GL_ZERO, GL_SRC_COLOR
  BLEND_DEST_MASK,                      // GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
  BLEND_COUNT
};

// Pre-built depth-stencil state identifiers
enum DepthStateId
{
  DEPTH_ENABLED_WRITE,                  // glEnable(GL_DEPTH_TEST) + glDepthMask(true)
  DEPTH_ENABLED_READONLY,               // glEnable(GL_DEPTH_TEST) + glDepthMask(false)
  DEPTH_DISABLED,                       // glDisable(GL_DEPTH_TEST)
  DEPTH_COUNT
};

// Pre-built rasteriser state identifiers
enum RasterStateId
{
  RASTER_CULL_BACK,                     // glEnable(GL_CULL_FACE) CCW front
  RASTER_CULL_NONE,                     // glDisable(GL_CULL_FACE)
  RASTER_CULL_FRONT,                    // Cull front faces, render back faces only
  RASTER_COUNT
};

class RenderStates
{
public:
  RenderStates();
  ~RenderStates();

  void Initialise(ID3D11Device* device);
  void Shutdown();

  // Apply a pre-built state to the pipeline
  void SetBlendState(ID3D11DeviceContext* ctx, BlendStateId id);
  void SetDepthState(ID3D11DeviceContext* ctx, DepthStateId id);
  void SetRasterState(ID3D11DeviceContext* ctx, RasterStateId id);

  // Query current state (for save/restore patterns)
  BlendStateId  GetCurrentBlendState()  const { return m_currentBlend; }
  DepthStateId  GetCurrentDepthState()  const { return m_currentDepth; }
  RasterStateId GetCurrentRasterState() const { return m_currentRaster; }

private:
  ID3D11BlendState*        m_blendStates[BLEND_COUNT];
  ID3D11DepthStencilState* m_depthStates[DEPTH_COUNT];
  ID3D11RasterizerState*   m_rasterStates[RASTER_COUNT];

  BlendStateId  m_currentBlend;
  DepthStateId  m_currentDepth;
  RasterStateId m_currentRaster;
};

extern RenderStates* g_renderStates;

#endif
