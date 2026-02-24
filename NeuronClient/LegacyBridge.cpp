#include "pch.h"
#include "LegacyBridge.h"

using namespace OpenGLD3D;

// =============================================================================
// Phase 0 stubs — these are NOT wired into the rendering path yet.
// Each method will be implemented during its respective migration phase
// (see DirecXMig.md for the phased plan).
//
// Calling any of these before implementation is a fatal error.
// =============================================================================

void LegacyBridge::Init()
{
  // Phase 1+: create upload buffers, descriptor heaps, samplers,
  //           root signature, default texture on Core's device.
}

void LegacyBridge::Shutdown()
{
  // Phase 5: release all bridge-owned resources.
}

void LegacyBridge::BeginFrame()
{
  // Phase 5: set descriptor heaps, root signature, render targets
  //          after Core::Prepare().
  Fatal("LegacyBridge::BeginFrame not yet implemented");
}

// --- Upload ring buffers ---

UploadRingBuffer& LegacyBridge::GetUploadBuffer()
{
  Fatal("LegacyBridge::GetUploadBuffer not yet implemented");
  static UploadRingBuffer dummy;
  return dummy;
}

void LegacyBridge::ResetUploadBuffer()
{
  Fatal("LegacyBridge::ResetUploadBuffer not yet implemented");
}

// --- Shader-visible SRV/CBV heap ---

ID3D12DescriptorHeap* LegacyBridge::GetSRVCBVHeap()
{
  Fatal("LegacyBridge::GetSRVCBVHeap not yet implemented");
  return nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE LegacyBridge::AllocateSRVSlot(UINT& outIndex)
{
  Fatal("LegacyBridge::AllocateSRVSlot not yet implemented");
  outIndex = 0;
  return {};
}

D3D12_GPU_DESCRIPTOR_HANDLE LegacyBridge::GetSRVGPUHandle(UINT index)
{
  Fatal("LegacyBridge::GetSRVGPUHandle not yet implemented");
  return {};
}

UINT LegacyBridge::GetSRVDescriptorSize()
{
  Fatal("LegacyBridge::GetSRVDescriptorSize not yet implemented");
  return 0;
}

// --- Shader-visible sampler heap ---

ID3D12DescriptorHeap* LegacyBridge::GetSamplerHeap()
{
  Fatal("LegacyBridge::GetSamplerHeap not yet implemented");
  return nullptr;
}

UINT LegacyBridge::GetSamplerDescriptorSize()
{
  Fatal("LegacyBridge::GetSamplerDescriptorSize not yet implemented");
  return 0;
}

// --- PSO cache ---

ID3D12PipelineState* LegacyBridge::GetOrCreatePSO(const PSOKey& key)
{
  Fatal("LegacyBridge::GetOrCreatePSO not yet implemented");
  return nullptr;
}

// --- Root signature ---

ID3D12RootSignature* LegacyBridge::GetRootSignature()
{
  Fatal("LegacyBridge::GetRootSignature not yet implemented");
  return nullptr;
}

// --- Default texture ---

UINT LegacyBridge::GetDefaultTextureSRVIndex()
{
  Fatal("LegacyBridge::GetDefaultTextureSRVIndex not yet implemented");
  return 0;
}
