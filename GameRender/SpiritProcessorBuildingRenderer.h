#pragma once

#include "ReceiverBuildingRenderer.h"

// Renderer for SpiritProcessor.  Adds floating-spirit glyph rendering on
// top of the base ReceiverBuildingRenderer alpha pass (spirit link lines +
// surges).
class SpiritProcessorBuildingRenderer : public ReceiverBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
