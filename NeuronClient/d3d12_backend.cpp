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
                  &heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_GRAPHICS_PPV_ARGS(m_resource)),
                "Failed to create upload ring buffer");

  D3D12_RANGE readRange = {0, 0};
  ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_cpuBase)), "Failed to map upload ring buffer");

  m_gpuBase = m_resource->GetGPUVirtualAddress();
}

void UploadRingBuffer::Shutdown()
{
  if (m_resource)
  {
    m_resource->Unmap(0, nullptr);
    m_resource = nullptr;
  }
  m_cpuBase = nullptr;
  m_gpuBase = 0;
  m_size = 0;
  m_offset = 0;
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

bool D3D12Backend::Init()
{
  DEBUG_ASSERT_TEXT(FRAME_COUNT <= Neuron::Graphics::Core::GetBackBufferCount(),
                   "FRAME_COUNT exceeds Core back buffer count");

  auto* device = GetDevice();

  CreateDescriptorHeaps();
  CreateRootSignature();
  CreateUploadBuffer();

  // Core::Prepare() has already opened the command list.
  CreateDefaultTexture();
  CreateSamplers();

  // Set up per-frame rendering state.
  BeginFrame();

  return true;
}

void D3D12Backend::Shutdown()
{
  // Wait for the GPU to finish all outstanding work so resources can be
  // safely released.  Must happen before Core::Shutdown() destroys the device.
  WaitForGpu();

  m_psoCache.clear();
  m_rootSignature = nullptr;
  m_srvCbvHeap = nullptr;
  m_samplerHeap = nullptr;
  m_defaultTexture = nullptr;

  for (UINT i = 0; i < FRAME_COUNT; i++)
    m_uploadBuffers[i].Shutdown();
}

ID3D12Device* D3D12Backend::GetDevice() const
{
  return Neuron::Graphics::Core::GetD3DDevice();
}

ID3D12GraphicsCommandList* D3D12Backend::GetCommandList() const
{
  return Neuron::Graphics::Core::GetCommandList();
}

void D3D12Backend::CreateDescriptorHeaps()
{
  auto* device = GetDevice();

  // SRV/CBV heap (shader-visible)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = MAX_SRV_DESCRIPTORS;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_GRAPHICS_PPV_ARGS(m_srvCbvHeap)), "Failed to create SRV/CBV heap");
    m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srvNextFreeSlot = 0;
  }

  // Sampler heap (shader-visible)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = NUM_STATIC_SAMPLERS + 16; // extra room
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_GRAPHICS_PPV_ARGS(m_samplerHeap)), "Failed to create sampler heap");
    m_samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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

  com_ptr<ID3DBlob> signature;
  com_ptr<ID3DBlob> error;
  HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.put(), error.put());
  if (FAILED(hr))
  {
    if (error)
      OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
    ThrowIfFailed(hr, "Failed to serialize root signature");
  }

  ThrowIfFailed(GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(m_rootSignature)),
                "Failed to create root signature");
}

void D3D12Backend::CreateUploadBuffer()
{
  for (UINT i = 0; i < FRAME_COUNT; i++)
    m_uploadBuffers[i].Init(GetDevice(), UPLOAD_BUFFER_SIZE);
}

void D3D12Backend::CreateDefaultTexture()
{
  auto* device = GetDevice();
  auto* cmdList = GetCommandList();
  UINT frameIndex = Neuron::Graphics::Core::GetCurrentFrameIndex();
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

  ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_GRAPHICS_PPV_ARGS(m_defaultTexture)), "Failed to create default texture");

  // Upload the white pixel
  UINT64 uploadSize = 0;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
  device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &uploadSize);

  auto alloc = m_uploadBuffers[frameIndex].Allocate(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  auto dest = static_cast<UINT8*>(alloc.cpuPtr);
  // RGBA white
  dest[0] = 255;
  dest[1] = 255;
  dest[2] = 255;
  dest[3] = 255;

  D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
  srcLoc.pResource = m_uploadBuffers[frameIndex].GetResource();
  srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  srcLoc.PlacedFootprint = footprint;
  srcLoc.PlacedFootprint.Offset = alloc.offset;

  D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
  dstLoc.pResource = m_defaultTexture.get();
  dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dstLoc.SubresourceIndex = 0;

  cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

  // Transition to shader resource
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_defaultTexture.get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  // Create SRV
  UINT srvIndex;
  D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = AllocateSRVSlot(srvIndex);
  m_defaultTextureSRVIndex = srvIndex;

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(m_defaultTexture.get(), &srvDesc, srvHandle);
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

    GetDevice()->CreateSampler(&desc, samplerHandle);
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

ID3D12PipelineState* D3D12Backend::GetOrCreatePSO(const PSOKey& key)
{
  auto it = m_psoCache.find(key);
  if (it != m_psoCache.end())
    return it->second.get();

  // Build PSO
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature = m_rootSignature.get();

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

  // Render target format — use Core's formats
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = Neuron::Graphics::Core::GetBackBufferFormat();
  psoDesc.DSVFormat = Neuron::Graphics::Core::GetDepthBufferFormat();

  com_ptr<ID3D12PipelineState> pso;
  ThrowIfFailed(GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(pso)), "Failed to create PSO");

  auto* rawPtr = pso.get();
  m_psoCache[key] = std::move(pso);
  return rawPtr;
}

void D3D12Backend::BeginFrame()
{
  // Core::Prepare() has already reset the command allocator/list and
  // transitioned the back buffer to RENDER_TARGET.  We just need to set
  // our shader-visible descriptor heaps, root signature, and render targets.

  auto* cmdList = GetCommandList();

  // Set our shader-visible descriptor heaps (overrides Core's CPU-only heaps)
  ID3D12DescriptorHeap* heaps[] = {m_srvCbvHeap.get(), m_samplerHeap.get()};
  cmdList->SetDescriptorHeaps(_countof(heaps), heaps);

  // Set root signature
  cmdList->SetGraphicsRootSignature(m_rootSignature.get());

  // Set render targets from Core
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Neuron::Graphics::Core::GetRenderTargetView();
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Neuron::Graphics::Core::GetDepthStencilView();
  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void D3D12Backend::EndFrame()
{
  using namespace Neuron::Graphics;

  // Transition back buffer to present state via Core's resource state tracker
  Core::GetGpuResourceStateTracker()->TransitionResource(
    Core::GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, true);

  // Close command list, execute, present, move to next frame
  Core::Present();

  // Prepare next frame (reset allocator/cmdlist, transition back buffer to RT)
  Core::Prepare();

  // Reset this frame's upload buffer
  m_uploadBuffers[Core::GetCurrentFrameIndex()].Reset();

  // Set up our per-frame rendering state
  BeginFrame();
}

void D3D12Backend::WaitForGpu()
{
  Neuron::Graphics::Core::WaitForGpu();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Backend::GetCurrentRTVHandle() const
{
  return Neuron::Graphics::Core::GetRenderTargetView();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Backend::GetCurrentDSVHandle() const
{
  return Neuron::Graphics::Core::GetDepthStencilView();
}
