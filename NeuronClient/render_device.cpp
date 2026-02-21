#include "pch.h"
#include "render_device.h"
#include "debug_utils.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

RenderDevice* g_renderDevice = nullptr;

RenderDevice::RenderDevice()
  : m_device(nullptr),
    m_context(nullptr),
    m_swapChain(nullptr),
    m_backBufferRTV(nullptr),
    m_depthStencilBuffer(nullptr),
    m_depthStencilView(nullptr),
    m_width(0),
    m_height(0),
    m_vsync(false)
{
}

RenderDevice::~RenderDevice()
{
  Shutdown();
}

bool RenderDevice::Initialise(HWND hWnd, int width, int height, bool windowed, bool vsync)
{
  m_width = width;
  m_height = height;
  m_vsync = vsync;

  DXGI_SWAP_CHAIN_DESC scd;
  ZeroMemory(&scd, sizeof(scd));
  scd.BufferCount                        = 1;
  scd.BufferDesc.Width                   = width;
  scd.BufferDesc.Height                  = height;
  scd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferDesc.RefreshRate.Numerator   = 0;
  scd.BufferDesc.RefreshRate.Denominator = 1;
  scd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.OutputWindow                       = hWnd;
  scd.SampleDesc.Count                   = 1;
  scd.SampleDesc.Quality                 = 0;
  scd.Windowed                           = windowed ? TRUE : FALSE;
  scd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  D3D_FEATURE_LEVEL featureLevelOut;

  UINT createFlags = 0;
#if defined(_DEBUG)
  createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    createFlags,
    &featureLevel, 1,
    D3D11_SDK_VERSION,
    &scd,
    &m_swapChain,
    &m_device,
    &featureLevelOut,
    &m_context);

  if (FAILED(hr))
  {
    // Retry without debug layer
    createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
    hr = D3D11CreateDeviceAndSwapChain(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      createFlags,
      &featureLevel, 1,
      D3D11_SDK_VERSION,
      &scd,
      &m_swapChain,
      &m_device,
      &featureLevelOut,
      &m_context);
  }

  if (FAILED(hr))
  {
    DebugTrace("D3D11CreateDeviceAndSwapChain failed with HRESULT 0x{:08X}\n", static_cast<unsigned>(hr));
    return false;
  }

  CreateBackBufferViews();

  // Set the viewport
  D3D11_VIEWPORT vp;
  vp.TopLeftX = 0.0f;
  vp.TopLeftY = 0.0f;
  vp.Width    = static_cast<float>(m_width);
  vp.Height   = static_cast<float>(m_height);
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  m_context->RSSetViewports(1, &vp);

  DebugTrace("D3D11 device created: {}x{} {}\n", m_width, m_height, windowed ? "windowed" : "fullscreen");
  return true;
}

void RenderDevice::CreateBackBufferViews()
{
  // Render target view from back buffer
  ID3D11Texture2D* backBuffer = nullptr;
  HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
  DarwiniaDebugAssert(SUCCEEDED(hr));

  hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_backBufferRTV);
  DarwiniaDebugAssert(SUCCEEDED(hr));
  backBuffer->Release();

  // Depth-stencil buffer
  D3D11_TEXTURE2D_DESC dsd;
  ZeroMemory(&dsd, sizeof(dsd));
  dsd.Width              = m_width;
  dsd.Height             = m_height;
  dsd.MipLevels          = 1;
  dsd.ArraySize          = 1;
  dsd.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
  dsd.SampleDesc.Count   = 1;
  dsd.SampleDesc.Quality = 0;
  dsd.Usage              = D3D11_USAGE_DEFAULT;
  dsd.BindFlags          = D3D11_BIND_DEPTH_STENCIL;

  hr = m_device->CreateTexture2D(&dsd, nullptr, &m_depthStencilBuffer);
  DarwiniaDebugAssert(SUCCEEDED(hr));

  hr = m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView);
  DarwiniaDebugAssert(SUCCEEDED(hr));

  // Bind both to the output merger
  m_context->OMSetRenderTargets(1, &m_backBufferRTV, m_depthStencilView);
}

void RenderDevice::ReleaseBackBufferViews()
{
  if (m_context)
    m_context->OMSetRenderTargets(0, nullptr, nullptr);

  SAFE_RELEASE(m_depthStencilView);
  SAFE_RELEASE(m_depthStencilBuffer);
  SAFE_RELEASE(m_backBufferRTV);
}

void RenderDevice::Shutdown()
{
  ReleaseBackBufferViews();
  SAFE_RELEASE(m_swapChain);
  SAFE_RELEASE(m_context);
  SAFE_RELEASE(m_device);
}

void RenderDevice::BeginFrame(float r, float g, float b, float a)
{
  float clearColor[4] = { r, g, b, a };
  m_context->ClearRenderTargetView(m_backBufferRTV, clearColor);
  m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void RenderDevice::EndFrame()
{
  m_swapChain->Present(m_vsync ? 1 : 0, 0);
}

void RenderDevice::OnResize(int width, int height)
{
  if (!m_swapChain || (width == m_width && height == m_height))
    return;

  m_width = width;
  m_height = height;

  ReleaseBackBufferViews();

  HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  DarwiniaDebugAssert(SUCCEEDED(hr));

  CreateBackBufferViews();

  D3D11_VIEWPORT vp;
  vp.TopLeftX = 0.0f;
  vp.TopLeftY = 0.0f;
  vp.Width    = static_cast<float>(m_width);
  vp.Height   = static_cast<float>(m_height);
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  m_context->RSSetViewports(1, &vp);
}
