#include "pch.h"

#include "WallBuildingRenderer.h"
#include "wall.h"
#include "globals.h"
#include "matrix34.h"
#include "ShapeStatic.h"

void WallBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    const Wall& wall = static_cast<const Wall&>(_building);

    LegacyVector3 predictedPos = wall.m_pos - LegacyVector3(0, wall.m_fallSpeed, 0) * _ctx.predictionTime;
    Matrix34 mat(wall.m_front, g_upVector, predictedPos);
    wall.m_shape->Render(_ctx.predictionTime, mat);
}
