#pragma once

#include "WorldObjectRenderer.h"

// Render companion for Snow effects.
// Billboard sprite rendering with a starburst texture.
class SnowRenderer : public WorldObjectRenderer
{
  public:
    void Render(const WorldObject& _object, const WorldObjectRenderContext& _ctx) override;
};
