#include "pch.h"
#include "d3d12_backend.h"

#include "CompiledShaders/VertexShader.h"
#include "CompiledShaders/PixelShader.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;
using namespace OpenGLD3D;

D3D12Backend OpenGLD3D::g_backend;

// ---- Helpers ----

// Map colour-manipulating blend factors to their alpha equivalents.
// D3D12 forbids SRC_COLOR / INV_SRC_COLOR / DEST_COLOR / INV_DEST_COLOR
// in SrcBlendAlpha / DestBlendAlpha.
static D3D12_BLEND ToAlphaBlend(D3D12_BLEND blend)
{
  switch (blend)
  {
  case D3D12_BLEND_SRC_COLOR:      return D3D12_BLEND_SRC_ALPHA;
  case D3D12_BLEND_INV_SRC_COLOR:  return D3D12_BLEND_INV_SRC_ALPHA;
  case D3D12_BLEND_DEST_COLOR:     return D3D12_BLEND_DEST_ALPHA;
  case D3D12_BLEND_INV_DEST_COLOR: return D3D12_BLEND_INV_DEST_ALPHA;
  default:                         return blend;
  }
}

static void ThrowIfFailed(HRESULT hr, const char* msg = "D3D12 call failed")
{
  if (FAILED(hr))
    Fatal("{} (HRESULT 0x{:08X})", msg, static_cast<unsigned>(hr));
}

// ---- UploadRingBuffer ----

void UploadRingBuffer::Init(ID3D12Device* device, SIZE_T size)
{
  m_size = size;
  m_offset = 0;

  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ThrowIfFailed(device->CreateCommittedResource(
                  &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_resource)),
                "Failed to create upload ring buffer");

  D3D12_RANGE readRange = {0, 0};
  ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_cpuBase)), "Failed to map upload ring buffer");

  m_gpuBase = m_resource->GetGPUVirtualAddress();
}

void UploadRingBuffer::Reset() { m_offset = 0; }

UploadRingBuffer::Allocation UploadRingBuffer::Allocate(SIZE_T sizeBytes, SIZE_T alignment)
{
  // Align offset
  m_offset = (m_offset + alignment - 1) & ~(alignment - 1);

  // Wrap if we've exceeded the buffer
  if (m_offset + sizeBytes > m_size)
    m_offset = 0;

  Allocation alloc;
  alloc.cpuPtr = m_cpuBase + m_offset;
  alloc.gpuAddr = m_gpuBase + m_offset;
  alloc.offset = m_offset;

  m_offset += sizeBytes;
  return alloc;
}

// ---- D3D12Backend ----

bool D3D12Backend::Init(HWND hWnd, bool windowed, int width, int height, int colourDepth, int zDepth, bool waitVRT)
{
  m_width = width;
  m_height = height;
  m_vsync = waitVRT;

  CreateDevice();
  CreateCommandQueue();
  CreateSwapChain(hWnd, windowed, width, height, waitVRT);
  CreateDescriptorHeaps();
  CreateRTVAndDSV(width, height, zDepth);
  CreateRootSignature();
  CreateUploadBuffer();
  CreateConstantBuffer();

  // Create command allocators and command list before CreateDefaultTexture,
  // which records CopyTextureRegion / ResourceBarrier on m_commandList.
  for (UINT i = 0; i < FRAME_COUNT; i++)
  {
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])),
                  "Failed to create command allocator");
  }

  ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr,
                                            IID_PPV_ARGS(&m_commandList)), "Failed to create command list");

  CreateDefaultTexture();
  CreateSamplers();

  // Begin first frame
  BeginFrame();

  return true;
}

void D3D12Backend::Shutdown()
{
  WaitForGpu();

  if (m_fenceEvent)
  {
    CloseHandle(m_fenceEvent);
    m_fenceEvent = nullptr;
  }
}

void D3D12Backend::CreateDevice()
{
#ifdef _DEBUG
  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    debugController->EnableDebugLayer();
#endif

  ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)), "Failed to create DXGI factory");

  // Try to find a hardware adapter
  ComPtr<IDXGIAdapter1> adapter;
  for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++)
  {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      continue;

    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
      break;
  }

  if (!m_device)
  {
    // Fallback to WARP
    ComPtr<IDXGIAdapter> warpAdapter;
    m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
    ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)), "Failed to create D3D12 device");
  }
}

