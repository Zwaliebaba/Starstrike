#pragma once

#include "ReceiverBuildingRenderer.h"

// Renderer for SpiritReceiver.  Adds a separate head-shape render on top
// of the base shape, provides status-light starbursts at marker positions,
// and delegates the alpha pass to ReceiverBuildingRenderer.
class SpiritReceiverBuildingRenderer : public ReceiverBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderPorts(const Building& _building) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
