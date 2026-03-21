#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for AITarget.
// Render() only shows the building shape in editor mode.
// RenderAlphas() is intentionally empty (the original was empty too).
class AITargetBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
