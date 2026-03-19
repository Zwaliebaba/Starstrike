#pragma once

#include "EntityRenderer.h"

// Render companion for ArmyAnt entities.
// Reads shape/scale/position from the entity (const access)
// and issues the draw call via ShapeStatic::Render.
class ArmyAntRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
