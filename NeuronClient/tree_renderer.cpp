#include "pch.h"
#include "tree_renderer.h"
#include "d3d12_backend.h"
#include "UploadRingBuffer.h"
#include "matrix34.h"
#include "opengl_directx.h"
#include "tree.h"
#include "tree_mesh_data.h"

#include "CompiledShaders/TreeVertexShader.h"
#include "CompiledShaders/TreePixelShader.h"

using namespace DirectX;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

TreeRenderer& TreeRenderer::Get()
{
  static TreeRenderer s_instance;
  return s_instance;
}

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void TreeRenderer::Init()
{
  // --- Input Layout ---
  D3D12_INPUT_ELEMENT_DESC inputElements[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  D3D12_INPUT_LAYOUT_DESC inputLayout = {inputElements, _countof(inputElements)};

  // --- Blend State: Additive (SrcAlpha / One) ---
  D3D12_BLEND_DESC blendDesc = {};
  blendDesc.RenderTarget[0].BlendEnable = TRUE;
  blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  // --- Rasterizer: No culling (double-sided quads) ---
  D3D12_RASTERIZER_DESC rasterDesc = {};
  rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
  rasterDesc.FrontCounterClockwise = FALSE;
  rasterDesc.DepthClipEnable = TRUE;

  // --- Depth/Stencil: Test enabled, write disabled ---
  D3D12_DEPTH_STENCIL_DESC depthDesc = {};
  depthDesc.DepthEnable = TRUE;
  depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // --- PSO (uses shared root signature from OpenGL translation layer) ---
  // NOTE: shares root signature with OpenGL translation layer.
  // If tree shaders diverge (new params, different SRV layout),
  // create a dedicated root sig here instead.
  m_pso.SetRootSignature(OpenGLD3D::g_glState.GetRootSignature());
  m_pso.SetInputLayout(&inputLayout);
  m_pso.SetBlendState(blendDesc);
  m_pso.SetRasterizerState(rasterDesc);
  m_pso.SetDepthStencilState(depthDesc);
  m_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
  m_pso.SetRenderTargetFormat(Graphics::Core::Get().GetBackBufferFormat(), Graphics::Core::Get().GetDepthBufferFormat());
  m_pso.SetVertexShader(g_pTreeVertexShader, sizeof(g_pTreeVertexShader));
  m_pso.SetPixelShader(g_pTreePixelShader, sizeof(g_pTreePixelShader));
  m_pso.Finalize();

  DebugTrace("TreeRenderer initialised\n");
}

void TreeRenderer::Shutdown()
{
  m_gpuBuffers.clear();
  m_pendingUploads.clear();
 m_pso.Destroy();
  m_textureLoaded = false;
  DebugTrace("TreeRenderer shut down\n");
}

// ---------------------------------------------------------------------------
// EnsureUploaded — lazy-generates mesh and uploads to GPU
// ---------------------------------------------------------------------------

void TreeRenderer::EnsureUploaded(Tree* _tree)
{
  int uid = _tree->m_id.GetUniqueId();

  // If already uploaded and mesh is clean, nothing to do.
  if (!_tree->m_meshDirty && m_gpuBuffers.contains(uid))
    return;

  // Safety net: if meshes are empty, generate now.
  if (_tree->m_branchMesh.vertices.empty() && _tree->m_leafMesh.vertices.empty())
    _tree->Generate();

  const auto& branchVerts = _tree->m_branchMesh.vertices;
  const auto& leafVerts = _tree->m_leafMesh.vertices;

  UINT branchCount = static_cast<UINT>(branchVerts.size());
  UINT leafCount = static_cast<UINT>(leafVerts.size());
  UINT totalCount = branchCount + leafCount;

  if (totalCount == 0)
    return;

  SIZE_T vertexSize = sizeof(TreeVertex);
  SIZE_T bufferSize = static_cast<SIZE_T>(totalCount) * vertexSize;
  auto* device = Graphics::Core::Get().GetD3DDevice();
  auto* cmdList = Graphics::Core::Get().GetCommandList();

  // --- Create default-heap vertex buffer ---
  D3D12_HEAP_PROPERTIES defaultHeap = {};
  defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_RESOURCE_DESC bufferDesc = {};
  bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufferDesc.Width = bufferSize;
  bufferDesc.Height = 1;
  bufferDesc.DepthOrArraySize = 1;
  bufferDesc.MipLevels = 1;
  bufferDesc.SampleDesc.Count = 1;
  bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  TreeGPUData gpu;
  check_hresult(device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                IID_GRAPHICS_PPV_ARGS(gpu.vertexBuffer)));

  // --- Upload via ring buffer ---
  auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(bufferSize, sizeof(float));

  auto* dst = static_cast<uint8_t*>(alloc.cpuPtr);
  if (branchCount > 0)
    memcpy(dst, branchVerts.data(), branchCount * vertexSize);
  if (leafCount > 0)
    memcpy(dst + branchCount * vertexSize, leafVerts.data(), leafCount * vertexSize);

  // Transition COMMON → COPY_DEST
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = gpu.vertexBuffer.get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  cmdList->CopyBufferRegion(gpu.vertexBuffer.get(), 0, Graphics::GetCurrentUploadBuffer().GetResource(), alloc.offset, bufferSize);

  // Transition COPY_DEST → VERTEX_AND_CONSTANT_BUFFER
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  cmdList->ResourceBarrier(1, &barrier);

  // --- Fill VB view and offsets ---
  gpu.vbView.BufferLocation = gpu.vertexBuffer->GetGPUVirtualAddress();
  gpu.vbView.SizeInBytes = static_cast<UINT>(bufferSize);
  gpu.vbView.StrideInBytes = static_cast<UINT>(vertexSize);
  gpu.branchVertexStart = 0;
  gpu.branchVertexCount = branchCount;
  gpu.leafVertexStart = branchCount;
  gpu.leafVertexCount = leafCount;

  m_gpuBuffers[uid] = std::move(gpu);
  _tree->m_meshDirty = false;
}

