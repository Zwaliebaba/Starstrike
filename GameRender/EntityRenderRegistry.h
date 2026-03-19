#pragma once

#include "entity.h"

class EntityRenderer;

// Type-indexed table of entity render companions.
// Replaces polymorphic Entity::Render() dispatch.
// Call Get() with the entity type to retrieve the companion,
// or nullptr if no companion is registered (fallback to legacy path).
class EntityRenderRegistry
{
  public:
    void Register(int _entityType, EntityRenderer* _renderer);
    EntityRenderer* Get(int _entityType) const;

  private:
    EntityRenderer* m_renderers[Entity::NumEntityTypes] = {};
};

extern EntityRenderRegistry g_entityRenderRegistry;
