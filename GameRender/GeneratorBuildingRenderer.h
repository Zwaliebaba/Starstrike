#pragma once

#include "PowerBuildingRenderer.h"

// Renderer for Generator buildings.  Adds a throughput counter rendered
// at the shape's counter marker on top of the standard power-building
// opaque + alpha passes.
class GeneratorBuildingRenderer : public PowerBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
