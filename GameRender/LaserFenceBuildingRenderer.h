#pragma once

#include "DefaultBuildingRenderer.h"

// Renders LaserFence posts (scaled shape with GL_NORMALIZE) and the
// translucent fence quad + burn overlay connecting adjacent posts.
// Overrides RenderLights with an empty body to suppress the base-class
// team-ownership starburst (original code does the same).
class LaserFenceBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderLights(const Building& _building) override;
};
