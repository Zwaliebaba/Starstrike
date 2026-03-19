#pragma once

#include "EntityRenderer.h"

// Render companion for TriffidEgg entities.
// Shape-based rendering with pulsating scale, object lighting, and shadow.
class TriffidEggRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
