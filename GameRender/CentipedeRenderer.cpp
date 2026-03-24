#include "pch.h"

#include "CentipedeRenderer.h"
#include "centipede.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"
#include "location.h"

void CentipedeRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Centipede& centipede = static_cast<const Centipede&>(_entity);

    LegacyVector3 predictedPos = centipede.m_pos + centipede.m_vel * _ctx.predictionTime;
    predictedPos.y = g_context->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z);

    ShapeStatic* shape = centipede.m_shape;

    if (!centipede.m_dead && centipede.m_linked)
    {
        glDisable(GL_TEXTURE_2D);

        LegacyVector3 predictedFront = centipede.m_front;
        LegacyVector3 predictedUp = g_context->m_location->m_landscape.m_normalMap->GetValue(predictedPos.x, predictedPos.z);
        LegacyVector3 predictedRight = predictedUp ^ predictedFront;
        predictedFront = predictedRight ^ predictedUp;
        predictedFront.Normalise();

        Matrix34 mat(predictedFront, predictedUp, predictedPos);

        mat.f *= centipede.m_size;
        mat.u *= centipede.m_size;
        mat.r *= centipede.m_size;

        g_context->m_renderer->SetObjectLighting();
        shape->Render(_ctx.predictionTime, mat);
        g_context->m_renderer->UnsetObjectLighting();

        glDisable(GL_NORMALIZE);
    }
}
