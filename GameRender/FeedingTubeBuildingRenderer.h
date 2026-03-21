#pragma once

#include "DefaultBuildingRenderer.h"

// Renders FeedingTube: base shape via default, and the multitexture
// cylindrical signal beam connecting paired tubes.
class FeedingTubeBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;

  private:
    void RenderSignal(const class FeedingTube& _tube, float _predictionTime, float _radius, float _alpha);
};
