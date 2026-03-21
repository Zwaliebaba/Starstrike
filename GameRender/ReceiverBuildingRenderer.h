#pragma once

#include "DefaultBuildingRenderer.h"

class LegacyVector3;

// Renderer for the ReceiverBuilding family.  Inherits the default opaque
// pass and adds spirit-link line rendering + in-transit spirit surge dots
// in the alpha pass.  Also provides static helpers for rendering
// unprocessed spirit glyphs (used by SpiritProcessorBuildingRenderer).
class ReceiverBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;

    // Batched spirit glyph rendering — call Begin, then one or more
    // RenderSpirit calls, then End.
    static void BeginRenderUnprocessedSpirits();
    static void RenderUnprocessedSpirit(const LegacyVector3& _pos, float _life = 1.0f);
    static void EndRenderUnprocessedSpirits();
};
