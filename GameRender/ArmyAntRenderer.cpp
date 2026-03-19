#include "pch.h"

#include "ArmyAntRenderer.h"
#include "armyant.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"

void ArmyAntRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const ArmyAnt& ant = static_cast<const ArmyAnt&>(_entity);

    if (ant.m_dead)
        return;

    LegacyVector3 predictedPos = ant.m_pos + ant.m_vel * _ctx.predictionTime;
    LegacyVector3 predictedUp = g_upVector;

    g_app->m_renderer->SetObjectLighting();
    glDisable(GL_TEXTURE_2D);

    Matrix34 mat(ant.m_front, predictedUp, predictedPos);
    mat.u *= ant.m_scale;
    mat.f *= ant.m_scale;
    mat.r *= ant.m_scale;

    ant.m_shape->Render(_ctx.predictionTime, mat);

    g_app->m_renderer->UnsetObjectLighting();
}
