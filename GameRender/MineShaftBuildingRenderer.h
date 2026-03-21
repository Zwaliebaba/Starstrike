#pragma once

#include "MineBuildingRenderer.h"

// Renderer for the Mine type (the mine shaft).  Adds spinning wheels to the
// base MineBuildingRenderer cart + track-rail rendering.
class MineShaftBuildingRenderer : public MineBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
