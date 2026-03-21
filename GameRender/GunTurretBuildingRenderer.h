#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for GunTurret.
// Render() draws the turret shape at the turret-front orientation,
// computes barrel position from shape markers, and renders the barrel.
// RenderPorts() draws custom status light billboards at marker positions
// (replaces the base Building::RenderPorts pattern).
class GunTurretBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderPorts(const Building& _building) override;
};
