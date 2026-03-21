#pragma once

#include "MineBuildingRenderer.h"

// Renderer for Refinery.  Adds spinning wheels and a counter display to the
// base MineBuildingRenderer cart + track-rail rendering.
class RefineryBuildingRenderer : public MineBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
