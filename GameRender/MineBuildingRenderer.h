#pragma once

#include "DefaultBuildingRenderer.h"

class MineBuilding;
class MineCart;

// Base renderer for all MineBuilding-family types (TrackLink, TrackJunction,
// TrackStart, TrackEnd, Refinery, Mine).  Renders mine carts in the opaque
// pass and track-rail quads in the alpha pass.
class MineBuildingRenderer : public DefaultBuildingRenderer
{
  public:
    void Render(const Building& _building, const BuildingRenderContext& _ctx) override;
    void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) override;

  protected:
    void RenderCart(const MineBuilding& _mine, MineCart* _cart, float _predictionTime);
};
