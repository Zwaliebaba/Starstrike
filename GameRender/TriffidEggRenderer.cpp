#include "pch.h"
#include "TriffidEggRenderer.h"
#include "ShadowRenderer.h"
#include "triffid.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"
#include "hi_res_time.h"
#include "main.h"

void TriffidEggRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const TriffidEgg& egg = static_cast<const TriffidEgg&>(_entity);

    if (egg.m_dead)
        return;

    LegacyVector3 predictedPos = egg.m_pos + egg.m_vel * _ctx.predictionTime;

    LegacyVector3 direction = egg.m_vel;
    LegacyVector3 right = (g_upVector ^ direction).Normalise();
    LegacyVector3 up = egg.m_up;
    up.RotateAround(right * _ctx.predictionTime * egg.m_force * egg.m_force * 30.0f);
    LegacyVector3 front = right ^ up;
    up.Normalise();
    front.Normalise();

    //
    // Make our size pulsate a little
    float age = (egg.m_timerSync - GetHighResTime()) / egg.m_life;
    age = std::max(age, 0.0f);
    age = std::min(age, 1.0f);
    float size = egg.m_size + fabs(sinf(g_gameTime * 2.0f)) * (1.0f - age) * 0.4f;

    predictedPos.y -= size;
    Matrix34 transform(front, up, predictedPos);
    transform.f *= size;
    transform.u *= size;
    transform.r *= size;

    glEnable(GL_NORMALIZE);
    g_context->m_renderer->SetObjectLighting();
    egg.m_shape->Render(_ctx.predictionTime, transform);
    g_context->m_renderer->UnsetObjectLighting();
    glDisable(GL_NORMALIZE);

    //
    // Render our shadow

    ShadowRenderer::Begin();
    ShadowRenderer::Render(predictedPos, size * 10.0f);
    ShadowRenderer::End();
}
