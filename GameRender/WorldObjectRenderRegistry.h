#pragma once

#include "worldobject.h"

class WorldObjectRenderer;

// Type-indexed table of WorldObject (effect) render companions.
// Replaces polymorphic WorldObject::Render() dispatch for effects.
// Call Get() with the effect type to retrieve the companion,
// or nullptr if no companion is registered (fallback to legacy path).
class WorldObjectRenderRegistry
{
  public:
    void Register(int _effectType, WorldObjectRenderer* _renderer);
    WorldObjectRenderer* Get(int _effectType) const;

  private:
    WorldObjectRenderer* m_renderers[WorldObject::NumEffectTypes] = {};
};

extern WorldObjectRenderRegistry g_worldObjectRenderRegistry;
