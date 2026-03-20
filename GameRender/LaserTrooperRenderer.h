#pragma once

#include "EntityRenderer.h"

// Render companion for LaserTrooper entities.
// Renders billboard sprite quads, ground shadows, side-on camera
// thickness, and death fragment animations — all hand-drawn GL geometry.
class LaserTrooperRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
