#include "pch.h"

#include "WorldObjectRenderRegistry.h"
#include "WorldObjectRenderer.h"

WorldObjectRenderRegistry g_worldObjectRenderRegistry;

void WorldObjectRenderRegistry::Register(int _effectType, WorldObjectRenderer* _renderer)
{
    DEBUG_ASSERT(_effectType >= 0 && _effectType < WorldObject::NumEffectTypes);
    m_renderers[_effectType] = _renderer;
}

WorldObjectRenderer* WorldObjectRenderRegistry::Get(int _effectType) const
{
    if (_effectType < 0 || _effectType >= WorldObject::NumEffectTypes)
        return nullptr;
    return m_renderers[_effectType];
}
