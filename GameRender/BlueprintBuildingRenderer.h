#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer for the BlueprintBuilding family.  Overrides the default opaque
// pass to apply velocity-predicted position and g_upVector orientation.
// In the alpha pass, renders the infection-coloured link line to the
// connected building, then delegates to DefaultBuildingRenderer::RenderAlphas
// for lights and ports.
class BlueprintBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
