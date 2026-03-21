#include "pch.h"

#include "TrackJunctionBuildingRenderer.h"

// ---------------------------------------------------------------------------
// TrackJunctionBuildingRenderer::Render
// ---------------------------------------------------------------------------

void TrackJunctionBuildingRenderer::Render(const Building& _building,
                                            const BuildingRenderContext& _ctx)
{
    // TrackJunction skips cart rendering — just render the shape.
    DefaultBuildingRenderer::Render(_building, _ctx);
}
