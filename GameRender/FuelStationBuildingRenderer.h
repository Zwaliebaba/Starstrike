#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for FuelStation.
// Render() is the standard Building::Render shape pass (DefaultBuildingRenderer).
// RenderAlphas() renders the countdown display screen and projection effect
// when the linked EscapeRocket is in countdown or flight state.
// NOTE: does NOT call FuelBuildingRenderer::RenderAlphas — the original
// FuelStation::RenderAlphas intentionally skipped the fuel-flow quad.
class FuelStationBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
