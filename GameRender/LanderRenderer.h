#pragma once

#include "EntityRenderer.h"

// Render companion for Lander entities.
// Renders 3D shape with team colour and reloading dimming.
class LanderRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
