#pragma once

#include "EntityRenderer.h"

// Render companion for Armour entities.
// Renders tank body, flag banner, deploy flag, and passenger count text.
class ArmourRenderer : public EntityRenderer
{
  public:
    void Render(const Entity& _entity, const EntityRenderContext& _ctx) override;
};
