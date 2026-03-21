#pragma once

#include "SpawnBuildingRenderer.h"

// Renderer for SpawnPoint buildings.  Adds cloudy glow blobs in the alpha
// pass and custom control-pad status lights in RenderPorts.
class SpawnPointBuildingRenderer : public SpawnBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderPorts(const Building& _building) override;
};
