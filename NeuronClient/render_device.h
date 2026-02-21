#ifndef INCLUDED_RENDER_DEVICE_H
#define INCLUDED_RENDER_DEVICE_H

#include <d3d11.h>
#include <dxgi.h>

class RenderDevice
{
public:
  RenderDevice();
  ~RenderDevice();

  bool Initialise(HWND hWnd, int width, int height, bool windowed, bool vsync);
  void Shutdown();

  void BeginFrame(float r, float g, float b, float a);
  void EndFrame();

  ID3D11Device*            GetDevice()          { return m_device; }
  ID3D11DeviceContext*     GetContext()          { return m_context; }
  IDXGISwapChain*          GetSwapChain()        { return m_swapChain; }
  ID3D11RenderTargetView*  GetBackBufferRTV()    { return m_backBufferRTV; }
  ID3D11DepthStencilView*  GetDepthStencilView() { return m_depthStencilView; }
  int GetBackBufferWidth()  const { return m_width; }
  int GetBackBufferHeight() const { return m_height; }

  void OnResize(int width, int height);

private:
  void CreateBackBufferViews();
  void ReleaseBackBufferViews();

  ID3D11Device*            m_device;
  ID3D11DeviceContext*     m_context;
  IDXGISwapChain*          m_swapChain;
  ID3D11RenderTargetView*  m_backBufferRTV;
  ID3D11Texture2D*         m_depthStencilBuffer;
  ID3D11DepthStencilView*  m_depthStencilView;
  int  m_width;
  int  m_height;
  bool m_vsync;
};

extern RenderDevice* g_renderDevice;

#endif
