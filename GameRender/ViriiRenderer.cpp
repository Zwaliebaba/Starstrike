#include "pch.h"

#include "ViriiRenderer.h"
#include "virii.h"
#include "entity.h"
#include "camera.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "resource.h"
#include "preferences.h"
#include "team.h"
#include "GameApp.h"

void ViriiRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Virii& v = static_cast<const Virii&>(_entity);

    float predictionTime = _ctx.predictionTime + SERVER_ADVANCE_PERIOD;

    LegacyVector3 predictedPos = v.m_pos + v.m_vel * predictionTime;

    int _detail = 1;
    int entityDetail = g_prefsManager->GetInt("RenderEntityDetail");
    if (entityDetail == 3)
    {
        float rangeToCam = (v.m_pos - g_app->m_camera->GetPos()).Mag();
        if (rangeToCam > 1000.0f)
            _detail = 4;
        else if (rangeToCam > 600.0f)
            _detail = 3;
        else if (rangeToCam > 300.0f)
            _detail = 2;
    }
    else
    {
        float rangeToCam = (v.m_pos - g_app->m_camera->GetPos()).Mag();
        if (entityDetail == 1 && rangeToCam > 1000.0f)
            _detail = 2;
        else if (entityDetail == 2 && rangeToCam > 1000.0f)
            _detail = 3;
        else if (entityDetail == 2 && rangeToCam > 500.0f)
            _detail = 2;
    }

    if (v.m_onGround && _detail == 1)
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z) + v.m_hoverHeight;

    float health = static_cast<float>(v.m_stats[Entity::StatHealth]) / EntityBlueprint::GetStat(v.m_type, Entity::StatHealth);

    RGBAColour wormColour = g_app->m_location->m_teams[v.m_id.GetTeamId()].m_colour * health;
    RGBAColour glowColour(200, 100, 100);
    wormColour.a = 200;
    glowColour.a = 150;

    if (v.m_dead)
    {
        wormColour.r = wormColour.g = wormColour.b = 250;
        wormColour.a = 100 * v.m_stats[Entity::StatHealth] / 100.0f;

        glowColour.r = glowColour.g = glowColour.b = 250;
        glowColour.a = 250 * v.m_stats[Entity::StatHealth] / 100.0f;
    }

    LegacyVector3 landNormal = g_upVector;
    if (_detail == 1)
        landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);

    ViriiHistory prevPos;
    prevPos.m_pos = predictedPos;
    prevPos.m_right = -v.m_front ^ landNormal;
    LegacyVector3 firstPos;
    if (v.m_positionHistory.Size() > 0)
        firstPos = v.m_positionHistory[0]->m_pos;
    prevPos.m_distance = (predictedPos - firstPos).Mag();
    prevPos.m_glowDiff = (predictedPos - firstPos).SetLength(10.0f);

#define wormWidth       3.0f
#define wormTexW        32.0f/(32.0f+128.0f)
#define wormTexHeight   (10.0f * 2)/1024.0f

#define glowWidth       12.0f
#define glowTexXpos     32.0f/(32.0f+128.0f)
#define glowTexH        128.0f/512.0f

    float wormTexYpos = 0.0f;
    float skippedDistance = 0.0f;

    int lastIndex = v.m_positionHistory.Size();
    if (_detail > 1)
    {
        int amountToChop = _detail - 1;
        amountToChop = min(amountToChop, 2);
        lastIndex *= (1.0f - 0.25f * amountToChop);
    }

    glBegin(GL_QUADS);

    for (int i = 0; i < lastIndex; i++)
    {
        ViriiHistory* history = v.m_positionHistory[i];

        if (!history->m_required)
        {
            skippedDistance += history->m_distance;
            continue;
        }

        LegacyVector3& pos = history->m_pos;
        LegacyVector3 wormRightAngle = prevPos.m_right * wormWidth;
        LegacyVector3 glowRightAngle = prevPos.m_right * glowWidth;
        float distance = prevPos.m_distance + skippedDistance;
        const LegacyVector3& glowDiff = prevPos.m_glowDiff;

        skippedDistance = 0.0f;

        //
        // Worm shape

        int newCol = wormColour.a - distance;
        if (newCol < 0)
            newCol = 0;
        wormColour.a = newCol;
        glColor4ubv(wormColour.GetData());

        glTexCoord2f(0.0f, wormTexYpos);
        glVertex3fv((prevPos.m_pos - wormRightAngle).GetData());
        glTexCoord2f(wormTexW, wormTexYpos);
        glVertex3fv((prevPos.m_pos + wormRightAngle).GetData());
        wormTexYpos += (distance * 6) / 512.0f;
        glTexCoord2f(wormTexW, wormTexYpos);
        glVertex3fv((pos + wormRightAngle).GetData());
        glTexCoord2f(0.0f, wormTexYpos);
        glVertex3fv((pos - wormRightAngle).GetData());

        //
        // Glow effect

        if (_detail < 4)
        {
            newCol = glowColour.a - distance;
            if (newCol < 0)
                newCol = 0;
            glowColour.a = newCol;
            glColor4ubv(glowColour.GetData());

            glTexCoord2f(glowTexXpos, 0.0f);
            glVertex3fv((prevPos.m_pos - glowRightAngle + glowDiff).GetData());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3fv((prevPos.m_pos + glowRightAngle + glowDiff).GetData());

            glTexCoord2f(1.0f, glowTexH);
            glVertex3fv((pos + glowRightAngle - glowDiff).GetData());
            glTexCoord2f(glowTexXpos, glowTexH);
            glVertex3fv((pos - glowRightAngle - glowDiff).GetData());
        }

        prevPos = *history;
    }

    glEnd();

#undef wormWidth
#undef wormTexW
#undef wormTexHeight
#undef glowWidth
#undef glowTexXpos
#undef glowTexH
}
