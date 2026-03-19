#pragma once

// Abstract interface for GPU resource cleanup on tree destruction.
// Defined in GameLogic (no DX12 headers), implemented in NeuronClient.
class ITreeRenderBackend
{
  public:
    virtual ~ITreeRenderBackend() = default;
    virtual void ReleaseTree(int uniqueId) = 0;
};

extern ITreeRenderBackend* g_treeRenderBackend;
