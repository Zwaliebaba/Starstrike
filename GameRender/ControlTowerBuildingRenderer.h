#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer companion for ControlTower.
// Render() draws the base building shape + the dish shape at m_dishMatrix.
// RenderAlphas() draws the control line to heaven and the signal flash,
// plus the base Building lights/ports.
class ControlTowerBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
