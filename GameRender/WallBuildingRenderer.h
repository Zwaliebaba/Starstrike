#pragma once

#include "DefaultBuildingRenderer.h"

// Wall has a custom Render that applies position prediction using m_fallSpeed.
// RenderAlphas / RenderLights / RenderPorts use the default base pattern.
class WallBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
