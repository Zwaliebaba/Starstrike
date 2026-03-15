#pragma once

#include "DeviceNotify.h"
#include "DescriptorHeap.h"
#include "ResourceStateTracker.h"

namespace Neuron::Graphics
{
  // Controls all the DirectX device resources.
  class Core
  {
    public:
      static constexpr unsigned int ALLOW_TEARING = 0x1;
      static constexpr unsigned int ENABLE_HDR = 0x2;

      static Core& Get();

      // Device notification callback for game-layer resource management.
      void RegisterDeviceNotify(Neuron::IDeviceNotify* deviceNotify) noexcept { m_deviceNotify = deviceNotify; }

      Core(const Core&) = delete;
      Core& operator=(const Core&) = delete;
      Core(Core&&) = delete;
      Core& operator=(Core&&) = delete;

      void Startup(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                   UINT backBufferCount = 2, D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_12_0,
                   unsigned int flags = 0) noexcept(false);
      void Shutdown();

      void CreateDeviceResources();
      void ReleaseDeviceResources();

      void CreateWindowSizeDependentResources();
      void ReleaseWindowSizeDependentResources();

      void SetWindow(HWND window, int width, int height) noexcept;
      bool WindowSizeChanged(int width, int height);
      void HandleDeviceLost();
      void Prepare();
      void Present();
      void WaitForGpu() noexcept;
      void ResetCommandAllocatorAndCommandlist();
      void ExecuteCommandList(bool force = false);

      // Device Accessors.
      Windows::Foundation::Size GetOutputSize() const noexcept { return m_outputSize; }
      Windows::Foundation::Size GetScreenSize();

      // Direct3D Accessors.
      auto GetD3DDevice() noexcept { return m_d3dDevice.get(); }
      auto GetSwapChain() noexcept { return m_swapChain.get(); }
      auto GetDXGIFactory() noexcept { return m_dxgiFactory.get(); }
      HWND GetWindow() noexcept { return m_window; }
      D3D_FEATURE_LEVEL GetDeviceFeatureLevel() noexcept { return m_d3dFeatureLevel; }
      GpuResource& GetRenderTarget() noexcept { return m_renderTargets[m_backBufferIndex]; }
      GpuResource* GetRenderTargets() { return m_renderTargets.data(); }
      GpuResource& GetDepthStencil() noexcept { return m_depthStencil; }
      ID3D12CommandQueue* GetCommandQueue() noexcept { return m_commandQueue.get(); }
      ID3D12CommandAllocator* GetCommandAllocator() noexcept { return m_commandAllocators[m_backBufferIndex].get(); }
      auto GetCommandList() noexcept { return m_commandList.get(); }
      DXGI_FORMAT GetBackBufferFormat() noexcept { return m_backBufferFormat; }
      DXGI_FORMAT GetDepthBufferFormat() noexcept { return m_depthBufferFormat; }
      D3D12_VIEWPORT GetScreenViewport() noexcept { return m_screenViewport; }
      D3D12_RECT GetScissorRect() noexcept { return m_scissorRect; }
      UINT GetCurrentFrameIndex() noexcept { return m_backBufferIndex; }
      UINT GetBackBufferCount() noexcept { return m_backBufferCount; }
      DXGI_COLOR_SPACE_TYPE GetColorSpace() noexcept { return m_colorSpace; }
      unsigned int GetDeviceOptions() noexcept { return m_options; }

      auto* GetGpuResourceStateTracker() { return &sm_gpuResourceStateTracker; }

      D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() noexcept { return m_rtvDescriptors[m_backBufferIndex]; }
      D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() noexcept { return m_dsvDescriptor; }

      DescriptorHandle AllocateDescriptor(const D3D12_DESCRIPTOR_HEAP_TYPE _type, const UINT _count = 1, bool _forceCpu = false)
      {
        return DescriptorAllocator::Allocate(_type, _count, _forceCpu);
      }

      void UpdateColorSpace();

      // Occlusion state — when fully occluded, skip rendering to save GPU.
      bool IsOccluded() const noexcept { return m_isOccluded; }
      void CheckForOcclusion();

    private:
      Core() = default;
      ~Core() = default;

      void MoveToNextFrame();
      void GetAdapter(IDXGIAdapter1** ppAdapter);

      static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

      UINT m_backBufferIndex{};

      // Direct3D objects.
      com_ptr<ID3D12Device10> m_d3dDevice;
      com_ptr<ID3D12GraphicsCommandList7> m_commandList;
      com_ptr<ID3D12CommandQueue> m_commandQueue;
      com_ptr<ID3D12CommandAllocator> m_commandAllocators[MAX_BACK_BUFFER_COUNT];
      ResourceStateTracker sm_gpuResourceStateTracker;
      bool m_openCommandList = false;

      // Swap chain objects.
      com_ptr<IDXGIFactory7> m_dxgiFactory;
      com_ptr<IDXGISwapChain3> m_swapChain;
      std::array<GpuResource, MAX_BACK_BUFFER_COUNT> m_renderTargets;
      std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MAX_BACK_BUFFER_COUNT> m_rtvDescriptors{};
      GpuResource m_depthStencil;
      D3D12_CPU_DESCRIPTOR_HANDLE m_dsvDescriptor{};

      // Presentation fence objects.
      com_ptr<ID3D12Fence> m_fence;
      UINT64 m_fenceValues[MAX_BACK_BUFFER_COUNT]{};
      handle m_fenceEvent;

      // Direct3D rendering objects.
      D3D12_VIEWPORT m_screenViewport{};
      D3D12_RECT m_scissorRect{};

      // Direct3D properties.
      DXGI_FORMAT m_backBufferFormat{};
      DXGI_FORMAT m_depthBufferFormat{};
      UINT m_backBufferCount{};
      D3D_FEATURE_LEVEL m_d3dMinFeatureLevel{};

      // Cached device properties.
      HWND m_window{};
      D3D_FEATURE_LEVEL m_d3dFeatureLevel{D3D_FEATURE_LEVEL_12_0};
      DWORD m_dxgiFactoryFlags{};
      Windows::Foundation::Size m_outputSize = {};

      // HDR Support
      DXGI_COLOR_SPACE_TYPE m_colorSpace{DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709};

      // Core options (see flags above)
      unsigned int m_options{};

      // Device notification callback.
      Neuron::IDeviceNotify* m_deviceNotify{};

      // Occlusion tracking.
      bool m_isOccluded{};
  };
}
