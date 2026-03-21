#pragma once

#include "FuelBuildingRenderer.h"

// Renderer companion for EscapeRocket.
// Render() uses a predicted position instead of the standard Building render.
// RenderAlphas() adds the fuel pipe flow (via FuelBuildingRenderer) plus a
// glow/starburst effect around the rocket's booster when fuelled.
class EscapeRocketBuildingRenderer : public FuelBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
