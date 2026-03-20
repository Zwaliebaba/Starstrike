#include "pch.h"

#include "LanderRenderer.h"
#include "lander.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"
#include "location.h"
#include "team.h"

void LanderRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Lander& lander = static_cast<const Lander&>(_entity);

    if (lander.m_dead)
        return;

    //
    // Work out our predicted position and orientation

    LegacyVector3 predictedPos = lander.m_pos + lander.m_vel * _ctx.predictionTime;

    LegacyVector3 entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
    LegacyVector3 entityFront = lander.m_front;
    entityFront.Normalise();
    LegacyVector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    int teamId = lander.m_id.GetTeamId();
    RGBAColour colour = g_app->m_location->m_teams[teamId].m_colour;

    if (lander.m_reloading > 0.0f)
    {
        colour.r = (colour.r == 255 ? 255 : 100);
        colour.g = (colour.g == 255 ? 255 : 100);
        colour.b = (colour.b == 255 ? 255 : 100);
    }
    glColor3ubv(colour.GetData());

    //
    // 3d Shape

    g_app->m_renderer->SetObjectLighting();

    glEnable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    Matrix34 mat(entityFront, entityUp, predictedPos);
    lander.m_shape->Render(_ctx.predictionTime, mat);

    glEnable(GL_BLEND);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    g_app->m_renderer->UnsetObjectLighting();
    glEnable(GL_CULL_FACE);
}
