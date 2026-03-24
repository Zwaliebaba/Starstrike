#include "pch.h"

#include "SpawnPointBuildingRenderer.h"
#include "spawnpoint.h"
#include "camera.h"
#include "GameApp.h"
#include "landscape.h"
#include "location.h"
#include "main.h"
#include "matrix34.h"
#include "preferences.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"

void SpawnPointBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    SpawnBuildingRenderer::RenderAlphas(_building, _ctx);

    const auto& sp = static_cast<const SpawnPoint&>(_building);

    LegacyVector3 camUp = g_context->m_camera->GetUp();
    LegacyVector3 camRight = g_context->m_camera->GetRight();

    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/cloudyglow.bmp"));

    float timeIndex = static_cast<float>(g_gameTime) + sp.m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt("RenderBuildingDetail", 1);
    int maxBlobs = 20;
    if (buildingDetail == 2) maxBlobs = 10;
    if (buildingDetail == 3) maxBlobs = 0;

    float alpha = static_cast<float>(const_cast<SpawnPoint&>(sp).GetNumPortsOccupied())
                / static_cast<float>(const_cast<SpawnPoint&>(sp).GetNumPorts());

    for (int i = 0; i < maxBlobs; ++i)
    {
        LegacyVector3 pos = sp.m_centerPos;
        pos += LegacyVector3(0, 25, 0);
        pos.x += sinf(timeIndex + i) * i * 0.7f;
        pos.y += cosf(timeIndex + i) * sinf(i * 10.0f) * 12.0f;
        pos.z += cosf(timeIndex + i) * i * 0.7f;

        float size = 10.0f + sinf(timeIndex + i * 10.0f) * 10.0f;
        size = std::max(size, 5.0f);

        glColor4f(0.6f, 0.2f, 0.1f, alpha);

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((pos - camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((pos + camRight * size + camUp * size).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((pos + camRight * size - camUp * size).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((pos - camRight * size - camUp * size).GetData());
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
}

void SpawnPointBuildingRenderer::RenderPorts(const Building& _building)
{
    const auto& sp = static_cast<const SpawnPoint&>(_building);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_QUADS);

    for (int i = 0; i < const_cast<SpawnPoint&>(sp).GetNumPorts(); ++i)
    {
        LegacyVector3 portPos;
        LegacyVector3 portFront;
        const_cast<SpawnPoint&>(sp).GetPortPosition(i, portPos, portFront);

        LegacyVector3 portUp = g_upVector;
        Matrix34 mat(portFront, portUp, portPos);

        //
        // Render the status light

        float size = 6.0f;
        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = Building::s_controlPad->GetMarkerWorldMatrix(Building::s_controlPadStatus, mat).pos;
        statusPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
        statusPos.y += 5.0f;

        WorldObjectId occupantId = const_cast<SpawnPoint&>(sp).GetPortOccupant(i);
        if (!occupantId.IsValid())
        {
            glColor4ub(150, 150, 150, 255);
        }
        else
        {
            RGBAColour teamColour = g_context->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv(teamColour.GetData());
        }

        glTexCoord2i(0, 0);
        glVertex3fv((statusPos - camR - camU).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((statusPos + camR - camU).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((statusPos + camR + camU).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((statusPos - camR + camU).GetData());
    }

    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
}
