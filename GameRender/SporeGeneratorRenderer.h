#pragma once

#include "EntityRenderer.h"

// Render companion for SporeGenerator entities.
// Complex renderer: shape + tail geometry + shadow.
class SporeGeneratorRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
