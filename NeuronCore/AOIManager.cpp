#include "pch.h"
#include "AOIManager.h"
#include "entity.h"

AOIManager::AOIManager()
{
}

void AOIManager::UpdateClientPosition(uint8_t clientId, const LegacyVector3& position)
{
    m_clientAOIs[clientId].position = position;
}

bool AOIManager::IsInInterest(uint8_t clientId, const LegacyVector3& entityPos) const
{
    auto it = m_clientAOIs.find(clientId);
    if (it == m_clientAOIs.end())
        return false;
    
    float distSq = DistanceSquared(it->second.position, entityPos);
    return distSq <= INTEREST_RADIUS_SQUARED;
}

bool AOIManager::IsInInterest(uint8_t clientId, NetEntityId entityId, const LegacyVector3& entityPos)
{
    auto it = m_clientAOIs.find(clientId);
    if (it == m_clientAOIs.end())
        return false;
    
    float distSq = DistanceSquared(it->second.position, entityPos);
    
    // Check if within primary interest radius
    if (distSq <= INTEREST_RADIUS_SQUARED)
    {
        MarkEntityVisible(clientId, entityId);
        return true;
    }
    
    // Check if was recently visible (hysteresis to prevent pop-in/out)
    if (it->second.recentlyVisible.count(entityId) > 0)
    {
        if (distSq <= HYSTERESIS_RADIUS_SQUARED)
        {
            return true;
        }
        else
        {
            // Entity has left the hysteresis zone, remove from tracking
            it->second.recentlyVisible.erase(entityId);
        }
    }
    
    return false;
}

uint8_t AOIManager::GetEntityPriority(uint8_t clientId, const LegacyVector3& entityPos) const
{
    auto it = m_clientAOIs.find(clientId);
    if (it == m_clientAOIs.end())
        return EntityPriority::Background;
    
    float distSq = DistanceSquared(it->second.position, entityPos);
    
    // Priority based on distance
    if (distSq <= 25.0f * 25.0f)        // Within 25 units
        return EntityPriority::Critical;
    else if (distSq <= 50.0f * 50.0f)   // Within 50 units
        return EntityPriority::High;
    else if (distSq <= 100.0f * 100.0f) // Within 100 units
        return EntityPriority::Medium;
    else if (distSq <= INTEREST_RADIUS_SQUARED)
        return EntityPriority::Low;
    else
        return EntityPriority::Background;
}

void AOIManager::FilterEntitiesForClient(uint8_t clientId,
                                          const std::vector<Entity*>& allEntities,
                                          std::vector<Entity*>& filteredEntities)
{
    filteredEntities.clear();
    filteredEntities.reserve(allEntities.size() / 4); // Estimate ~25% will be in range
    
    auto it = m_clientAOIs.find(clientId);
    if (it == m_clientAOIs.end())
        return;
    
    const LegacyVector3& clientPos = it->second.position;
    
    for (Entity* entity : allEntities)
    {
        if (!entity)
            continue;
        
        const LegacyVector3& entityPos = entity->m_pos;
        float distSq = DistanceSquared(clientPos, entityPos);
        
        if (distSq <= INTEREST_RADIUS_SQUARED)
        {
            filteredEntities.push_back(entity);
        }
    }
}

void AOIManager::MarkEntityVisible(uint8_t clientId, NetEntityId entityId)
{
    auto it = m_clientAOIs.find(clientId);
    if (it != m_clientAOIs.end())
    {
        it->second.recentlyVisible.insert(entityId);
    }
}

void AOIManager::RemoveClient(uint8_t clientId)
{
    m_clientAOIs.erase(clientId);
}

float AOIManager::DistanceSquared(const LegacyVector3& a, const LegacyVector3& b) const
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}
