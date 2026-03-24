#include "pch.h"

#include "AITargetBuildingRenderer.h"
#include "building.h"
#include "GameApp.h"
#include "main.h"

void AITargetBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    if (g_context->m_editing)
        DefaultBuildingRenderer::Render(_building, _ctx);
}

void AITargetBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    // Intentionally empty — original AITarget::RenderAlphas was empty.
}
