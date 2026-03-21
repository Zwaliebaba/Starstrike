#include "pch.h"

#include "FuelGeneratorBuildingRenderer.h"
#include "rocket.h"
#include "main.h"
#include "ShapeStatic.h"

void FuelGeneratorBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    FuelBuildingRenderer::Render(_building, _ctx);

    auto& gen = static_cast<const FuelGenerator&>(_building);

    // Advance pump animation — mirrors the original mutation in Render().
    const_cast<FuelGenerator&>(gen).m_pumpMovement += g_advanceTime * (gen.m_surges / 10.0f) * 2;

    LegacyVector3 pumpPos = const_cast<FuelGenerator&>(gen).GetPumpPos();
    Matrix34 mat(gen.m_front, g_upVector, pumpPos);

    gen.m_pump->Render(_ctx.predictionTime, mat);
}
