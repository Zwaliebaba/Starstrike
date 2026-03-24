#include "pch.h"

#include "OfficerRenderer.h"
#include "FlagRenderer.h"
#include "officer.h"
#include "ShapeStatic.h"
#include "resource.h"
#include "GameApp.h"
#include "camera.h"
#include "location.h"
#include "hi_res_time.h"
#include "main.h"

void OfficerRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Officer& officer = static_cast<const Officer&>(_entity);

    RenderShield(officer, _ctx.predictionTime);

    if (officer.m_enabled)
        RenderFlag(officer, _ctx.predictionTime);
}

void OfficerRenderer::RenderSpirit(const LegacyVector3& _pos)
{
    LegacyVector3 pos = _pos;

    int innerAlpha = 255;
    int outerAlpha = 40;
    int glowAlpha = 20;

    float spiritInnerSize = 0.5f;
    float spiritOuterSize = 1.5f;
    float spiritGlowSize = 10;

    float size = spiritInnerSize;
    glColor4ub(100, 250, 100, innerAlpha);

    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glVertex3fv((pos - g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();

    size = spiritOuterSize;
    glColor4ub(100, 250, 100, outerAlpha);

    glBegin(GL_QUADS);
    glVertex3fv((pos - g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetRight() * size).GetData());
    glVertex3fv((pos + g_context->m_camera->GetUp() * size).GetData());
    glVertex3fv((pos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();

    size = spiritGlowSize;
    glColor4ub(100, 250, 100, glowAlpha);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("textures/glow.bmp"));
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((pos - g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((pos + g_context->m_camera->GetRight() * size).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((pos + g_context->m_camera->GetUp() * size).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((pos - g_context->m_camera->GetRight() * size).GetData());
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void OfficerRenderer::RenderShield(const Officer& _officer, float _predictionTime)
{
    float timeIndex = g_gameTime + _officer.m_id.GetUniqueId() * 20;

    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    LegacyVector3 predictedPos = _officer.m_pos + _officer.m_vel * _predictionTime;
    predictedPos.y += 10.0f;

    for (int i = 0; i < _officer.m_shield; ++i)
    {
        LegacyVector3 spiritPos = predictedPos;
        spiritPos.x += sinf(timeIndex * 1.8f + i * 2.0f) * 10.0f;
        spiritPos.y += cosf(timeIndex * 2.1f + i * 1.2f) * 6.0f;
        spiritPos.z += sinf(timeIndex * 2.4f + i * 1.4f) * 10.0f;

        RenderSpirit(spiritPos);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glDepthMask(true);
    glEnable(GL_CULL_FACE);
}

void OfficerRenderer::RenderFlag(const Officer& _officer, float _predictionTime)
{
    float timeIndex = g_gameTime + _officer.m_id.GetUniqueId() * 10;
    float size = 20.0f;

    LegacyVector3 up = g_upVector;
    LegacyVector3 front = _officer.m_front * -1;
    front.y = 0;
    front.Normalise();

    if (_officer.m_orders != Officer::OrderNone)
        up.RotateAround(front * sinf(timeIndex * 2) * 0.3f);

    LegacyVector3 entityUp = g_upVector;
    LegacyVector3 entityRight(_officer.m_front ^ entityUp);
    LegacyVector3 entityFront = entityUp ^ entityRight;
    Matrix34 mat(entityFront, entityUp, _officer.m_pos + _officer.m_vel * _predictionTime);
    LegacyVector3 flagPos = _officer.m_shape->GetMarkerWorldMatrix(_officer.m_flagMarker, mat).pos;

    int texId = -1;
    if (_officer.m_orders == Officer::OrderNone)
        texId = Resource::GetTexture("icons/banner_none.bmp");
    else if (_officer.m_orders == Officer::OrderGoto)
        texId = Resource::GetTexture("icons/banner_goto.bmp");
    else if (_officer.m_orders == Officer::OrderFollow && !_officer.m_absorb)
        texId = Resource::GetTexture("icons/banner_follow.bmp");
    else
        if (_officer.m_orders == Officer::OrderFollow && _officer.m_absorb)
            texId = Resource::GetTexture("icons/banner_absorb.bmp");

    // Flag is a mutable object on the Officer — we need a non-const copy
    // to call SetTexture/SetPosition/SetOrientation/SetSize/Render.
    // This is a temporary compromise during the transition; the Flag
    // will eventually move to the renderer's own state.
    Flag flag = _officer.m_flag;
    flag.SetTexture(texId);
    flag.SetPosition(flagPos);
    flag.SetOrientation(front, up);
    flag.SetSize(size);
    FlagRenderer::Render(flag);
}
