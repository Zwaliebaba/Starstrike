#include "pch.h"
#include "d3d12_backend.h"
#include "UploadRingBuffer.h"

#include "CompiledShaders/VertexShader.h"
#include "CompiledShaders/PixelShader.h"

using namespace OpenGLD3D;

OpenGLTranslationState OpenGLD3D::g_glState;

// ---- Helpers ----

// Map colour-manipulating blend factors to their alpha equivalents.
// D3D12 forbids SRC_COLOR / INV_SRC_COLOR / DEST_COLOR / INV_DEST_COLOR
// in SrcBlendAlpha / DestBlendAlpha.
static D3D12_BLEND ToAlphaBlend(D3D12_BLEND blend)
{
  switch (blend)
  {
  case D3D12_BLEND_SRC_COLOR:
    return D3D12_BLEND_SRC_ALPHA;
  case D3D12_BLEND_INV_SRC_COLOR:
    return D3D12_BLEND_INV_SRC_ALPHA;
  case D3D12_BLEND_DEST_COLOR:
    return D3D12_BLEND_DEST_ALPHA;
  case D3D12_BLEND_INV_DEST_COLOR:
    return D3D12_BLEND_INV_DEST_ALPHA;
  default:
    return blend;
  }
}

// ---- OpenGLTranslationState ----

bool OpenGLTranslationState::Init()
{
  CreateRootSignature();

  // Core::Prepare() has already opened the command list.
  CreateDefaultTexture();
  CreateSamplers();

  // Register as frame listener so OnFrameBegin is called each frame.
  Graphics::Core::Get().RegisterFrameListener(this);

  // Initial per-frame rendering state (first frame).
  OnFrameBegin(Graphics::Core::Get().GetCommandList());

  return true;
}

void OpenGLTranslationState::Shutdown()
{
  // Unregister before releasing resources so Core::Prepare() never dispatches
  // to a dangling listener.
  Graphics::Core::Get().UnregisterFrameListener(this);

  // Wait for the GPU to finish all outstanding work so resources can be
  // safely released.  Must happen before Core::Shutdown() destroys the device.
  Graphics::Core::Get().WaitForGpu();

  m_psoCache.clear();
  m_rootSignature.Reset(0, 0);
  m_defaultTexture = nullptr;
}

void OpenGLTranslationState::CreateRootSignature()
{
  // Shared root signature — used by uber and tree pipelines.
  // Param 0: CBV  b0  (SceneConstants  — per-frame)
  // Param 1: CBV  b1  (DrawConstants   — per-draw)
  // Param 2: SRV table t0  (texture 0)
  // Param 3: SRV table t1  (texture 1)
  // Param 4: Sampler table s0-s1

  m_rootSignature.Reset(5, 0);

  m_rootSignature[0].InitAsConstantBuffer(0); // b0 — SceneConstants
  m_rootSignature[1].InitAsConstantBuffer(1); // b1 — DrawConstants
  m_rootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1,
                                           D3D12_SHADER_VISIBILITY_PIXEL); // t0
  m_rootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1,
                                           D3D12_SHADER_VISIBILITY_PIXEL); // t1
  m_rootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 2,
                                           D3D12_SHADER_VISIBILITY_PIXEL); // s0-s1

  m_rootSignature.Finalize(L"Shared Root Signature",
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void OpenGLTranslationState::CreateDefaultTexture()
{
  auto* device = Graphics::Core::Get().GetD3DDevice();
  auto* cmdList = Graphics::Core::Get().GetCommandList();
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

  check_hresult(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                IID_GRAPHICS_PPV_ARGS(m_defaultTexture)));

  // Upload the white pixel
  UINT64 uploadSize = 0;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
  device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &uploadSize);

  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  auto dest = static_cast<UINT8*>(alloc.cpuPtr);
  // RGBA white
  dest[0] = 255;
  dest[1] = 255;
  dest[2] = 255;
  dest[3] = 255;

  D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
  srcLoc.pResource = Graphics::GetCurrentUploadBuffer().GetResource();
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
  m_defaultTextureSRVHandle = AllocateSRVSlot();

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(m_defaultTexture.get(), &srvDesc, m_defaultTextureSRVHandle);
}

void OpenGLTranslationState::CreateSamplers()
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

  // Allocate a contiguous block of sampler descriptors from the unified heap.
  m_samplerBaseHandle = Graphics::DescriptorAllocator::Allocate(
    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, NUM_STATIC_SAMPLERS);

  UINT samplerDescSize = Graphics::DescriptorAllocator::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_samplerBaseHandle;

  for (UINT i = 0; i < NUM_STATIC_SAMPLERS; i++)
  {
    D3D12_SAMPLER_DESC desc = {};
    desc.Filter = configs[i].filter;
    desc.AddressU = configs[i].addressU;
    desc.AddressV = configs[i].addressV;
    desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    desc.MaxLOD = D3D12_FLOAT32_MAX;
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

    Graphics::Core::Get().GetD3DDevice()->CreateSampler(&desc, cpuHandle);
    cpuHandle.ptr += samplerDescSize;
  }
}

Graphics::DescriptorHandle OpenGLTranslationState::AllocateSRVSlot()
{
  return Graphics::DescriptorAllocator::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

ID3D12PipelineState* OpenGLTranslationState::GetOrCreatePSO(const PSOKey& key)
{
  auto it = m_psoCache.find(key);
  if (it != m_psoCache.end())
    return it->second.get();

  // Build PSO
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature = m_rootSignature.GetSignature();

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
  psoDesc.RTVFormats[0] = Graphics::Core::Get().GetBackBufferFormat();
  psoDesc.DSVFormat = Graphics::Core::Get().GetDepthBufferFormat();

  com_ptr<ID3D12PipelineState> pso;
  check_hresult(Graphics::Core::Get().GetD3DDevice()->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(pso)));

  auto* rawPtr = pso.get();
  m_psoCache[key] = std::move(pso);
  return rawPtr;
}

void OpenGLTranslationState::OnFrameBegin(ID3D12GraphicsCommandList* cmdList)
{
  // Core::Prepare() has already reset the command allocator/list and
  // transitioned the back buffer to RENDER_TARGET.  Set the unified
  // shader-visible descriptor heaps, root signature, and render targets.

  Graphics::DescriptorAllocator::SetDescriptorHeaps(cmdList);

  // Set root signature
  cmdList->SetGraphicsRootSignature(m_rootSignature.GetSignature());

  // Set render targets from Core
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Graphics::Core::Get().GetRenderTargetView();
  D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Graphics::Core::Get().GetDepthStencilView();
  cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}
