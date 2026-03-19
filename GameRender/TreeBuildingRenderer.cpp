#include "pch.h"

#include "TreeBuildingRenderer.h"
#include "tree_renderer.h"
#include "tree.h"
#include "location.h"
#include "resource.h"
#include "GameApp.h"

// Texture loaded once, shared across all tree renders.
static int s_treeTextureId = -1;

static int EnsureTreeTexture()
{
    if (s_treeTextureId == -1)
        s_treeTextureId = g_app->m_resource->GetTexture("textures/laser.bmp");
    return s_treeTextureId;
}

void TreeBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    // Trees use alpha-blended rendering only (PerformDepthSort returns true).
    // The opaque pass is a no-op.
}

void TreeBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    const auto& tree = static_cast<const Tree&>(_building);
    bool skipLeaves = (Location::ChristmasModEnabled() == 1);
    TreeRenderer::Get().DrawTree(tree, _ctx.predictionTime, EnsureTreeTexture(), skipLeaves);
}
