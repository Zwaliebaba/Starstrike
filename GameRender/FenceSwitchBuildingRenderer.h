#pragma once

#include "DefaultBuildingRenderer.h"

class LegacyVector3;

// Renders FenceSwitch: base shape via DefaultBuildingRenderer, connection
// beams to linked LaserFence posts, and custom status lights that change
// colour based on the current switch value.
class FenceSwitchBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderLights(const Building& _building) override;

  private:
    void RenderConnection(const LegacyVector3& _ourPos, const LegacyVector3& _targetPos, bool _active);
};