// ---------------------------------------------------------------------------
// DrawTree — renders a single tree with dedicated PSO
// ---------------------------------------------------------------------------

void TreeRenderer::DrawTree(Tree* _tree, float _predictionTime, unsigned int _textureGLId, bool _skipLeaves)
{
  EnsureUploaded(_tree);

  int uid = _tree->m_id.GetUniqueId();
  auto it = m_gpuBuffers.find(uid);
  if (it == m_gpuBuffers.end())
    return; // Nothing to draw

  TreeGPUData& gpu = it->second;

  // --- Lazy texture SRV resolution ---
  if (!m_textureLoaded)
  {
    m_treeTextureSRV = OpenGLD3D::GetTextureSRVGPUHandle(_textureGLId);
    m_textureLoaded = true;
  }

  // --- Build world × view matrix ---
  float actualHeight = _tree->GetActualHeight(_predictionTime);

  LegacyVector3 up(0.0f, 1.0f, 0.0f);
  Matrix34 mat(_tree->m_front, up, _tree->m_pos);
  XMFLOAT4X4 worldFloat = mat.ToXMFLOAT4X4();
  XMMATRIX world = XMLoadFloat4x4(&worldFloat);

  // Replicate: Scale(h, h, h) * World * View
  XMMATRIX scale = XMMatrixScaling(actualHeight, actualHeight, actualHeight);
  XMMATRIX view = OpenGLD3D::GetModelViewStack().GetTopXM();
  XMMATRIX proj = OpenGLD3D::GetProjectionStack().GetTopXM();
  XMMATRIX worldView = XMMatrixMultiply(XMMatrixMultiply(scale, world), view);

  auto* cmdList = Graphics::Core::Get().GetCommandList();

  // --- Set PSO (shared root sig is already bound by BeginFrame) ---
  cmdList->SetPipelineState(m_pso.GetPipelineStateObject());

  // --- Bind scene constants (b0) ---
  OpenGLD3D::EnsureSceneConstantsUploaded();
  cmdList->SetGraphicsRootConstantBufferView(0, OpenGLD3D::GetSceneConstantsGPUAddr());

  // --- Bind texture SRV (param 2) ---
  cmdList->SetGraphicsRootDescriptorTable(2, m_treeTextureSRV);

  // --- Bind sampler (param 4) — linear/wrap/trilinear (index 0) ---
  // Index 0 = linear/wrap/mipLinear — matches the old static sampler
  cmdList->SetGraphicsRootDescriptorTable(4, OpenGLD3D::g_glState.GetSamplerBaseGPUHandle());

  // --- Bind vertex buffer ---
  cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  cmdList->IASetVertexBuffers(0, 1, &gpu.vbView);

  // --- Populate draw constants ---
  OpenGLD3D::DrawConstants dc = {};
  XMStoreFloat4x4(&dc.WorldMatrix, worldView);
  XMStoreFloat4x4(&dc.ProjectionMatrix, proj);
  dc.FogEnabled = OpenGLD3D::GetFogState().enabled ? 1u : 0u;

  // --- Draw branches ---
  dc.MatDiffuse = XMFLOAT4(_tree->m_branchColourArray[0] / 255.0f, _tree->m_branchColourArray[1] / 255.0f,
                           _tree->m_branchColourArray[2] / 255.0f, _tree->m_branchColourArray[3] / 255.0f);

  auto cbAlloc = Graphics::GetCurrentUploadBuffer().Allocate(sizeof(OpenGLD3D::DrawConstants), 256);
  memcpy(cbAlloc.cpuPtr, &dc, sizeof(dc));
  cmdList->SetGraphicsRootConstantBufferView(1, cbAlloc.gpuAddr);
  cmdList->DrawInstanced(gpu.branchVertexCount, 1, gpu.branchVertexStart, 0);

  // --- Draw leaves (skipped when Christmas mod is enabled) ---
  if (gpu.leafVertexCount > 0 && !_skipLeaves)
  {
    dc.MatDiffuse = XMFLOAT4(_tree->m_leafColourArray[0] / 255.0f, _tree->m_leafColourArray[1] / 255.0f,
                             _tree->m_leafColourArray[2] / 255.0f, _tree->m_leafColourArray[3] / 255.0f);

    auto cbAlloc2 = Graphics::GetCurrentUploadBuffer().Allocate(sizeof(OpenGLD3D::DrawConstants), 256);
    memcpy(cbAlloc2.cpuPtr, &dc, sizeof(dc));
    cmdList->SetGraphicsRootConstantBufferView(1, cbAlloc2.gpuAddr);
    cmdList->DrawInstanced(gpu.leafVertexCount, 1, gpu.leafVertexStart, 0);
  }
}

// ---------------------------------------------------------------------------
// ReleaseTree — ITreeRenderBackend implementation
// ---------------------------------------------------------------------------

void TreeRenderer::ReleaseTree(int _uniqueId) { m_gpuBuffers.erase(_uniqueId); }
