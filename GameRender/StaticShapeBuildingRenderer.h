#pragma once

#include "DefaultBuildingRenderer.h"

// StaticShape applies a uniform scale to the transform matrix and enables
// GL_NORMALIZE so scaled normals remain unit-length for correct lighting.
// RenderAlphas / RenderLights / RenderPorts use the default base pattern.
class StaticShapeBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
