#include "pch.h"
#include "render_states.h"
#include "debug_utils.h"

RenderStates* g_renderStates = nullptr;

RenderStates::RenderStates()
  : m_currentBlend(BLEND_DISABLED),
    m_currentDepth(DEPTH_ENABLED_WRITE),
    m_currentRaster(RASTER_CULL_BACK)
{
  ZeroMemory(m_blendStates, sizeof(m_blendStates));
  ZeroMemory(m_depthStates, sizeof(m_depthStates));
  ZeroMemory(m_rasterStates, sizeof(m_rasterStates));
}

RenderStates::~RenderStates()
{
  Shutdown();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ID3D11BlendState* CreateBlend(ID3D11Device* device, BOOL enable,
  D3D11_BLEND src, D3D11_BLEND dest)
{
  D3D11_BLEND_DESC bd;
  ZeroMemory(&bd, sizeof(bd));
  bd.RenderTarget[0].BlendEnable    = enable;
  bd.RenderTarget[0].SrcBlend       = src;
  bd.RenderTarget[0].DestBlend      = dest;
  bd.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
  bd.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
  bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  bd.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
  bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  ID3D11BlendState* state = nullptr;
  HRESULT hr = device->CreateBlendState(&bd, &state);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create blend state");
  return state;
}

static ID3D11DepthStencilState* CreateDepth(ID3D11Device* device,
  BOOL depthEnable, D3D11_DEPTH_WRITE_MASK writeMask)
{
  D3D11_DEPTH_STENCIL_DESC dsd;
  ZeroMemory(&dsd, sizeof(dsd));
  dsd.DepthEnable    = depthEnable;
  dsd.DepthWriteMask = writeMask;
  dsd.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
  dsd.StencilEnable  = FALSE;

  ID3D11DepthStencilState* state = nullptr;
  HRESULT hr = device->CreateDepthStencilState(&dsd, &state);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create depth-stencil state");
  return state;
}

static ID3D11RasterizerState* CreateRaster(ID3D11Device* device,
  D3D11_CULL_MODE cullMode)
{
  D3D11_RASTERIZER_DESC rd;
  ZeroMemory(&rd, sizeof(rd));
  rd.FillMode              = D3D11_FILL_SOLID;
  rd.CullMode              = cullMode;
  rd.FrontCounterClockwise = TRUE;
  rd.DepthClipEnable       = TRUE;

  ID3D11RasterizerState* state = nullptr;
  HRESULT hr = device->CreateRasterizerState(&rd, &state);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create rasteriser state");
  return state;
}

// ---------------------------------------------------------------------------
// Initialise / Shutdown
// ---------------------------------------------------------------------------

void RenderStates::Initialise(ID3D11Device* device)
{
  // Blend states
  m_blendStates[BLEND_DISABLED]         = CreateBlend(device, FALSE, D3D11_BLEND_ONE,        D3D11_BLEND_ZERO);
  m_blendStates[BLEND_ALPHA]            = CreateBlend(device, TRUE,  D3D11_BLEND_SRC_ALPHA,  D3D11_BLEND_INV_SRC_ALPHA);
  m_blendStates[BLEND_ADDITIVE]         = CreateBlend(device, TRUE,  D3D11_BLEND_SRC_ALPHA,  D3D11_BLEND_ONE);
  m_blendStates[BLEND_ADDITIVE_PURE]    = CreateBlend(device, TRUE,  D3D11_BLEND_ONE,        D3D11_BLEND_ONE);
  m_blendStates[BLEND_SUBTRACTIVE_COLOR]= CreateBlend(device, TRUE,  D3D11_BLEND_SRC_ALPHA,  D3D11_BLEND_INV_SRC_COLOR);
  m_blendStates[BLEND_MULTIPLICATIVE]   = CreateBlend(device, TRUE,  D3D11_BLEND_ZERO,       D3D11_BLEND_SRC_COLOR);
  m_blendStates[BLEND_DEST_MASK]        = CreateBlend(device, TRUE,  D3D11_BLEND_ZERO,       D3D11_BLEND_INV_SRC_ALPHA);

  // Depth-stencil states
  m_depthStates[DEPTH_ENABLED_WRITE]    = CreateDepth(device, TRUE,  D3D11_DEPTH_WRITE_MASK_ALL);
  m_depthStates[DEPTH_ENABLED_READONLY] = CreateDepth(device, TRUE,  D3D11_DEPTH_WRITE_MASK_ZERO);
  m_depthStates[DEPTH_DISABLED]         = CreateDepth(device, FALSE, D3D11_DEPTH_WRITE_MASK_ZERO);

  // Rasteriser states
  m_rasterStates[RASTER_CULL_BACK]      = CreateRaster(device, D3D11_CULL_BACK);
  m_rasterStates[RASTER_CULL_NONE]      = CreateRaster(device, D3D11_CULL_NONE);

  DebugTrace("RenderStates initialised ({} blend, {} depth, {} raster)\n",
    static_cast<int>(BLEND_COUNT), static_cast<int>(DEPTH_COUNT), static_cast<int>(RASTER_COUNT));
}

void RenderStates::Shutdown()
{
  for (int i = 0; i < BLEND_COUNT; ++i)
    SAFE_RELEASE(m_blendStates[i]);
  for (int i = 0; i < DEPTH_COUNT; ++i)
    SAFE_RELEASE(m_depthStates[i]);
  for (int i = 0; i < RASTER_COUNT; ++i)
    SAFE_RELEASE(m_rasterStates[i]);
}

// ---------------------------------------------------------------------------
// Apply states
// ---------------------------------------------------------------------------

void RenderStates::SetBlendState(ID3D11DeviceContext* ctx, BlendStateId id)
{
  DarwiniaDebugAssert(id >= 0 && id < BLEND_COUNT);
  float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  ctx->OMSetBlendState(m_blendStates[id], blendFactor, 0xFFFFFFFF);
  m_currentBlend = id;
}

void RenderStates::SetDepthState(ID3D11DeviceContext* ctx, DepthStateId id)
{
  DarwiniaDebugAssert(id >= 0 && id < DEPTH_COUNT);
  ctx->OMSetDepthStencilState(m_depthStates[id], 0);
  m_currentDepth = id;
}

void RenderStates::SetRasterState(ID3D11DeviceContext* ctx, RasterStateId id)
{
  DarwiniaDebugAssert(id >= 0 && id < RASTER_COUNT);
  ctx->RSSetState(m_rasterStates[id]);
  m_currentRaster = id;
}
