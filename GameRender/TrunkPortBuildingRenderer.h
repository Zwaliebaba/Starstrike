#pragma once

#include "DefaultBuildingRenderer.h"

// Renders TrunkPort: base shape + 3D destination text labels at markers,
// and the animated heightmap portal surface when the port is open.
class TrunkPortBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
