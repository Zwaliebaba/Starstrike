#include "pch.h"

#include "SpiderRenderer.h"
#include "spider.h"
#include "entity_leg.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "GameApp.h"
#include "location.h"
#include "main.h"

void SpiderRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const Spider& spider = static_cast<const Spider&>(_entity);

    if (spider.m_dead)
        return;

    glDisable(GL_TEXTURE_2D);

    g_context->m_renderer->SetObjectLighting();

    //
    // Render body

    LegacyVector3 predictedMovement = _ctx.predictionTime * spider.m_vel;
    LegacyVector3 predictedPos = spider.m_pos + predictedMovement;

    LegacyVector3 up = g_context->m_location->m_landscape.m_normalMap->GetValue(spider.m_pos.x, spider.m_pos.z);
    LegacyVector3 right = spider.m_up ^ spider.m_front;
    LegacyVector3 front = right ^ up;

    Matrix34 mat(front, up, predictedPos);

    if (spider.m_renderDamaged)
    {
        float timeIndex = g_gameTime + spider.m_id.GetUniqueId() * 10;
        if (frand() > 0.5f)
            mat.r *= (1.0f + sinf(timeIndex) * 0.5f);
        else
            mat.u *= (1.0f + sinf(timeIndex) * 0.5f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    }

    spider.m_shape->Render(_ctx.predictionTime, mat);

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //
    // Render Legs

    for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
        spider.m_legs[i]->Render(_ctx.predictionTime, predictedMovement);

    g_context->m_renderer->UnsetObjectLighting();
}
