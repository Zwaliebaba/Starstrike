#include "pch.h"

#include "SpiritProcessorBuildingRenderer.h"
#include "spiritreceiver.h"
#include "globals.h"

// ---------------------------------------------------------------------------
// SpiritProcessorBuildingRenderer::RenderAlphas
// ---------------------------------------------------------------------------

void SpiritProcessorBuildingRenderer::RenderAlphas(const Building& _building,
                                                   const BuildingRenderContext& _ctx)
{
    ReceiverBuildingRenderer::RenderAlphas(_building, _ctx);

    auto& processor = static_cast<const SpiritProcessor&>(_building);

    // Render all floating spirits
    BeginRenderUnprocessedSpirits();

    float predictionTime = _ctx.predictionTime - SERVER_ADVANCE_PERIOD;

    for (int i = 0; i < processor.m_floatingSpirits.Size(); ++i)
    {
        UnprocessedSpirit* spirit = processor.m_floatingSpirits[i];
        LegacyVector3 pos = spirit->m_pos;
        pos += spirit->m_vel * predictionTime;
        pos += spirit->m_hover * predictionTime;
        float life = spirit->GetLife();
        RenderUnprocessedSpirit(pos, life);
    }

    EndRenderUnprocessedSpirits();
}
