#pragma once

#include "TeleportBuildingRenderer.h"

// Renderer companion for RadarDish (inherits from Teleport).
// Render() renders the animated shape instance (instead of the base shape).
// RenderAlphas() draws the multi-texture radar signal tube, then delegates
// to TeleportBuildingRenderer::RenderAlphas for in-transit spirits and
// lights/ports.
class RadarDishBuildingRenderer : public TeleportBuildingRenderer
{
    void RenderSignal(const Building& _building, float _predictionTime,
                      float _radius, float _alpha);

  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
