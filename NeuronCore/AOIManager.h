#pragma once

#include "NetworkTypes.h"
#include "NetworkPackets.h"
#include "LegacyVector3.h"
#include <unordered_map>
#include <vector>

class Entity;

//=============================================================================
// Area of Interest (AOI) Manager
// Filters entity updates to only send relevant entities to each client
//=============================================================================

class AOIManager
{
public:
    // Default interest radius (units from player position)
    static constexpr float DEFAULT_INTEREST_RADIUS = 150.0f;
    static constexpr float INTEREST_RADIUS_SQUARED = DEFAULT_INTEREST_RADIUS * DEFAULT_INTEREST_RADIUS;
    
    // Extended radius for entities that were recently in view (hysteresis)
    static constexpr float HYSTERESIS_RADIUS = 175.0f;
    static constexpr float HYSTERESIS_RADIUS_SQUARED = HYSTERESIS_RADIUS * HYSTERESIS_RADIUS;
    
    AOIManager();
    
    // Update a client's known position
    void UpdateClientPosition(uint8_t clientId, const LegacyVector3& position);
    
    // Check if an entity is within a client's area of interest
    bool IsInInterest(uint8_t clientId, const LegacyVector3& entityPos) const;
    bool IsInInterest(uint8_t clientId, NetEntityId entityId, const LegacyVector3& entityPos);
    
    // Get priority level for an entity (closer = higher priority)
    uint8_t GetEntityPriority(uint8_t clientId, const LegacyVector3& entityPos) const;
    
    // Filter a list of entities to only those in a client's AOI
    void FilterEntitiesForClient(uint8_t clientId,
                                  const std::vector<Entity*>& allEntities,
                                  std::vector<Entity*>& filteredEntities);
    
    // Mark an entity as recently visible (for hysteresis)
    void MarkEntityVisible(uint8_t clientId, NetEntityId entityId);
    
    // Clear a client's tracking data
    void RemoveClient(uint8_t clientId);
    
private:
    struct ClientAOI
    {
        LegacyVector3 position;
        std::unordered_set<NetEntityId> recentlyVisible;
        uint32_t lastUpdateTick;
    };
    
    std::unordered_map<uint8_t, ClientAOI> m_clientAOIs;
    
    float DistanceSquared(const LegacyVector3& a, const LegacyVector3& b) const;
};

//=============================================================================
// Entity Priority Levels
//=============================================================================

namespace EntityPriority
{
    constexpr uint8_t Critical = 0;   // Own units, nearby enemies, damage events
    constexpr uint8_t High = 1;       // Units in combat, moving units
    constexpr uint8_t Medium = 2;     // Idle units, buildings
    constexpr uint8_t Low = 3;        // Distant entities, decorative objects
    constexpr uint8_t Background = 4; // Very distant, update rarely
}

