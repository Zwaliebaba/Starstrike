#pragma once

#include "DefaultBuildingRenderer.h"

class ConstructionYard;

// Renderer companion for ConstructionYard.
// Render() adds rung shapes and primitive shapes at marker positions with
// sine-wave y offsets on top of the default building shape.
// RenderAlphas() renders cloudyglow blobs, starbursts, and rung-spike
// starbursts with alpha smoothing.
class ConstructionYardBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
