#pragma once

#include "EntityRenderer.h"

class DarwinianRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
