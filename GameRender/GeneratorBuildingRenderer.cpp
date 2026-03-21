#include "pch.h"

#include "GeneratorBuildingRenderer.h"
#include "generator.h"
#include "matrix34.h"
#include "ShapeStatic.h"
#include "text_renderer.h"

void GeneratorBuildingRenderer::Render(const Building& _building, const BuildingRenderContext& _ctx)
{
    PowerBuildingRenderer::Render(_building, _ctx);

    const auto& gen = static_cast<const Generator&>(_building);

    Matrix34 generatorMat(gen.m_front, g_upVector, gen.m_pos);
    Matrix34 counterMat = gen.m_shape->GetMarkerWorldMatrix(gen.m_counter, generatorMat);

    glColor4f(0.6f, 0.8f, 0.9f, 1.0f);
    g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u, 7.0f, "%d", static_cast<int>(gen.m_throughput * 10.0f));
    counterMat.pos += counterMat.f * 0.1f;
    counterMat.pos += (counterMat.f ^ counterMat.u) * 0.2f;
    counterMat.pos += counterMat.u * 0.2f;
    g_gameFont.SetRenderShadow(true);
    glColor4f(0.6f, 0.8f, 0.9f, 0.0f);
    g_gameFont.DrawText3D(counterMat.pos, counterMat.f, counterMat.u, 7.0f, "%d", static_cast<int>(gen.m_throughput * 10.0f));
    g_gameFont.SetRenderShadow(false);
}
