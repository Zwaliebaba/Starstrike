#include "pch.h"

#include "BlueprintConsoleBuildingRenderer.h"
#include "blueprintstore.h"
#include "camera.h"
#include "GameApp.h"
#include "location.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"

// ---------------------------------------------------------------------------
// BlueprintConsoleBuildingRenderer::Render
// ---------------------------------------------------------------------------

void BlueprintConsoleBuildingRenderer::Render(const Building& _building,
                                               const BuildingRenderContext& _ctx)
{
    // Velocity-predicted first copy (base BlueprintBuilding behaviour).
    BlueprintBuildingRenderer::Render(_building, _ctx);

    // Second copy at the exact position with zero prediction time.
    Matrix34 mat(_building.m_front, g_upVector, _building.m_pos);
    _building.m_shape->Render(0.0f, mat);
}

// ---------------------------------------------------------------------------
// BlueprintConsoleBuildingRenderer::RenderPorts
// ---------------------------------------------------------------------------

void BlueprintConsoleBuildingRenderer::RenderPorts(const Building& _building)
{
    auto& console = static_cast<const BlueprintConsole&>(_building);

    for (int i = 0; i < const_cast<Building&>(static_cast<Building&>(
             const_cast<BlueprintConsole&>(console))).GetNumPorts(); ++i)
    {
        LegacyVector3 portPos;
        LegacyVector3 portFront;
        const_cast<BlueprintConsole&>(console).GetPortPosition(i, portPos, portFront);

        LegacyVector3 portUp = g_upVector;
        Matrix34 mat(portFront, portUp, portPos);

        //
        // Render the status light

        float size = 6.0f;
        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos =
            Building::s_controlPad->GetMarkerWorldMatrix(Building::s_controlPadStatus, mat).pos;
        statusPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(
            statusPos.x, statusPos.z);
        statusPos.y += 5.0f;

        WorldObjectId occupantId =
            const_cast<BlueprintConsole&>(console).GetPortOccupant(i);
        if (!occupantId.IsValid())
            glColor4ub(150, 150, 150, 255);
        else
        {
            RGBAColour teamColour =
                g_context->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv(teamColour.GetData());
        }

        glDisable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,
                      Resource::GetTexture("textures/starburst.bmp"));
        glDepthMask(false);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv(const_cast<float*>((statusPos - camR - camU).GetData()));
        glTexCoord2i(1, 0);
        glVertex3fv(const_cast<float*>((statusPos + camR - camU).GetData()));
        glTexCoord2i(1, 1);
        glVertex3fv(const_cast<float*>((statusPos + camR + camU).GetData()));
        glTexCoord2i(0, 1);
        glVertex3fv(const_cast<float*>((statusPos - camR + camU).GetData()));
        glEnd();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glDepthMask(true);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
    }
}
