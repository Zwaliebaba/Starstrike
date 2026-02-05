#pragma once

#include "NetworkTypes.h"
#include "NetworkPackets.h"
#include "AOIManager.h"
#include "NetSocket.h"
#include "ClientPeer.h"
#include <unordered_map>
#include <queue>

class Entity;

//=============================================================================
// AuthoritativeServer - Server-authoritative game server
// 
// Key differences from lockstep Server:
// - Server runs the simulation, clients only send inputs
// - Server sends state snapshots, not commands
// - No waiting for client acknowledgments to progress
// - Uses AOI to filter what each client receives
//=============================================================================

class AuthoritativeServer
{
public:
    AuthoritativeServer();
    ~AuthoritativeServer();
    
    void Initialize();
    void Shutdown();
    
    //-------------------------------------------------------------------------
    // Main loop
    //-------------------------------------------------------------------------
    
    // Process incoming packets (call from network thread)
    void ProcessIncomingPackets();
    
    // Run simulation tick (call at fixed rate, e.g., 20Hz)
    void Tick();
    
    // Send state updates to all clients (call after Tick)
    void SendStateUpdates();
    
    //-------------------------------------------------------------------------
    // Client management
    //-------------------------------------------------------------------------
    
    void OnClientConnect(const char* ip, const uint8_t* data, size_t size);
    void OnClientDisconnect(uint8_t clientId);
    void OnClientInput(uint8_t clientId, const ClientInputPacket& input);
    void OnClientCommand(uint8_t clientId, const CommandPacket& command);
    
    //-------------------------------------------------------------------------
    // Entity state tracking
    //-------------------------------------------------------------------------
    
    // Register an entity for network synchronization
    NetEntityId RegisterEntity(Entity* entity, uint8_t teamId);
    
    // Unregister an entity
    void UnregisterEntity(NetEntityId entityId);
    
    // Get the network ID for an entity
    NetEntityId GetNetEntityId(Entity* entity) const;
    
    //-------------------------------------------------------------------------
    // Reliable message queue
    //-------------------------------------------------------------------------
    
    void QueueReliableMessage(uint8_t clientId, NetPacketType type, const DataWriter& data);
    
private:
    //-------------------------------------------------------------------------
    // Internal structures
    //-------------------------------------------------------------------------
    
    struct ClientState
    {
        ClientPeer* peer;
        uint8_t teamId;
        LegacyVector3 viewPosition;
        uint32_t lastInputSequence;
        uint32_t lastAckedTick;
        std::queue<std::pair<uint32_t, DataWriter>> pendingReliable; // commandId -> data
    };
    
    struct NetworkedEntity
    {
        Entity* entity;
        NetEntityId netId;
        uint8_t teamId;
        uint8_t lastSentStateFlags;
        QuantizedPosition lastSentPosition;
        uint16_t lastSentHealth;
        uint8_t lastSentState;
    };
    
    //-------------------------------------------------------------------------
    // Packet building
    //-------------------------------------------------------------------------
    
    // Build delta state for an entity (only changed fields)
    EntityStateDelta BuildEntityDelta(NetworkedEntity& netEntity);
    
    // Build state snapshot packets for a client (may produce multiple packets for MTU)
    std::vector<StateSnapshotPacket> BuildSnapshotsForClient(uint8_t clientId);
    
    // Send a packet to a client
    void SendPacket(uint8_t clientId, NetPacketType type, const DataWriter& data);
    void SendPacketRaw(const net_ip_address_t& dest, const DataWriter& data);
    
    //-------------------------------------------------------------------------
    // Command processing
    //-------------------------------------------------------------------------
    
    void ProcessCommand(uint8_t clientId, const CommandPacket& cmd);
    void AcknowledgeCommand(uint8_t clientId, uint32_t commandId, bool success);
    
    // Individual command handlers (return true on success)
    bool ProcessCreateUnitCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessMoveCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessAttackCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessAimBuildingCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessToggleFenceCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessRunProgramCommand(ClientState& client, const CommandPacket& cmd);
    bool ProcessTargetProgramCommand(ClientState& client, const CommandPacket& cmd);
    
    //-------------------------------------------------------------------------
    // Members
    //-------------------------------------------------------------------------
    
    NetLib* m_netLib;
    NetSocket* m_socket;
    
    uint32_t m_currentTick;
    
    AOIManager m_aoiManager;
    
    std::unordered_map<uint8_t, ClientState> m_clients;
    std::unordered_map<NetEntityId, NetworkedEntity> m_networkedEntities;
    std::unordered_map<Entity*, NetEntityId> m_entityToNetId;
    
    NetEntityId m_nextEntityId;
    
    // Pending spawns/destroys to broadcast
    std::vector<EntitySpawnPacket> m_pendingSpawns;
    std::vector<EntityDestroyPacket> m_pendingDestroys;
};

