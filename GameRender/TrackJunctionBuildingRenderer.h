#pragma once

#include "MineBuildingRenderer.h"

// Renderer for TrackJunction.  Skips cart rendering in the opaque pass
// (just renders the shape), but keeps the track-rail alpha pass from the
// base MineBuildingRenderer.
class TrackJunctionBuildingRenderer : public MineBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
};