void D3D12Backend::CreateCommandQueue()
{
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)), "Failed to create command queue");

  // Create fence
  ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Failed to create fence");
  m_fenceValues[m_frameIndex] = 1;
  m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  ASSERT(m_fenceEvent != nullptr);
}

void D3D12Backend::CreateSwapChain(HWND hWnd, bool windowed, int width, int height, bool waitVRT)
{
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = FRAME_COUNT;
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;

  ComPtr<IDXGISwapChain1> swapChain;
  ThrowIfFailed(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain),
                "Failed to create swap chain");

  // Disable Alt+Enter fullscreen toggle
  m_factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

  ThrowIfFailed(swapChain.As(&m_swapChain), "Failed to get IDXGISwapChain3");
  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12Backend::CreateDescriptorHeaps()
{
  // RTV heap
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = FRAME_COUNT;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)), "Failed to create RTV heap");
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  }

  // DSV heap
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_dsvHeap)), "Failed to create DSV heap");
  }

  // SRV/CBV heap (shader-visible)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = MAX_SRV_DESCRIPTORS;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvCbvHeap)), "Failed to create SRV/CBV heap");
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srvNextFreeSlot = 0;
  }

  // Sampler heap (shader-visible)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = NUM_STATIC_SAMPLERS + 16; // extra room
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_samplerHeap)), "Failed to create sampler heap");
    m_samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  }
}

void D3D12Backend::CreateRTVAndDSV(int width, int height, int zDepth)
{
  // Create RTVs for each back buffer
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT i = 0; i < FRAME_COUNT; i++)
  {
    ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])), "Failed to get swap chain buffer");
    m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
    rtvHandle.ptr += m_rtvDescriptorSize;
  }

  // Create depth stencil buffer
  if (zDepth > 0)
  {
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (zDepth == 16)
      dsvFormat = DXGI_FORMAT_D16_UNORM;
    else
      if (zDepth == 32)
        dsvFormat = DXGI_FORMAT_D32_FLOAT;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = dsvFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = dsvFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                    &clearValue, IID_PPV_ARGS(&m_depthStencilBuffer)),
                  "Failed to create depth stencil buffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = dsvFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
  }
}

void D3D12Backend::CreateRootSignature()
{
  // Root parameter 0: CBV (constant buffer at b0)
  // Root parameter 1: Descriptor table for SRV (t0) — texture 0
  // Root parameter 2: Descriptor table for SRV (t1) — texture 1
  // Root parameter 3: Descriptor table for samplers

  D3D12_ROOT_PARAMETER rootParams[4] = {};

  // Param 0: CBV
  rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParams[0].Descriptor.ShaderRegister = 0;
  rootParams[0].Descriptor.RegisterSpace = 0;
  rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  // Param 1: SRV table (t0)
  D3D12_DESCRIPTOR_RANGE srvRange0 = {};
  srvRange0.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  srvRange0.NumDescriptors = 1;
  srvRange0.BaseShaderRegister = 0;
  rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
  rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange0;
  rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  // Param 2: SRV table (t1)
  D3D12_DESCRIPTOR_RANGE srvRange1 = {};
  srvRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  srvRange1.NumDescriptors = 1;
  srvRange1.BaseShaderRegister = 1;
  rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
  rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange1;
  rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  // Param 3: Sampler table
  D3D12_DESCRIPTOR_RANGE samplerRange = {};
  samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
  samplerRange.NumDescriptors = 2; // s0 and s1
  samplerRange.BaseShaderRegister = 0;
  rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
  rootParams[3].DescriptorTable.pDescriptorRanges = &samplerRange;
  rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
  rootSigDesc.NumParameters = _countof(rootParams);
  rootSigDesc.pParameters = rootParams;
  rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;
  HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  if (FAILED(hr))
  {
    if (error)
      OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
    ThrowIfFailed(hr, "Failed to serialize root signature");
  }

  ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
                "Failed to create root signature");
}

void D3D12Backend::CreateUploadBuffer() { m_uploadBuffer.Init(m_device.Get(), UPLOAD_BUFFER_SIZE); }

