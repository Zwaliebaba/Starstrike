#include "pch.h"

#include "PylonEndBuildingRenderer.h"

void PylonEndBuildingRenderer::RenderAlphas([[maybe_unused]] const Building& _building, [[maybe_unused]] const BuildingRenderContext& _ctx)
{
    // Intentionally empty — PylonEnd suppresses the power line rendering
    // that PowerBuildingRenderer::RenderAlphas would normally draw.
}
