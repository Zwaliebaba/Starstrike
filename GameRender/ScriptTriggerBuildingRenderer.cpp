#include "pch.h"

#include "ScriptTriggerBuildingRenderer.h"

void ScriptTriggerBuildingRenderer::RenderAlphas([[maybe_unused]] const Building& _building, [[maybe_unused]] const BuildingRenderContext& _ctx)
{
    // ScriptTrigger intentionally suppresses all alpha rendering
    // (no ownership lights, no ports).
}
