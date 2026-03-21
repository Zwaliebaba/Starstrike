#pragma once

#include "DefaultBuildingRenderer.h"

class LegacyVector3;

// Renderer companion for Teleport.
// RenderAlphas() renders spirits for entities currently in transit,
// then delegates to DefaultBuildingRenderer::RenderAlphas for lights/ports.
// Teleport has no Render override — the base Building::Render pattern works.
class TeleportBuildingRenderer : public DefaultBuildingRenderer
{
  protected:
    void RenderSpirit(const LegacyVector3& _pos, int _teamId);

  public:
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;
};
