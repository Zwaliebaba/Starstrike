#include "pch.h"

#include "PylonEndBuildingRenderer.h"

void PylonEndBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // Intentionally empty — PylonEnd suppresses the power line rendering
    // that PowerBuildingRenderer::RenderAlphas would normally draw.
}
