#include "pch.h"

#include "SoulDestroyerRenderer.h"
#include "ShadowRenderer.h"
#include "QuadBatcher.h"
#include "souldestroyer.h"
#include "ShapeStatic.h"
#include "matrix34.h"
#include "renderer.h"
#include "camera.h"
#include "hi_res_time.h"
#include "GameApp.h"
#include "globals.h"

void SoulDestroyerRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    const SoulDestroyer& sd = static_cast<const SoulDestroyer&>(_entity);

    float predictionTime = _ctx.predictionTime - SERVER_ADVANCE_PERIOD;

    if (sd.m_dead)
        return;

    glDisable(GL_TEXTURE_2D);

    LegacyVector3 predictedPos = sd.m_pos + sd.m_vel * predictionTime;
    LegacyVector3 predictedFront = sd.m_front;
    LegacyVector3 predictedUp = sd.m_up;
    LegacyVector3 predictedRight = predictedUp ^ predictedFront;
    predictedFront = predictedRight ^ predictedUp;

    RenderShapes(_entity, predictionTime);

    //
    // Render shadows

    ShadowRenderer::Begin();
    ShadowRenderer::Render(predictedPos, 50.0f);

    for (int i = 1; i < sd.m_positionHistory.Size(); i += 1)
    {
        LegacyVector3 pos1 = *sd.m_positionHistory.GetPointer(i);
        LegacyVector3 pos2 = *sd.m_positionHistory.GetPointer(i - 1);

        LegacyVector3 pos = pos1 + (pos2 - pos1);
        LegacyVector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
        pos += vel * predictionTime;

        float scale = 1.0f - (static_cast<float>(i) / static_cast<float>(sd.m_positionHistory.Size()));
        scale *= 1.5f;
        if (i == sd.m_positionHistory.Size() - 1)
            scale = 0.8f;
        scale = std::max(scale, 0.5f);

        ShadowRenderer::Render(pos, scale * 20.0f);
    }

    ShadowRenderer::End();

    //
    // Render our spirits

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    glDepthMask(false);

    // TODO Phase 6: eliminate mutation — m_spirits.MarkNotUsed() should move to Advance()
    SoulDestroyer& mutableSd = const_cast<SoulDestroyer&>(sd);

    float timeNow = GetHighResTime();
    for (int i = 0; i < sd.m_spirits.Size(); ++i)
    {
        if (sd.m_spirits.ValidIndex(i))
        {
            float birthTime = sd.m_spirits[i];
            if (timeNow >= birthTime + 60.0f)
                mutableSd.m_spirits.MarkNotUsed(i);
            else
            {
                float alpha = 1.0f - (timeNow - sd.m_spirits[i]) / 60.0f;
                alpha = std::min(alpha, 1.0f);
                alpha = std::max(alpha, 0.0f);
                LegacyVector3 pos = sd.m_pos + sd.m_spiritPosition[i];
                pos += sd.m_vel * predictionTime;
                RenderSpirit(pos, alpha);
            }
        }
    }

    QuadBatcher::Get().Flush();

    glDepthMask(true);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void SoulDestroyerRenderer::RenderShapes(const Entity& _entity, float _predictionTime)
{
    const SoulDestroyer& sd = static_cast<const SoulDestroyer&>(_entity);

    LegacyVector3 predictedPos = sd.m_pos + sd.m_vel * _predictionTime;
    LegacyVector3 predictedFront = sd.m_front;
    LegacyVector3 predictedUp = sd.m_up;
    LegacyVector3 predictedRight = predictedUp ^ predictedFront;
    predictedFront = predictedRight ^ predictedUp;
    predictedFront.Normalise();
    predictedUp.Normalise();
    Matrix34 mat(predictedFront, predictedUp, predictedPos);

    glEnable(GL_NORMALIZE);
    glDisable(GL_TEXTURE_2D);

    g_context->m_renderer->SetObjectLighting();

    SoulDestroyer::s_shapeHead->Render(_predictionTime, mat);

    for (int i = 1; i < sd.m_positionHistory.Size(); i += 1)
    {
        LegacyVector3 pos1 = *sd.m_positionHistory.GetPointer(i);
        LegacyVector3 pos2 = *sd.m_positionHistory.GetPointer(i - 1);

        LegacyVector3 pos = pos1 + (pos2 - pos1);
        LegacyVector3 front = (pos2 - pos1).Normalise();
        LegacyVector3 right = front ^ g_upVector;
        LegacyVector3 up = right ^ front;
        LegacyVector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
        pos += vel * _predictionTime;

        float scale = 1.0f - (static_cast<float>(i) / static_cast<float>(sd.m_positionHistory.Size()));
        scale *= 1.5f;
        if (i == sd.m_positionHistory.Size() - 1)
            scale = 0.8f;
        scale = std::max(scale, 0.5f);

        Matrix34 tailMat(front, up, pos);
        tailMat.u *= scale;
        tailMat.r *= scale;
        tailMat.f *= scale;

        SoulDestroyer::s_shapeTail->Render(_predictionTime, tailMat);
    }

    g_context->m_renderer->UnsetObjectLighting();

    glDisable(GL_NORMALIZE);
}

void SoulDestroyerRenderer::RenderSpirit(const LegacyVector3& _pos, float _alpha)
{
    LegacyVector3 pos = _pos;
    int innerAlpha = 200 * _alpha;
    int outerAlpha = 100 * _alpha;
    int spiritInnerSize = 4 * _alpha;
    int spiritOuterSize = 12 * _alpha;

    RGBAColour colour(100, 50, 50);
    float distToParticle = (g_context->m_camera->GetPos() - pos).Mag();

    LegacyVector3 camUp = g_context->m_camera->GetUp();
    LegacyVector3 camRight = g_context->m_camera->GetRight();

    auto& batcher = QuadBatcher::Get();
    auto makeVtx = [](const LegacyVector3& p, uint32_t c) {
        return QuadBatcher::MakeVertex(p.x, p.y, p.z, c);
    };

    // Inner glow
    float size = spiritInnerSize / sqrtf(sqrtf(distToParticle));
    uint32_t innerColor = QuadBatcher::PackColorBGRA(colour.r, colour.g, colour.b, innerAlpha);
    batcher.Emit(
        makeVtx(pos - camUp * size, innerColor),
        makeVtx(pos + camRight * size, innerColor),
        makeVtx(pos + camUp * size, innerColor),
        makeVtx(pos - camRight * size, innerColor));

    // Outer glow
    size = spiritOuterSize / sqrtf(sqrtf(distToParticle));
    uint32_t outerColor = QuadBatcher::PackColorBGRA(colour.r, colour.g, colour.b, outerAlpha);
    batcher.Emit(
        makeVtx(pos - camUp * size, outerColor),
        makeVtx(pos + camRight * size, outerColor),
        makeVtx(pos + camUp * size, outerColor),
        makeVtx(pos - camRight * size, outerColor));
}
