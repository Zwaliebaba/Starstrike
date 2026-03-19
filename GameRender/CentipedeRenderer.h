#pragma once

#include "EntityRenderer.h"

// Render companion for Centipede entities.
// Renders linked body segment with terrain-aligned orientation and scale.
class CentipedeRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
