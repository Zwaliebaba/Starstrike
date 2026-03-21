#pragma once

#include "DefaultBuildingRenderer.h"

// ScriptTrigger overrides RenderAlphas to do nothing (suppresses the base
// Building lights/ports rendering).  The opaque Render pass uses the
// default shape render from DefaultBuildingRenderer.
class ScriptTriggerBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
