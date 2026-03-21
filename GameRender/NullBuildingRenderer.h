#pragma once

#include "BuildingRenderer.h"

// Renderer that renders nothing in all passes.  Used for invisible buildings
// like SpawnPopulationLock that override Render() with empty bodies.
class NullBuildingRenderer : public BuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override {}
};
