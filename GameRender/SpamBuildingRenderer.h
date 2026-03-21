#pragma once

#include "DefaultBuildingRenderer.h"

// Renders Spam building: rotating shape (mutates m_front/m_up as legacy
// behaviour), billboard cloud glow + starburst effects.
class SpamBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
