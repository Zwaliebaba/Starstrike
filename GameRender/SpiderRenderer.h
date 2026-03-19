#pragma once

#include "EntityRenderer.h"

// Render companion for Spider entities.
// Renders the body shape (with damage flicker) and delegates
// to EntityLeg::Render for each of the 8 legs.
class SpiderRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
