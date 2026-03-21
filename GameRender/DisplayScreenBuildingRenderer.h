#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for DisplayScreen.
// Render() inherits from DefaultBuildingRenderer (base Building shape).
// RenderAlphas() renders a glow blob, smooth-shaded rays from markers to a
// rotating armour shape, and the armour with a multiplicative blend mode.
class DisplayScreenBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