void D3D12Backend::CreateConstantBuffer()
{
  // Create an upload-heap buffer for constant data (per-frame)
  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  UINT cbSize = CBV_SIZE_ALIGNED * FRAME_COUNT;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = cbSize;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(&m_constantBuffer)), "Failed to create constant buffer");

  D3D12_RANGE readRange = {0, 0};
  ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferCPU)), "Failed to map constant buffer");

  m_constantBufferGPU = m_constantBuffer->GetGPUVirtualAddress();
}

void D3D12Backend::CreateDefaultTexture()
{
  // Create a 1x1 white texture for use when texturing is disabled
  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC texDesc = {};
  texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texDesc.Width = 1;
  texDesc.Height = 1;
  texDesc.DepthOrArraySize = 1;
  texDesc.MipLevels = 1;
  texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  texDesc.SampleDesc.Count = 1;

  ThrowIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&m_defaultTexture)), "Failed to create default texture");

  // Upload the white pixel
  UINT64 uploadSize = 0;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
  m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &uploadSize);

  auto alloc = m_uploadBuffer.Allocate(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  auto dest = static_cast<UINT8*>(alloc.cpuPtr);
  // RGBA white
  dest[0] = 255;
  dest[1] = 255;
  dest[2] = 255;
  dest[3] = 255;

  D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
  srcLoc.pResource = m_uploadBuffer.GetResource();
  srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  srcLoc.PlacedFootprint = footprint;
  srcLoc.PlacedFootprint.Offset = alloc.offset;

  D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
  dstLoc.pResource = m_defaultTexture.Get();
  dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dstLoc.SubresourceIndex = 0;

  m_commandList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

  // Transition to shader resource
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_defaultTexture.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  m_commandList->ResourceBarrier(1, &barrier);

  // Create SRV
  UINT srvIndex;
  D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = AllocateSRVSlot(srvIndex);
  m_defaultTextureSRVIndex = srvIndex;

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Texture2D.MipLevels = 1;
  m_device->CreateShaderResourceView(m_defaultTexture.Get(), &srvDesc, srvHandle);
}

void D3D12Backend::CreateSamplers()
{
  // Create standard sampler combinations
  struct SamplerConfig
  {
    D3D12_FILTER filter;
    D3D12_TEXTURE_ADDRESS_MODE addressU;
    D3D12_TEXTURE_ADDRESS_MODE addressV;
  };

  SamplerConfig configs[NUM_STATIC_SAMPLERS] = {
    {D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP}, // 0: linear/wrap/mipLinear
    {D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP}, // 1: point/wrap
    {D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP}, // 2: linear/clamp
    {D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP}, // 3: point/clamp
    {D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP}, // 4: linear/wrap/mipPoint
    {D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP}, // 5: linear/clamp/mipPoint
    {D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP}, // 6: point/wrap/mipLinear
    {D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP}, // 7: point/clamp/mipLinear
  };

  D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();

  for (UINT i = 0; i < NUM_STATIC_SAMPLERS; i++)
  {
    D3D12_SAMPLER_DESC desc = {};
    desc.Filter = configs[i].filter;
    desc.AddressU = configs[i].addressU;
    desc.AddressV = configs[i].addressV;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    m_device->CreateSampler(&desc, samplerHandle);
    samplerHandle.ptr += m_samplerDescriptorSize;
  }
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Backend::AllocateSRVSlot(UINT& outIndex)
{
  ASSERT_TEXT(m_srvNextFreeSlot < MAX_SRV_DESCRIPTORS, L"SRV descriptor heap full");
  outIndex = m_srvNextFreeSlot++;
  D3D12_CPU_DESCRIPTOR_HANDLE handle = m_srvCbvHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += outIndex * m_srvDescriptorSize;
  return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Backend::GetSRVGPUHandle(UINT index) const
{
  D3D12_GPU_DESCRIPTOR_HANDLE handle = m_srvCbvHeap->GetGPUDescriptorHandleForHeapStart();
  handle.ptr += index * m_srvDescriptorSize;
  return handle;
}

void D3D12Backend::UploadConstants(const PerFrameConstants& constants)
{
  UINT8* dest = m_constantBufferCPU + m_frameIndex * CBV_SIZE_ALIGNED;
  memcpy(dest, &constants, sizeof(PerFrameConstants));
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Backend::GetConstantBufferGPUAddress() const
{
  return m_constantBufferGPU + m_frameIndex * CBV_SIZE_ALIGNED;
}

ID3D12PipelineState* D3D12Backend::GetOrCreatePSO(const PSOKey& key)
{
  auto it = m_psoCache.find(key);
  if (it != m_psoCache.end())
    return it->second.Get();

  // Build PSO
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature = m_rootSignature.Get();

  // Shaders (pre-compiled headers)
  psoDesc.VS = {g_pVertexShader, sizeof(g_pVertexShader)};
  psoDesc.PS = {g_pPixelShader, sizeof(g_pPixelShader)};

  // Input layout (matches CustomVertex)
  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};

  // Rasterizer state
  psoDesc.RasterizerState.FillMode = key.fillMode == 1 ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
  switch (key.cullMode)
  {
  case 0:
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    break;
  case 1:
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    break;
  case 2: default:
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    break;
  }
  psoDesc.RasterizerState.FrontCounterClockwise = FALSE; // We handle winding via cull mode mapping
  psoDesc.RasterizerState.DepthClipEnable = TRUE;

  // Depth stencil
  psoDesc.DepthStencilState.DepthEnable = key.depthEnable ? TRUE : FALSE;
  psoDesc.DepthStencilState.DepthWriteMask = key.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(key.depthFunc);
  psoDesc.DepthStencilState.StencilEnable = FALSE;

  // Blend state
  psoDesc.BlendState.RenderTarget[0].BlendEnable = key.blendEnable ? TRUE : FALSE;
  psoDesc.BlendState.RenderTarget[0].SrcBlend = static_cast<D3D12_BLEND>(key.srcBlend);
  psoDesc.BlendState.RenderTarget[0].DestBlend = static_cast<D3D12_BLEND>(key.dstBlend);
  psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = ToAlphaBlend(static_cast<D3D12_BLEND>(key.srcBlend));
  psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = ToAlphaBlend(static_cast<D3D12_BLEND>(key.dstBlend));
  psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  // Sample
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.SampleDesc.Count = 1;

  // Topology type
  switch (key.topologyType)
  {
  case 0:
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    break;
  case 1:
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    break;
  case 2: default:
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    break;
  }

  // Render target format
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.DSVFormat = m_depthStencilBuffer ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_UNKNOWN;

  ComPtr<ID3D12PipelineState> pso;
  ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)), "Failed to create PSO");

  auto* rawPtr = pso.Get();
  m_psoCache[key] = std::move(pso);
  return rawPtr;
}

