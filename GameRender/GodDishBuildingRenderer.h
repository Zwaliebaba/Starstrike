#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for GodDish.
// Render() inherits from DefaultBuildingRenderer (base Building pattern).
// RenderAlphas() renders the central glow effect and starbursts, then
// delegates to DefaultBuildingRenderer::RenderAlphas for lights/ports.
class GodDishBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
