#pragma once

#include "NetworkTypes.h"
#include "LegacyVector3.h"
#include <unordered_map>

class Entity;
class AuthoritativeServer;
class PredictiveClient;

//=============================================================================
// NetworkEntityManager
// 
// Bridges the gap between game entities and the network system.
// - On server: Registers entities for network sync, tracks network IDs
// - On client: Applies interpolated state from server to local entities
//=============================================================================

class NetworkEntityManager
{
public:
    NetworkEntityManager();
    ~NetworkEntityManager();
    
    //-------------------------------------------------------------------------
    // Initialization
    //-------------------------------------------------------------------------
    
    void SetServer(AuthoritativeServer* server);
    void SetClient(PredictiveClient* client) { m_client = client; }
    
    bool IsServer() const { return m_server != nullptr; }
    bool IsClient() const { return m_client != nullptr; }
    
    //-------------------------------------------------------------------------
    // Entity Registration (Server-side)
    //-------------------------------------------------------------------------
    
    // Register an entity for network synchronization
    // Returns the assigned NetEntityId
    NetEntityId RegisterEntity(Entity* entity, uint8_t teamId);
    
    // Unregister an entity (when it dies or is removed)
    void UnregisterEntity(Entity* entity);
    void UnregisterEntity(NetEntityId netId);
    
    //-------------------------------------------------------------------------
    // Entity Lookup
    //-------------------------------------------------------------------------
    
    // Get network ID for a game entity
    NetEntityId GetNetworkId(Entity* entity) const;
    
    // Get game entity for a network ID
    Entity* GetEntity(NetEntityId netId) const;
    
    // Check if entity is registered
    bool IsRegistered(Entity* entity) const;
    bool IsRegistered(NetEntityId netId) const;
    
    //-------------------------------------------------------------------------
    // State Synchronization (Client-side)
    //-------------------------------------------------------------------------
    
    // Apply interpolated positions from network to entities
    // Call this each frame before rendering
    void ApplyNetworkState();
    
    // Check if an entity's position should come from network
    // (i.e., it's a remote entity, not locally controlled)
    bool IsRemoteEntity(Entity* entity) const;
    bool IsRemoteEntity(NetEntityId netId) const;
    
    //-------------------------------------------------------------------------
    // Local Player Control
    //-------------------------------------------------------------------------
    
    // Mark an entity as locally controlled (won't apply network interpolation)
    void SetLocallyControlled(Entity* entity, bool controlled);
    void SetLocallyControlled(NetEntityId netId, bool controlled);
    
    //-------------------------------------------------------------------------
    // Cleanup
    //-------------------------------------------------------------------------
    
    void Clear();
    
private:
    AuthoritativeServer* m_server;
    PredictiveClient* m_client;
    
    // Bidirectional mapping between game entities and network IDs
    std::unordered_map<Entity*, NetEntityId> m_entityToNetId;
    std::unordered_map<NetEntityId, Entity*> m_netIdToEntity;
    
    // Track which entities are locally controlled
    std::unordered_set<NetEntityId> m_locallyControlled;
    
    // Local team ID for determining remote vs local
    uint8_t m_localTeamId;
};

// Global network entity manager instance
extern NetworkEntityManager* g_networkEntityManager;

