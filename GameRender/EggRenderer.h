#pragma once

#include "EntityRenderer.h"

// Render companion for Egg entities.
// Billboard sprite rendering with throb animation and death fragments.
class EggRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
