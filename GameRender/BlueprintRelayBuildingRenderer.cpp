#include "pch.h"

#include "BlueprintRelayBuildingRenderer.h"
#include "blueprintstore.h"
#include "GameApp.h"

// ---------------------------------------------------------------------------
// BlueprintRelayBuildingRenderer::Render
// ---------------------------------------------------------------------------

void BlueprintRelayBuildingRenderer::Render(const Building& _building,
                                             const BuildingRenderContext& _ctx)
{
    BlueprintBuildingRenderer::Render(_building, _ctx);

    // In the editor the relay's y position is snapped to its configured
    // altitude so the designer can see where it will float at runtime.
    if (g_context->m_editing)
    {
        auto& relay = static_cast<const BlueprintRelay&>(_building);
        const_cast<BlueprintRelay&>(relay).m_pos.y = relay.m_altitude;
    }
}
