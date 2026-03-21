#pragma once

#include "SpawnBuildingRenderer.h"

// Renderer for MasterSpawnPoint buildings.  Only renders when the building
// is global or the editor is active — otherwise invisible.
class MasterSpawnPointBuildingRenderer : public SpawnBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
