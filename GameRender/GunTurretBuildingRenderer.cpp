#include "pch.h"
#include "GunTurretBuildingRenderer.h"

#include "gunturret.h"
#include "camera.h"
#include "GameApp.h"
#include "globals.h"
#include "location.h"
#include "renderer.h"
#include "resource.h"
#include "ShapeStatic.h"
#include "team.h"

void GunTurretBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    auto& turret = static_cast<const GunTurret&>(_building);

    LegacyVector3 turretFront = turret.m_turretFront;
    if (g_context->m_editing)
        turretFront = turret.m_front;

    Matrix34 turretPos(turretFront, g_upVector, turret.m_pos);
    turret.m_turret->Render(_ctx.predictionTime, turretPos);

    LegacyVector3 barrelPos = turret.m_turret->GetMarkerWorldMatrix(turret.m_barrelMount, turretPos).pos;
    LegacyVector3 barrelFront = turret.m_turret->GetMarkerWorldMatrix(turret.m_barrelMount, turretPos).f;
    LegacyVector3 barrelRight = barrelFront ^ turret.m_barrelUp;
    barrelFront = turret.m_barrelUp ^ barrelRight;
    barrelFront.Normalise();
    Matrix34 barrelMat(barrelFront, turret.m_barrelUp, barrelPos);
    turret.m_barrel->Render(_ctx.predictionTime, barrelMat);
}

void GunTurretBuildingRenderer::RenderPorts(const Building& _building)
{
    auto& turret = static_cast<const GunTurret&>(_building);

    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/starburst.bmp"));
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    int numPorts = turret.m_ports.Size();

    for (int i = 0; i < numPorts; ++i)
    {
        Matrix34 rootMat(turret.m_front, turret.m_up, turret.m_pos);
        Matrix34 worldMat = turret.m_shape->GetMarkerWorldMatrix(turret.m_statusMarkers[i], rootMat);

        //
        // Render the status light

        float size = 6.0f;
        LegacyVector3 camR = g_context->m_camera->GetRight() * size;
        LegacyVector3 camU = g_context->m_camera->GetUp() * size;

        LegacyVector3 statusPos = worldMat.pos;

        WorldObjectId occupantId = turret.m_ports[i]->m_occupant;
        if (!occupantId.IsValid())
            glColor4ub(150, 150, 150, 255);
        else
        {
            RGBAColour teamColour = g_context->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv(teamColour.GetData());
        }

        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((statusPos - camR - camU).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((statusPos + camR - camU).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((statusPos + camR + camU).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((statusPos - camR + camU).GetData());
        glEnd();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
}
