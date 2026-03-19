#pragma once

#include "PipelineState.h"
#include "tree_render_interface.h"

struct TreeMeshData;
class Tree;

// --- Per-tree GPU resources ---
struct TreeGPUData
{
  com_ptr<ID3D12Resource> vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW vbView{};
  UINT branchVertexStart = 0;
  UINT branchVertexCount = 0;
  UINT leafVertexStart = 0;
  UINT leafVertexCount = 0;
  int meshVersion = -1;
};

// --- Tree renderer (singleton) ---
// Owns the dedicated DX12 PSO and per-tree GPU vertex buffers.  Uses the
// shared root signature from OpenGLTranslationState.
// Implements ITreeRenderBackend so Tree::~Tree() can release GPU resources
// without depending on DX12 headers.
class TreeRenderer final : public ITreeRenderBackend
{
  public:
    static TreeRenderer& Get();

    void Init();
    void Shutdown();

    void DrawTree(const Tree& _tree, float _predictionTime, unsigned int _textureGLId, bool _skipLeaves = false);

    // ITreeRenderBackend
    void ReleaseTree(int _uniqueId) override;

  private:
    TreeRenderer() = default;
    ~TreeRenderer() override = default;

    void EnsureUploaded(const Tree& _tree);

    GraphicsPSO m_pso{L"Tree PSO"};

    D3D12_GPU_DESCRIPTOR_HANDLE m_treeTextureSRV{};
    bool m_textureLoaded = false;

    std::unordered_map<int, TreeGPUData> m_gpuBuffers;

    // One-shot upload resources awaiting GPU completion before release.
    // Cleared at the start of each frame after the previous frame's fence signals.
    std::vector<com_ptr<ID3D12Resource>> m_pendingUploads;
};
