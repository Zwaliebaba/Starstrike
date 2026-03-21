#include "pch.h"

#include "MineShaftBuildingRenderer.h"
#include "mine.h"
#include "ShapeStatic.h"

// ---------------------------------------------------------------------------
// MineShaftBuildingRenderer::Render
// ---------------------------------------------------------------------------

void MineShaftBuildingRenderer::Render(const Building& _building,
                                        const BuildingRenderContext& _ctx)
{
    MineBuildingRenderer::Render(_building, _ctx);

    auto& mine = static_cast<const Mine&>(_building);

    //
    // Render wheels

    Matrix34 mineMat(mine.m_front, g_upVector, mine.m_pos);
    Matrix34 wheel1Mat = mine.m_shape->GetMarkerWorldMatrix(mine.m_wheel1, mineMat);
    Matrix34 wheel2Mat = mine.m_shape->GetMarkerWorldMatrix(mine.m_wheel2, mineMat);

    float refinerySpeed = MineBuilding::RefinerySpeed();
    float predictedWheelRotate =
        mine.m_wheelRotate - 3.0f * refinerySpeed * _ctx.predictionTime;

    wheel1Mat.RotateAroundF(predictedWheelRotate * -1.0f);
    wheel2Mat.RotateAroundF(predictedWheelRotate);

    wheel1Mat.r *= 0.5f;
    wheel1Mat.u *= 0.5f;

    wheel2Mat.r *= 0.9f;
    wheel2Mat.u *= 0.9f;

    MineBuilding::s_wheelShape->Render(_ctx.predictionTime, wheel1Mat);
    MineBuilding::s_wheelShape->Render(_ctx.predictionTime, wheel2Mat);
}
