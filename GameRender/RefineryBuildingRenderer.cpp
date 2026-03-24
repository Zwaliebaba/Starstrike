#include "pch.h"

#include "RefineryBuildingRenderer.h"
#include "mine.h"
#include "GameApp.h"
#include "global_world.h"
#include "main.h"
#include "text_renderer.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// RefineryBuildingRenderer::Render
// ---------------------------------------------------------------------------

void RefineryBuildingRenderer::Render(const Building& _building,
                                       const BuildingRenderContext& _ctx)
{
    MineBuildingRenderer::Render(_building, _ctx);

    auto& refinery = static_cast<const Refinery&>(_building);

    //
    // Render wheels

    Matrix34 refineryMat(refinery.m_front, g_upVector, refinery.m_pos);
    Matrix34 wheel1Mat =
        refinery.m_shape->GetMarkerWorldMatrix(refinery.m_wheel1, refineryMat);
    Matrix34 wheel2Mat =
        refinery.m_shape->GetMarkerWorldMatrix(refinery.m_wheel2, refineryMat);
    Matrix34 wheel3Mat =
        refinery.m_shape->GetMarkerWorldMatrix(refinery.m_wheel3, refineryMat);

    float refinerySpeed = MineBuilding::RefinerySpeed();
    float predictedWheelRotate =
        refinery.m_wheelRotate - 3.0f * refinerySpeed * _ctx.predictionTime;

    wheel1Mat.RotateAroundF(predictedWheelRotate);
    wheel2Mat.RotateAroundF(predictedWheelRotate * -1.0f);
    wheel3Mat.RotateAroundF(predictedWheelRotate);

    wheel1Mat.r *= 1.3f;
    wheel1Mat.u *= 1.3f;
    wheel1Mat.f *= 1.3f;

    wheel2Mat.r *= 0.8f;
    wheel2Mat.u *= 0.8f;

    wheel3Mat.r *= 1.6f;
    wheel3Mat.u *= 1.6f;
    wheel3Mat.f *= 1.6f;

    MineBuilding::s_wheelShape->Render(_ctx.predictionTime, wheel1Mat);
    MineBuilding::s_wheelShape->Render(_ctx.predictionTime, wheel2Mat);
    MineBuilding::s_wheelShape->Render(_ctx.predictionTime, wheel3Mat);

    //
    // Render counter

    GlobalBuilding* gb = g_context->m_globalWorld->GetBuilding(
        refinery.m_id.GetUniqueId(), g_context->m_locationId);
    int numRefined = 0;
    if (gb)
        numRefined = gb->m_link;

    Matrix34 counterMat =
        refinery.m_shape->GetMarkerWorldMatrix(refinery.m_counter1, refineryMat);
    glColor4f(0.6f, 0.8f, 0.9f, 1.0f);
    g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u,
                          10.0f, "%d", numRefined);
    counterMat.pos += counterMat.f * 0.1f;
    counterMat.pos += (counterMat.f ^ counterMat.u) * 0.2f;
    counterMat.pos += counterMat.u * 0.2f;
    g_gameFont.SetRenderShadow(true);
    glColor4f(0.6f, 0.8f, 0.9f, 0.0f);
    g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u,
                          10.0f, "%d", numRefined);
    g_gameFont.SetRenderShadow(false);
}
