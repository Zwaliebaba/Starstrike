#include "pch.h"

#include "TripodRenderer.h"
#include "tripod.h"
#include "entity_leg.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"

void TripodRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Tripod& tripod = static_cast<const Tripod&>(_entity);

    glDisable(GL_TEXTURE_2D);

    g_app->m_renderer->SetObjectLighting();

    //
    // Render body

    LegacyVector3 predictedMovement = _ctx.predictionTime * tripod.m_vel;
    LegacyVector3 predictedPos = tripod.m_pos + predictedMovement;

    Matrix34 mat(tripod.m_front, tripod.m_up, predictedPos);
    tripod.m_shape->Render(_ctx.predictionTime, mat);

    //
    // Render Legs

    for (int i = 0; i < 3; ++i)
        tripod.m_legs[i]->Render(_ctx.predictionTime, predictedMovement);

    g_app->m_renderer->UnsetObjectLighting();
}
