#pragma once

#include "FuelBuildingRenderer.h"

// Renderer companion for FuelGenerator.
// Render() adds the animated pump shape on top of the FuelBuilding pipe.
// RenderAlphas() delegates to FuelBuildingRenderer (fuel-flow quads).
class FuelGeneratorBuildingRenderer : public FuelBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
