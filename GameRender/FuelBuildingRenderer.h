#pragma once

#include "DefaultBuildingRenderer.h"

class FuelBuilding;

// Base renderer for all FuelBuilding-family types.
// Render() renders the building shape + the fuel pipe to the linked building.
// RenderAlphas() renders the animated fuel-flow quad between linked buildings.
class FuelBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
