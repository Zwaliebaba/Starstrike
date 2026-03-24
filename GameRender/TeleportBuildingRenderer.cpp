#include "pch.h"
#include "TeleportBuildingRenderer.h"

#include "teleport.h"
#include "camera.h"
#include "GameApp.h"
#include "globals.h"
#include "location.h"
#include "team.h"

void TeleportBuildingRenderer::RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx)
{
    auto& teleport = static_cast<const Teleport&>(_building);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);

    for (int i = 0; i < teleport.m_inTransit.Size(); ++i)
    {
        const WorldObjectId* id = teleport.m_inTransit.GetPointer(i);
        if (!id) continue;
        WorldObject* obj = g_context->m_location->GetEntity(*id);
        if (obj)
        {
            Entity* ent = static_cast<Entity*>(obj);
            LegacyVector3 pos = obj->m_pos + obj->m_vel * _ctx.predictionTime;
            RenderSpirit(pos, ent->m_id.GetTeamId());
        }
    }

    glDepthMask(true);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DefaultBuildingRenderer::RenderAlphas(_building, _ctx);
}

void TeleportBuildingRenderer::RenderSpirit(const LegacyVector3& _pos, int _teamId)
{
    LegacyVector3 pos = _pos;

    int innerAlpha = 100;
    int outerAlpha = 50;
    int spiritInnerSize = 2;
    int spiritOuterSize = 6;

    RGBAColour colour;
    if (_teamId >= 0)
        colour = g_context->m_location->m_teams[_teamId].m_colour;

    float size = spiritInnerSize;
    glColor4ub(colour.r, colour.g, colour.b, innerAlpha);

    glBegin(GL_QUADS);
    glVertex3fv((pos - g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();

    size = spiritOuterSize;
    glColor4ub(colour.r, colour.g, colour.b, outerAlpha);
    glBegin(GL_QUADS);
    glVertex3fv((pos - g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();
}
