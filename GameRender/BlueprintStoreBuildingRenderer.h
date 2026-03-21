#pragma once

#include "BlueprintBuildingRenderer.h"

// Renderer for BlueprintStore.  Renders the base building and link line via
// BlueprintBuildingRenderer, then adds a floating darwinian infection display
// quad with scanline overlay in the alpha pass.
class BlueprintStoreBuildingRenderer : public BlueprintBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
