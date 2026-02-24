#pragma once

#include "d3d12_backend.h"

namespace OpenGLD3D
{
  // Transitional bridge that owns DX12 resources the OpenGL translation layer
  // needs but Neuron::Graphics::Core does not provide.  Created during the
  // D3D12Backend -> Core migration (see DirecXMig.md).
  //
  // Each subsystem will be migrated here from D3D12Backend in phases, then
  // eventually either absorbed into Core or removed when the legacy OpenGL
  // layer is retired.
  class LegacyBridge
  {
  public:
    static void Init();
    static void Shutdown();

    // Called after Core::Prepare() to bind per-frame state
    // (descriptor heaps, root signature, render targets).
    static void BeginFrame();

    // --- Upload ring buffers (one per back buffer) ---
    static UploadRingBuffer& GetUploadBuffer();
    static void ResetUploadBuffer();

    // --- Shader-visible SRV/CBV heap ---
    static ID3D12DescriptorHeap* GetSRVCBVHeap();
    static D3D12_CPU_DESCRIPTOR_HANDLE AllocateSRVSlot(UINT& outIndex);
    static D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(UINT index);
    static UINT GetSRVDescriptorSize();

    // --- Shader-visible sampler heap ---
    static ID3D12DescriptorHeap* GetSamplerHeap();
    static UINT GetSamplerDescriptorSize();

    // --- PSO cache ---
    static ID3D12PipelineState* GetOrCreatePSO(const PSOKey& key);

    // --- Root signature ---
    static ID3D12RootSignature* GetRootSignature();

    // --- Default 1x1 white texture ---
    static UINT GetDefaultTextureSRVIndex();
  };
}
