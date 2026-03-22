#include "pch.h"

#include "BoxKiteRenderer.h"
#include "darwinian.h"
#include "ShapeStatic.h"
#include "renderer.h"
#include "camera.h"
#include "resource.h"
#include "GameApp.h"
#include "hi_res_time.h"
#include "main.h"


void BoxKiteRenderer::Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx)
{
    const BoxKite& kite = static_cast<const BoxKite&>(_object);

    glDisable(GL_BLEND);
    glDepthMask(true);

    LegacyVector3 predictedPos = kite.m_pos + kite.m_vel * _ctx.predictionTime;

    float scale = (GetHighResTime() - kite.m_birthTime) / 10.0f;
    if (scale > kite.m_size)
        scale = kite.m_size;

    if (kite.m_deathTime != 0.0f && GetHighResTime() > kite.m_deathTime - 20.0f)
    {
        float deathScale = (kite.m_deathTime - GetHighResTime()) / 20.0f;
        if (deathScale < 0.0f)
            deathScale = 0.0f;
        scale = deathScale * kite.m_size;
    }

    g_app->m_renderer->SetObjectLighting();
    Matrix34 mat(kite.m_front, kite.m_up, predictedPos);
    mat.u *= scale;
    mat.r *= scale;
    mat.f *= scale;
    kite.m_shape->Render(_ctx.predictionTime, mat);
    g_app->m_renderer->UnsetObjectLighting();

    //
    // Candle in the middle

    glEnable(GL_BLEND);
    glDepthMask(false);

    LegacyVector3 camUp = g_app->m_camera->GetUp() * 3.0f * scale * kite.m_brightness;
    LegacyVector3 camRight = g_app->m_camera->GetRight() * 3.0f * scale * kite.m_brightness;

    glColor4f(1.0f, 0.75f, 0.2f, kite.m_brightness);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/starburst.bmp"));

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - camRight + camUp).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + camRight + camUp).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + camRight - camUp).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - camRight - camUp).GetData());
    glEnd();

    camUp *= 0.2f;
    camRight *= 0.2f;

    glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex3fv((predictedPos - camRight + camUp).GetData());
    glTexCoord2i(1, 0);
    glVertex3fv((predictedPos + camRight + camUp).GetData());
    glTexCoord2i(1, 1);
    glVertex3fv((predictedPos + camRight - camUp).GetData());
    glTexCoord2i(0, 1);
    glVertex3fv((predictedPos - camRight - camUp).GetData());
    glEnd();

    glDisable(GL_TEXTURE_2D);
}
