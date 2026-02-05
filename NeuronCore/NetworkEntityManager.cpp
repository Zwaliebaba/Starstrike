#include "pch.h"
#include "NetworkEntityManager.h"
#include "AuthoritativeServer.h"
#include "PredictiveClient.h"
#include "entity.h"

NetworkEntityManager* g_networkEntityManager = nullptr;

NetworkEntityManager::NetworkEntityManager()
    : m_server(nullptr)
    , m_client(nullptr)
    , m_localTeamId(255)
{
}

NetworkEntityManager::~NetworkEntityManager()
{
    Clear();
}

void NetworkEntityManager::SetServer(AuthoritativeServer* server)
{
    DebugTrace("[NetworkEntityManager] SetServer called, server={}\n", server != nullptr ? "valid" : "null");
    m_server = server;
}

NetEntityId NetworkEntityManager::RegisterEntity(Entity* entity, uint8_t teamId)
{
    if (!entity)
        return INVALID_NET_ENTITY_ID;
    
    // Check if already registered
    auto it = m_entityToNetId.find(entity);
    if (it != m_entityToNetId.end())
        return it->second;
    
    NetEntityId netId = INVALID_NET_ENTITY_ID;
    
    // On server, register with the authoritative server
    if (m_server)
    {
        DebugTrace("[NetworkEntityManager] Registering entity with server, teamId={}\n", teamId);
        netId = m_server->RegisterEntity(entity, teamId);
    }
    else
    {
        // On client, we receive IDs from server via EntitySpawnPacket
        // This path is for locally predicted entities only
        // For now, generate a temporary client-side ID (will be replaced when server confirms)
        DebugTrace("[NetworkEntityManager] No server - client mode, teamId={}\n", teamId);
        static NetEntityId s_clientTempId = 0x8000; // High bit set to distinguish from server IDs
        netId = s_clientTempId++;
    }
    
    if (netId != INVALID_NET_ENTITY_ID)
    {
        m_entityToNetId[entity] = netId;
        m_netIdToEntity[netId] = entity;
    }
    
    return netId;
}

void NetworkEntityManager::UnregisterEntity(Entity* entity)
{
    if (!entity)
        return;
    
    auto it = m_entityToNetId.find(entity);
    if (it != m_entityToNetId.end())
    {
        NetEntityId netId = it->second;
        
        // Notify server
        if (m_server)
        {
            m_server->UnregisterEntity(netId);
        }
        
        m_netIdToEntity.erase(netId);
        m_entityToNetId.erase(it);
        m_locallyControlled.erase(netId);
    }
}

void NetworkEntityManager::UnregisterEntity(NetEntityId netId)
{
    if (netId == INVALID_NET_ENTITY_ID)
        return;
    
    auto it = m_netIdToEntity.find(netId);
    if (it != m_netIdToEntity.end())
    {
        Entity* entity = it->second;
        
        if (m_server)
        {
            m_server->UnregisterEntity(netId);
        }
        
        m_entityToNetId.erase(entity);
        m_netIdToEntity.erase(it);
        m_locallyControlled.erase(netId);
    }
}

NetEntityId NetworkEntityManager::GetNetworkId(Entity* entity) const
{
    if (!entity)
        return INVALID_NET_ENTITY_ID;
    
    auto it = m_entityToNetId.find(entity);
    return (it != m_entityToNetId.end()) ? it->second : INVALID_NET_ENTITY_ID;
}

Entity* NetworkEntityManager::GetEntity(NetEntityId netId) const
{
    if (netId == INVALID_NET_ENTITY_ID)
        return nullptr;
    
    auto it = m_netIdToEntity.find(netId);
    return (it != m_netIdToEntity.end()) ? it->second : nullptr;
}

bool NetworkEntityManager::IsRegistered(Entity* entity) const
{
    return entity && m_entityToNetId.find(entity) != m_entityToNetId.end();
}

bool NetworkEntityManager::IsRegistered(NetEntityId netId) const
{
    return netId != INVALID_NET_ENTITY_ID && m_netIdToEntity.find(netId) != m_netIdToEntity.end();
}

void NetworkEntityManager::ApplyNetworkState()
{
    if (!m_client)
        return;
    
    // Iterate through all registered entities and apply interpolated state
    for (auto& [netId, entity] : m_netIdToEntity)
    {
        if (!entity)
            continue;
        
        // Skip locally controlled entities
        if (m_locallyControlled.count(netId) > 0)
            continue;
        
        // Get interpolated position from client
        LegacyVector3 pos;
        if (m_client->GetEntityPosition(netId, pos))
        {
            entity->m_pos = pos;
        }
        
        // Get interpolated velocity
        LegacyVector3 vel;
        if (m_client->GetEntityVelocity(netId, vel))
        {
            entity->m_vel = vel;
        }
    }
}

bool NetworkEntityManager::IsRemoteEntity(Entity* entity) const
{
    NetEntityId netId = GetNetworkId(entity);
    return IsRemoteEntity(netId);
}

bool NetworkEntityManager::IsRemoteEntity(NetEntityId netId) const
{
    if (netId == INVALID_NET_ENTITY_ID)
        return false;
    
    // Locally controlled entities are not remote
    if (m_locallyControlled.count(netId) > 0)
        return false;
    
    // If we're the server, nothing is remote
    if (m_server)
        return false;
    
    // On client, check if this entity belongs to our team
    Entity* entity = GetEntity(netId);
    if (entity && entity->m_id.GetTeamId() == m_localTeamId)
        return false;
    
    return true;
}

void NetworkEntityManager::SetLocallyControlled(Entity* entity, bool controlled)
{
    NetEntityId netId = GetNetworkId(entity);
    SetLocallyControlled(netId, controlled);
}

void NetworkEntityManager::SetLocallyControlled(NetEntityId netId, bool controlled)
{
    if (netId == INVALID_NET_ENTITY_ID)
        return;
    
    if (controlled)
        m_locallyControlled.insert(netId);
    else
        m_locallyControlled.erase(netId);
}

void NetworkEntityManager::Clear()
{
    m_entityToNetId.clear();
    m_netIdToEntity.clear();
    m_locallyControlled.clear();
}
