#pragma once

// Abstract interface for GPU resource cleanup on simulation object
// destruction.  Defined in GameLogic (no DX12 headers), implemented
// in NeuronClient.
//
// When a simulation object (entity or building) is destroyed, its
// destructor calls the appropriate Release method so the render
// companion can free per-instance GPU resources.
//
// This generalizes the existing ITreeRenderBackend pattern for all
// entity/building types.
class IRenderBackend
{
  public:
    virtual ~IRenderBackend() = default;
    virtual void ReleaseEntity(int _uniqueId) {}
    virtual void ReleaseBuilding(int _uniqueId) {}
};

extern IRenderBackend* g_renderBackend;
