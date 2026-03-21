#pragma once

#include "PowerBuildingRenderer.h"

// Renderer for SolarPanel buildings.  Adds smooth shading in the opaque
// pass, glow quads in the alpha pass, and custom port status lights.
class SolarPanelBuildingRenderer : public PowerBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderPorts(const Building& _building) override;
};
