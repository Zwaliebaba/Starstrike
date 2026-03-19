#pragma once

#include "EntityRenderer.h"

// Render companion for Tripod entities.
// Renders the body shape and delegates to EntityLeg::Render for each leg.
class TripodRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
