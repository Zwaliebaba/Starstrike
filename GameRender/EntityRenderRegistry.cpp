#include "pch.h"

#include "EntityRenderRegistry.h"
#include "EntityRenderer.h"

EntityRenderRegistry g_entityRenderRegistry;

void EntityRenderRegistry::Register(int _entityType, EntityRenderer* _renderer)
{
    DEBUG_ASSERT(_entityType >= 0 && _entityType < Entity::NumEntityTypes);
    m_renderers[_entityType] = _renderer;
}

EntityRenderer* EntityRenderRegistry::Get(int _entityType) const
{
    if (_entityType < 0 || _entityType >= Entity::NumEntityTypes)
        return nullptr;
    return m_renderers[_entityType];
}