void D3D12Backend::BeginFrame()
{
  // Transition back buffer to render target
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  m_commandList->ResourceBarrier(1, &barrier);

  // Set render target and depth stencil
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
  m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, m_depthStencilBuffer ? &dsvHandle : nullptr);

  // Set descriptor heaps
  ID3D12DescriptorHeap* heaps[] = {m_srvCbvHeap.Get(), m_samplerHeap.Get()};
  m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

  // Set root signature
  m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  // Reset upload buffer for the frame
  m_uploadBuffer.Reset();
}

void D3D12Backend::EndFrame()
{
  // Transition back buffer to present
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  m_commandList->ResourceBarrier(1, &barrier);

  // Close and execute
  ThrowIfFailed(m_commandList->Close(), "Failed to close command list");

  ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
  m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

  // Present
  UINT syncInterval = m_vsync ? 1 : 0;
  m_swapChain->Present(syncInterval, 0);

  MoveToNextFrame();
  BeginFrame();
}

void D3D12Backend::MoveToNextFrame()
{
  // Schedule a signal
  const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
  ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue), "Failed to signal fence");

  // Advance frame index
  m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

  // Wait if the next frame's resources are still in use
  if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
  {
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent), "Failed to set fence event");
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
  }

  m_fenceValues[m_frameIndex] = currentFenceValue + 1;

  // Reset allocator and command list for the new frame
  ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset(), "Failed to reset command allocator");
  ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr), "Failed to reset command list");
}

void D3D12Backend::WaitForGpu()
{
  // Signal and wait for all frames to complete
  UINT64 fenceValue = m_fenceValues[m_frameIndex];
  ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceValue), "Failed to signal fence for GPU wait");

  ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent), "Failed to set fence event for GPU wait");
  WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

  m_fenceValues[m_frameIndex] = fenceValue + 1;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Backend::GetCurrentRTVHandle() const
{
  D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += m_frameIndex * m_rtvDescriptorSize;
  return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Backend::GetCurrentDSVHandle() const { return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); }
