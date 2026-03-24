#include "pch.h"

#include "LaserTrooperRenderer.h"
#include "lasertrooper.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "entity.h"

void LaserTrooperRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const LaserTrooper& trooper = static_cast<const LaserTrooper&>(_entity);

    if (!trooper.m_enabled)
        return;

    //
    // Work out our predicted position and orientation

    LegacyVector3 predictedPos = trooper.m_pos + trooper.m_vel * _ctx.predictionTime;
    if (trooper.m_onGround && trooper.m_inWater == -1)
        predictedPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

    float size = 2.0f;

    LegacyVector3 entityUp = g_context->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
    entityUp.Normalise();
    LegacyVector3 entityFront = trooper.m_front;
    entityFront.RotateAround(trooper.m_angVel * _ctx.predictionTime);

    LegacyVector3 entityRight = entityFront ^ entityUp;
    entityFront *= 0.2f;
    entityUp *= size * 2.0f;
    entityRight.SetLength(size);

    int teamId = trooper.m_id.GetTeamId();

    if (!trooper.m_dead)
    {
        RGBAColour colour = g_context->m_location->m_teams[teamId].m_colour;

        if (trooper.m_reloading > 0.0f)
        {
            colour.r *= 0.7f;
            colour.g *= 0.7f;
            colour.b *= 0.7f;
        }

        //
        // Draw our texture

        float maxHealth = EntityBlueprint::GetStat(trooper.m_type, Entity::StatHealth);
        float health = static_cast<float>(trooper.m_stats[Entity::StatHealth]) / maxHealth;
        if (health > 1.0f)
            health = 1.0f;

        colour *= 0.5f + 0.5f * health;
        glColor3ubv(colour.GetData());
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3fv((predictedPos - entityRight + entityUp).GetData());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3fv((predictedPos + entityRight + entityUp).GetData());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3fv((predictedPos + entityRight).GetData());
        glTexCoord2f(0.0f, 0.0f);
        glVertex3fv((predictedPos - entityRight).GetData());
        glEnd();

        //
        // Draw our shadow on the landscape

        if (trooper.m_onGround)
        {
            glColor4ub(0, 0, 0, 90);
            LegacyVector3 pos1 = predictedPos - entityRight;
            LegacyVector3 pos2 = predictedPos + entityRight;
            LegacyVector3 pos4 = pos1 + LegacyVector3(0.0f, 0.0f, size * 2.0f);
            LegacyVector3 pos3 = pos2 + LegacyVector3(0.0f, 0.0f, size * 2.0f);

            pos1.y = 0.2f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos1.x, pos1.z);
            pos2.y = 0.2f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos2.x, pos2.z);
            pos3.y = 0.2f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos3.x, pos3.z);
            pos4.y = 0.2f + g_context->m_location->m_landscape.m_heightMap->GetValue(pos4.x, pos4.z);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3fv(pos1.GetData());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3fv(pos2.GetData());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3fv(pos3.GetData());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3fv(pos4.GetData());
            glEnd();
        }

        //
        // Draw a line through us if we are side-on with the camera

        float alpha = 1.0f - fabsf(g_context->m_camera->GetFront() * trooper.m_front);
        if (alpha > 0.5f)
        {
            colour.a = 255 * alpha;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4ubv(colour.GetData());
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3fv((predictedPos - entityFront * 1.5f + entityUp).GetData());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3fv((predictedPos + entityFront * 1.5f + entityUp).GetData());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3fv((predictedPos + entityFront * 1.5f).GetData());
            glTexCoord2f(0.0f, 0.0f);
            glVertex3fv((predictedPos - entityFront * 1.5f).GetData());
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    else
    {
        entityFront = trooper.m_front;
        entityFront.Normalise();
        entityUp = g_upVector;
        entityRight = entityFront ^ entityUp;
        entityUp *= size * 2.0f;
        entityRight.Normalise();
        entityRight *= size;
        unsigned char alpha = static_cast<float>(trooper.m_stats[Entity::StatHealth]) * 2.55f;

        glColor4ub(0, 0, 0, alpha);

        entityRight *= 0.5f;
        entityUp *= 0.5;
        float predictedHealth = trooper.m_stats[Entity::StatHealth];
        if (trooper.m_onGround)
            predictedHealth -= 40 * _ctx.predictionTime;
        else
            predictedHealth -= 20 * _ctx.predictionTime;
        float landHeight = g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

        for (int i = 0; i < 3; ++i)
        {
            LegacyVector3 fragmentPos = predictedPos;
            if (i == 0)
                fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if (i == 1)
                fragmentPos.z += 10.0f - predictedHealth / 10.0f;
            if (i == 2)
                fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
            fragmentPos.y += (fragmentPos.y - landHeight) * i * 0.5f;

            float left = 0.0f;
            float right = 1.0f;
            float top = 1.0f;
            float bottom = 0.0f;

            if (i == 0)
            {
                right -= (right - left) / 2;
                bottom -= (bottom - top) / 2;
            }
            if (i == 1)
            {
                left += (right - left) / 2;
                bottom -= (bottom - top) / 2;
            }
            if (i == 2)
            {
                top += (bottom - top) / 2;
                left += (right - left) / 2;
            }

            glBegin(GL_QUADS);
            glTexCoord2f(left, bottom);
            glVertex3fv((fragmentPos - entityRight + entityUp).GetData());
            glTexCoord2f(right, bottom);
            glVertex3fv((fragmentPos + entityRight + entityUp).GetData());
            glTexCoord2f(right, top);
            glVertex3fv((fragmentPos + entityRight).GetData());
            glTexCoord2f(left, top);
            glVertex3fv((fragmentPos - entityRight).GetData());
            glEnd();
        }
    }
}
