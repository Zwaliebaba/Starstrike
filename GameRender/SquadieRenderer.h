#pragma once

#include "EntityRenderer.h"

// Render companion for Squadie entities (insertion squad members).
// Reads shape/position/front from the entity (const access)
// and issues the draw call via ShapeStatic::Render.
class SquadieRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
