#include "pch.h"

#include "EggRenderer.h"
#include "ShadowRenderer.h"
#include "egg.h"
#include "resource.h"
#include "GameApp.h"
#include "camera.h"
#include "location.h"
#include "team.h"

void EggRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Egg& egg = static_cast<const Egg&>(_entity);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Resource::GetTexture("sprites/egg.bmp"));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(false);
    glDisable(GL_CULL_FACE);

    RGBAColour colour;
    if (egg.m_id.GetTeamId() >= 0)
        colour = g_context->m_location->m_teams[egg.m_id.GetTeamId()].m_colour;
    float alpha = egg.m_stats[Entity::StatHealth] / EntityBlueprint::GetStat(Entity::TypeEgg, Entity::StatHealth);
    if (alpha < 0.1f)
        alpha = 0.1f;
    glColor4ub(255, 255, 255, 255 * alpha);

    LegacyVector3 pos = egg.m_pos + egg.m_vel * _ctx.predictionTime;
    pos.y += 3.0f;
    LegacyVector3 up = g_context->m_camera->GetUp();
    LegacyVector3 right = g_context->m_camera->GetRight();
    float size = 4.0f;

    //
    // Render main egg shape

    if (!egg.m_dead)
    {
        glBegin(GL_QUADS);
        glTexCoord2i(0, 0);
        glVertex3fv((pos - right * size - up * size).GetData());
        glTexCoord2i(0, 1);
        glVertex3fv((pos - right * size + up * size).GetData());
        glTexCoord2i(1, 1);
        glVertex3fv((pos + right * size + up * size).GetData());
        glTexCoord2i(1, 0);
        glVertex3fv((pos + right * size - up * size).GetData());
        glEnd();

        //
        // Render throb

        if (egg.m_state == Egg::StateFertilised)
        {
            float predictedTimer = egg.m_timer + _ctx.predictionTime;
            float throb = fabs(sinf(predictedTimer)) * 8.0f * predictedTimer;
            throb *= alpha;
            if (throb > 255)
                throb = 255;
            if (throb < 0)
                throb = 0;
            size *= 1.3f;
            glColor4ub(255, 255, 255, throb);

            glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex3fv((pos - right * size - up * size).GetData());
            glTexCoord2i(0, 1);
            glVertex3fv((pos - right * size + up * size).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((pos + right * size + up * size).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((pos + right * size - up * size).GetData());
            glEnd();

            throb = fabs(sinf(predictedTimer)) * 8 * predictedTimer;
            throb *= alpha;
            if (throb > 255)
                throb = 255;
            if (throb < 0)
                throb = 0;
            size *= 0.5f;
            pos.y += 2.0f;
            glColor4ub(255, 255, 255, throb);

            glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex3fv((pos - right * size - up * size).GetData());
            glTexCoord2i(0, 1);
            glVertex3fv((pos - right * size + up * size).GetData());
            glTexCoord2i(1, 1);
            glVertex3fv((pos + right * size + up * size).GetData());
            glTexCoord2i(1, 0);
            glVertex3fv((pos + right * size - up * size).GetData());
            glEnd();
        }
    }
    else
    {
        float predictedHealth = egg.m_stats[Entity::StatHealth];
        if (egg.m_onGround)
            predictedHealth -= 40.0f * _ctx.predictionTime;
        else
            predictedHealth -= 20.0f * _ctx.predictionTime;
        float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);

        size *= 0.5f;

        glColor4ub(255, 255, 255, predictedHealth * 2.0f);

        for (int i = 0; i < 3; ++i)
        {
            LegacyVector3 fragmentPos = pos;
            if (i == 0)
                fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if (i == 1)
                fragmentPos.z += 10.0f - predictedHealth / 10.0f;
            if (i == 2)
                fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
            fragmentPos.y += (fragmentPos.y - landHeight) * i * 0.5f;

            float tleft = 0.0f;
            float tright = 1.0f;
            float ttop = 0.0f;
            float tbottom = 1.0f;

            if (i == 0)
            {
                tright -= (tright - tleft) / 2;
                tbottom -= (tbottom - ttop) / 2;
            }
            if (i == 1)
            {
                tleft += (tright - tleft) / 2;
                tbottom -= (tbottom - ttop) / 2;
            }
            if (i == 2)
            {
                ttop += (tbottom - ttop) / 2;
                tleft += (tright - tleft) / 2;
            }

            glBegin(GL_QUADS);
            glTexCoord2f(tleft, tbottom);
            glVertex3fv((fragmentPos - right * size + up * size).GetData());
            glTexCoord2f(tright, tbottom);
            glVertex3fv((fragmentPos + right * size + up * size).GetData());
            glTexCoord2f(tright, ttop);
            glVertex3fv((fragmentPos + right * size - up * size).GetData());
            glTexCoord2f(tleft, ttop);
            glVertex3fv((fragmentPos - right * size - up * size).GetData());
            glEnd();
        }
    }

    ShadowRenderer::Begin();
    ShadowRenderer::Render(pos, size);
    ShadowRenderer::End();

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDepthMask(true);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
