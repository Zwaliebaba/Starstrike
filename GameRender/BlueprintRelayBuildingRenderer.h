#pragma once

#include "BlueprintBuildingRenderer.h"

// Renderer for BlueprintRelay.  Delegates to BlueprintBuildingRenderer and
// applies an editor-only altitude fixup so that the relay is visible at its
// configured height when editing.
class BlueprintRelayBuildingRenderer : public BlueprintBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
