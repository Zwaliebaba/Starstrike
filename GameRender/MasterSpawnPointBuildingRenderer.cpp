#include "pch.h"

#include "MasterSpawnPointBuildingRenderer.h"
#include "building.h"
#include "GameApp.h"

void MasterSpawnPointBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    if (_building.m_isGlobal || g_app->m_editing)
    {
        SpawnBuildingRenderer::Render(_building, _ctx);
    }
}

void MasterSpawnPointBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    if (_building.m_isGlobal || g_app->m_editing)
    {
        SpawnBuildingRenderer::RenderAlphas(_building, _ctx);
    }
}
