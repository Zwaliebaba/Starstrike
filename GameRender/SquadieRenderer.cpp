#include "pch.h"

#include "SquadieRenderer.h"
#include "insertion_squad.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"
#include "location.h"
#include "landscape.h"
#include "main.h"

void SquadieRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Squadie& squadie = static_cast<const Squadie&>(_entity);

    if (!squadie.m_enabled)
        return;

    //
    // Work out our predicted position and orientation

    LegacyVector3 predictedPos = squadie.m_pos + squadie.m_vel * _ctx.predictionTime;
    if (squadie.m_onGround)
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

    LegacyVector3 entityUp = g_upVector;
    LegacyVector3 entityFront = squadie.m_front;
    entityFront.Normalise();
    LegacyVector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    if (!squadie.m_dead)
    {
        //
        // 3d Shape

        g_app->m_renderer->SetObjectLighting();
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_COLOR_MATERIAL);
        glDisable(GL_BLEND);

        Matrix34 mat(entityFront, entityUp, predictedPos);

        //
        // If we are damaged, flicker in and out based on our health

        if (squadie.m_renderDamaged)
        {
            float timeIndex = g_gameTime + squadie.m_id.GetUniqueId() * 10;
            float thefrand = frand();
            if (thefrand > 0.7f)
                mat.f *= (1.0f - sinf(timeIndex) * 0.5f);
            else if (thefrand > 0.4f)
                mat.u *= (1.0f - sinf(timeIndex) * 0.2f);
            else
                mat.r *= (1.0f - sinf(timeIndex) * 0.5f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
        }

        squadie.m_shape->Render(_ctx.predictionTime, mat);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_COLOR_MATERIAL);
        glEnable(GL_TEXTURE_2D);
        g_app->m_renderer->UnsetObjectLighting();
    }
}
