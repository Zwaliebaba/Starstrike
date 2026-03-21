#pragma once

#include "DefaultBuildingRenderer.h"

// Renderer for PylonEnd buildings.  Suppresses the power-line alpha pass
// that PowerBuilding normally draws — PylonEnd is the terminal node.
class PylonEndBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
