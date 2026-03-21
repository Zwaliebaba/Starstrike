#pragma once

#include "BlueprintBuildingRenderer.h"

// Renderer for BlueprintConsole.  Renders the base velocity-predicted shape
// via BlueprintBuildingRenderer, then a second copy at exact position with
// zero prediction time.  Also renders team-coloured port status lights.
class BlueprintConsoleBuildingRenderer : public BlueprintBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderPorts(const Building& _building) override;
};
