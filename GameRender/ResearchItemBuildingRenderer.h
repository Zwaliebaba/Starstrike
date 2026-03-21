#pragma once

#include "DefaultBuildingRenderer.h"

// Renders ResearchItem: rotating shape (mutates m_front/m_up like the
// original), billboard cloud glow + starbursts, and a control line beam
// to heaven.  Editor mode shows research type label.
class ResearchItemBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
