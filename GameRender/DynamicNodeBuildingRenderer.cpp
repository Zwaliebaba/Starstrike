#include "pch.h"

#include "DynamicNodeBuildingRenderer.h"
#include "generichub.h"
#include "main.h"
#include "landscape.h"
#include "location.h"
#include "GameApp.h"

void DynamicNodeBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    if (g_app->m_editing)
    {
        // Editor-mode normal-map alignment — mutates m_up/m_front.
        auto& node = const_cast<DynamicNode&>(static_cast<const DynamicNode&>(_building));
        node.m_up = g_app->m_location->m_landscape.m_normalMap->GetValue(node.m_pos.x, node.m_pos.z);
        LegacyVector3 right(1, 0, 0);
        node.m_front = right ^ node.m_up;
    }

    glShadeModel(GL_SMOOTH);

    // DynamicBase::Render only calls Building::Render if m_shape is non-null.
    if (_building.m_shape)
        DefaultBuildingRenderer::Render(_building, _ctx);

    glShadeModel(GL_FLAT);
}
