#include "pch.h"

#include "ScriptTriggerBuildingRenderer.h"

void ScriptTriggerBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // ScriptTrigger intentionally suppresses all alpha rendering
    // (no ownership lights, no ports).
}
