#include "pch.h"

#include "SpaceInvaderRenderer.h"
#include "airstrike.h"
#include "ShapeStatic.h"
#include "renderer.h"
#include "GameApp.h"


void SpaceInvaderRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const SpaceInvader& invader = static_cast<const SpaceInvader&>(_entity);

    LegacyVector3 predictedPos = invader.m_pos + invader.m_vel * _ctx.predictionTime;
    glDisable(GL_TEXTURE_2D);

    g_context->m_renderer->SetObjectLighting();

    Matrix34 mat(invader.m_front, g_upVector, predictedPos);
    mat.f *= 2.0f;
    mat.u *= 2.0f;
    mat.r *= 2.0f;
    invader.m_shape->Render(_ctx.predictionTime, mat);

    if (invader.m_armed)
    {
        mat = Matrix34(-g_upVector, invader.m_front, predictedPos - g_upVector * 12.0f);
        invader.m_bombShape->Render(_ctx.predictionTime, mat);
    }

    g_context->m_renderer->UnsetObjectLighting();
}
