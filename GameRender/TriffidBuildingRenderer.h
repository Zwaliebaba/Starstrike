#pragma once

#include "DefaultBuildingRenderer.h"

// Renders the Triffid building: animated head shape with stem lines,
// damage flicker effect, and an egg preview when triggered.
// Editor-only alpha pass draws launch cone + trigger radius.
class TriffidBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
