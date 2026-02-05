#include "pch.h"
#include "AuthoritativeServer.h"
#include "entity.h"
#include "NetLib.h"
#include "NetworkConst.h"
#include "Packet.h"

namespace
{
    AuthoritativeServer* s_instance = nullptr;
    
    // Helper function to find clientId by source address
    uint8_t FindClientByAddress(const net_ip_address_t* addr)
    {
        // For now, we just use the IP. In production, you'd also check port
        // and potentially have a session token
        char ip[16];
        IpToString(addr->sin_addr, ip);
        
        // This is a simplified lookup - in production you'd have a proper mapping
        // For localhost testing, all clients come from 127.0.0.1, so we rely on
        // the server's internal client tracking
        return 255; // Invalid client ID - caller should handle
    }
    
    void ServerListenCallback(const Packet& packet)
    {
        if (!s_instance)
        {
            DebugTrace("[Server] WARNING: s_instance is null!\n");
            return;
        }
        
        // Route packet based on type
        DataReader reader(packet.GetBuffer(), packet.GetSize());
        NetPacketHeader header;
        header.protocolVersion = reader.Read<uint16_t>();
        header.type = static_cast<NetPacketType>(reader.Read<uint8_t>());
        header.flags = reader.Read<uint8_t>();
        header.sequenceNum = reader.Read<uint32_t>();
        
        if (header.protocolVersion != NET_PROTOCOL_VERSION)
        {
            DebugTrace("[Server] Protocol version mismatch: got {}, expected {}\n", header.protocolVersion, NET_PROTOCOL_VERSION);
            return;
        }
        
        const auto fromAddr = packet.GetClientAddress();
        char ip[16];
        IpToString(fromAddr->sin_addr, ip);
        
        // Handle packet based on type
        switch (header.type)
        {
        case NetPacketType::ClientConnect:
            {
                DebugTrace("[Server] Handling ClientConnect from {}\n", ip);
                const uint8_t* payloadStart = packet.GetBuffer() + sizeof(NetPacketHeader);
                size_t payloadSize = packet.GetSize() - sizeof(NetPacketHeader);
                s_instance->OnClientConnect(ip, payloadStart, payloadSize);
            }
            break;
            
        case NetPacketType::ClientDisconnect:
            {
                DebugTrace("[Server] Handling ClientDisconnect\n");
                // Read clientId from the disconnect packet payload
                uint8_t clientId = reader.Read<uint8_t>();
                s_instance->OnClientDisconnect(clientId);
            }
            break;
            
        case NetPacketType::ClientInput:
            {
                // Read the input packet
                ClientInputPacket input;
                input.Read(reader);
                // The clientId should be in the input packet or we need another way to identify
                // For now, use the clientId from the input packet
                s_instance->OnClientInput(input.clientId, input);
            }
            break;
            
        case NetPacketType::Command:
            {
                DebugTrace("[Server] Handling Command\n");
                CommandPacket command;
                command.Read(reader);
                // Read clientId from command or use lookup
                // For now, we'd need to add clientId to CommandPacket
                // s_instance->OnClientCommand(clientId, command);
            }
            break;
            
        default:
            DebugTrace("[Server] Unknown packet type: {}\n", static_cast<int>(header.type));
            break;
        }
    }
}

AuthoritativeServer::AuthoritativeServer()
    : m_netLib(nullptr)
    , m_socket(nullptr)
    , m_currentTick(0)
    , m_nextEntityId(1)
{
    s_instance = this;
}

AuthoritativeServer::~AuthoritativeServer()
{
    Shutdown();
    s_instance = nullptr;
}

void AuthoritativeServer::Initialize()
{
    DebugTrace("[Server] Initializing...\n");
    
    m_netLib = new NetLib();
    m_netLib->Initialize();
    
    m_socket = new NetSocket();
    
    // Start listening on server port
    DebugTrace("[Server] Starting listener thread on port {}\n", SERVER_PORT);
    Windows::System::Threading::ThreadPool::RunAsync([this](auto&&)
    {
        m_socket->StartListening(ServerListenCallback, SERVER_PORT);
    });
    
    DebugTrace("[Server] Initialization complete\n");
}

void AuthoritativeServer::Shutdown()
{
    if (m_socket)
    {
        m_socket->StopListening();
        delete m_socket;
        m_socket = nullptr;
    }
    
    if (m_netLib)
    {
        delete m_netLib;
        m_netLib = nullptr;
    }
    
    m_clients.clear();
    m_networkedEntities.clear();
    m_entityToNetId.clear();
}

void AuthoritativeServer::Tick()
{
    m_currentTick++;
    
    // Game simulation happens here (called by game loop)
    // The server runs the authoritative simulation
}

void AuthoritativeServer::SendStateUpdates()
{
    // Send pending reliable messages first (spawns, destroys)
    if (!m_pendingSpawns.empty())
    {
        DebugTrace("[Server] Sending {} EntitySpawn packets\n", m_pendingSpawns.size());
    }
    
    for (const auto& spawn : m_pendingSpawns)
    {
        DataWriter writer;
        spawn.Write(writer);
        
        for (auto& [clientId, clientState] : m_clients)
        {
            DebugTrace("[Server] Sending EntitySpawn id={} to client {}\n", spawn.entityId, clientId);
            SendPacket(clientId, NetPacketType::EntitySpawn, writer);
        }
    }
    m_pendingSpawns.clear();
    
    for (const auto& destroy : m_pendingDestroys)
    {
        DataWriter writer;
        destroy.Write(writer);
        
        for (auto& [clientId, clientState] : m_clients)
        {
            SendPacket(clientId, NetPacketType::EntityDestroy, writer);
        }
    }
    m_pendingDestroys.clear();
    
    // Build and send state snapshots for each client
    for (auto& [clientId, clientState] : m_clients)
    {
        // Update AOI position based on client's view
        m_aoiManager.UpdateClientPosition(clientId, clientState.viewPosition);
        
        // Build snapshot packets (may be multiple to stay under MTU)
        auto snapshots = BuildSnapshotsForClient(clientId);
        
        for (const auto& snapshot : snapshots)
        {
            DataWriter writer;
            snapshot.Write(writer);
            SendPacket(clientId, NetPacketType::StateSnapshot, writer);
        }
    }
}

void AuthoritativeServer::OnClientConnect(const char* ip, const uint8_t* data, size_t size)
{
    DebugTrace("[Server] OnClientConnect from IP: {}\n", ip);
    
    // Find or create client
    uint8_t clientId = 0;
    for (uint8_t i = 0; i < 255; ++i)
    {
        if (m_clients.find(i) == m_clients.end())
        {
            clientId = i;
            break;
        }
    }
    
    DebugTrace("[Server] Assigned clientId: {}\n", clientId);
    
    ClientState& state = m_clients[clientId];
    state.peer = new ClientPeer(ip);
    state.teamId = 255; // Unassigned
    state.lastInputSequence = 0;
    state.lastAckedTick = 0;
    
    // Log the address we'll send the welcome to
    char peerIp[16];
    IpToString(state.peer->GetHost().sin_addr, peerIp);
    DebugTrace("[Server] ClientPeer address: {}:{}\n", peerIp, ntohs(state.peer->GetHost().sin_port));
    
    // Send welcome packet
    DebugTrace("[Server] Sending ServerWelcome to client {}\n", clientId);
    DataWriter welcome;
    welcome.WriteByte(clientId);
    welcome.WriteUInt32(m_currentTick);
    SendPacket(clientId, NetPacketType::ServerWelcome, welcome);
    
    DebugTrace("[Server] ServerWelcome sent\n");
}

void AuthoritativeServer::OnClientDisconnect(uint8_t clientId)
{
    auto it = m_clients.find(clientId);
    if (it != m_clients.end())
    {
        delete it->second.peer;
        m_clients.erase(it);
        m_aoiManager.RemoveClient(clientId);
    }
}

void AuthoritativeServer::OnClientInput(uint8_t clientId, const ClientInputPacket& input)
{
    auto it = m_clients.find(clientId);
    if (it == m_clients.end())
        return;
    
    ClientState& state = it->second;
    
    // Only process if newer than last input
    if (input.inputSequence <= state.lastInputSequence)
        return;
    
    state.lastInputSequence = input.inputSequence;
    state.lastAckedTick = input.lastServerTick;
    
    // Update view position for AOI
    state.viewPosition.x = input.aimPosition.GetX();
    state.viewPosition.y = input.aimPosition.GetY();
    state.viewPosition.z = input.aimPosition.GetZ();
    
    // Process input flags (movement, selection, etc.)
    // This is where you'd apply the input to the game state
}

void AuthoritativeServer::OnClientCommand(uint8_t clientId, const CommandPacket& command)
{
    ProcessCommand(clientId, command);
}

void AuthoritativeServer::ProcessCommand(uint8_t clientId, const CommandPacket& cmd)
{
    auto it = m_clients.find(clientId);
    if (it == m_clients.end())
        return;
    
    ClientState& client = it->second;
    bool success = true;
    
    switch (cmd.commandType)
    {
    case CommandPacket::CommandType::CreateUnit:
        // Validate and execute unit creation
        // The actual spawning happens through Location::SpawnEntities
        // which will call RegisterEntity through NetworkEntityManager
        success = ProcessCreateUnitCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::SelectUnit:
        // Server tracks what the client has selected (for validation)
        // Selection is mostly client-side UI
        success = true;
        break;
        
    case CommandPacket::CommandType::MoveUnit:
        // Issue move order to unit
        success = ProcessMoveCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::AttackTarget:
        // Issue attack order
        success = ProcessAttackCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::AimBuilding:
        // Aim a radar dish or turret
        success = ProcessAimBuildingCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::ToggleFence:
        // Toggle a fence switch
        success = ProcessToggleFenceCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::RunProgram:
        // Start a program (task)
        success = ProcessRunProgramCommand(client, cmd);
        break;
        
    case CommandPacket::CommandType::TargetProgram:
        // Target a program at a location
        success = ProcessTargetProgramCommand(client, cmd);
        break;
        
    default:
        success = false;
        break;
    }
    
    AcknowledgeCommand(clientId, cmd.commandId, success);
}

void AuthoritativeServer::AcknowledgeCommand(uint8_t clientId, uint32_t commandId, bool success)
{
    DataWriter ack;
    ack.WriteUInt32(commandId);
    ack.WriteByte(success ? 1 : 0);
    SendPacket(clientId, NetPacketType::CommandAck, ack);
}

NetEntityId AuthoritativeServer::RegisterEntity(Entity* entity, uint8_t teamId)
{
    NetEntityId netId = m_nextEntityId++;
    
    DebugTrace("[Server] RegisterEntity: netId={}, teamId={}, entityType={}, pos=({},{},{})\n",
        netId, teamId, static_cast<int>(entity->m_entityType),
        entity->m_pos.x, entity->m_pos.y, entity->m_pos.z);
    
    NetworkedEntity& netEntity = m_networkedEntities[netId];
    netEntity.entity = entity;
    netEntity.netId = netId;
    netEntity.teamId = teamId;
    netEntity.lastSentStateFlags = 0;
    netEntity.lastSentPosition = QuantizedPosition(entity->m_pos.x, entity->m_pos.y, entity->m_pos.z);
    netEntity.lastSentHealth = 100; // Default
    netEntity.lastSentState = 0;
    
    m_entityToNetId[entity] = netId;
    
    // Queue spawn notification
    EntitySpawnPacket spawn;
    spawn.entityId = netId;
    spawn.entityType = static_cast<uint8_t>(entity->m_entityType);
    spawn.teamId = teamId;
    spawn.position = netEntity.lastSentPosition;
    spawn.initialHealth = netEntity.lastSentHealth;
    m_pendingSpawns.push_back(spawn);
    
    return netId;
}

void AuthoritativeServer::UnregisterEntity(NetEntityId entityId)
{
    auto it = m_networkedEntities.find(entityId);
    if (it != m_networkedEntities.end())
    {
        m_entityToNetId.erase(it->second.entity);
        m_networkedEntities.erase(it);
        
        // Queue destroy notification
        EntityDestroyPacket destroy;
        destroy.entityId = entityId;
        destroy.reason = static_cast<uint8_t>(EntityDestroyPacket::Reason::Killed);
        m_pendingDestroys.push_back(destroy);
    }
}

NetEntityId AuthoritativeServer::GetNetEntityId(Entity* entity) const
{
    auto it = m_entityToNetId.find(entity);
    return (it != m_entityToNetId.end()) ? it->second : INVALID_NET_ENTITY_ID;
}

EntityStateDelta AuthoritativeServer::BuildEntityDelta(NetworkedEntity& netEntity)
{
    Entity* entity = netEntity.entity;
    EntityStateDelta delta;
    
    delta.entityId = netEntity.netId;
    delta.entityType = static_cast<uint8_t>(entity->m_entityType);
    delta.stateFlags = 0;
    
    // Check position change
    QuantizedPosition currentPos(entity->m_pos.x, entity->m_pos.y, entity->m_pos.z);
    if (currentPos.x != netEntity.lastSentPosition.x ||
        currentPos.y != netEntity.lastSentPosition.y ||
        currentPos.z != netEntity.lastSentPosition.z)
    {
        delta.stateFlags |= EntityStateFlags::Position;
        delta.position = currentPos;
        netEntity.lastSentPosition = currentPos;
    }
    
    // Check velocity (if entity has velocity)
    delta.stateFlags |= EntityStateFlags::Velocity;
    delta.velocity = QuantizedVelocity(entity->m_vel.x, entity->m_vel.y, entity->m_vel.z);
    
    // Add other state comparisons as needed (health, orders, etc.)
    
    return delta;
}

std::vector<StateSnapshotPacket> AuthoritativeServer::BuildSnapshotsForClient(uint8_t clientId)
{
    std::vector<StateSnapshotPacket> result;
    
    auto clientIt = m_clients.find(clientId);
    if (clientIt == m_clients.end())
        return result;
    
    StateSnapshotPacket currentPacket;
    currentPacket.serverTick = m_currentTick;
    currentPacket.ackInputSequence = clientIt->second.lastInputSequence;
    
    for (auto& [netId, netEntity] : m_networkedEntities)
    {
        Entity* entity = netEntity.entity;
        if (!entity)
            continue;
        
        // TODO: Re-enable AOI filtering once client positions are properly tracked
        // For now, send all entities to all clients
        // if (!m_aoiManager.IsInInterest(clientId, netId, entity->m_pos))
        //     continue;
        
        EntityStateDelta delta = BuildEntityDelta(netEntity);
        
        // Skip if nothing changed
        if (delta.stateFlags == 0)
            continue;
        
        // Check if adding this entity would exceed MTU
        if (!currentPacket.CanAddEntity(delta))
        {
            // Save current packet and start a new one
            if (!currentPacket.entities.empty())
            {
                result.push_back(std::move(currentPacket));
                currentPacket = StateSnapshotPacket();
                currentPacket.serverTick = m_currentTick;
                currentPacket.ackInputSequence = clientIt->second.lastInputSequence;
            }
        }
        
        currentPacket.AddEntity(delta);
    }
    
    // Don't forget the last packet
    if (!currentPacket.entities.empty())
    {
        result.push_back(std::move(currentPacket));
    }
    
    return result;
}

void AuthoritativeServer::SendPacket(uint8_t clientId, NetPacketType type, const DataWriter& payload)
{
    auto it = m_clients.find(clientId);
    if (it == m_clients.end())
    {
        DebugTrace("[Server] SendPacket: client {} not found!\n", clientId);
        return;
    }
    
    DebugTrace("[Server] SendPacket to client {}, type={}, payload size={}\n", clientId, static_cast<int>(type), payload.Size());
    
    // Build complete packet with header
    DataWriter fullPacket;
    fullPacket.WriteUInt16(NET_PROTOCOL_VERSION);
    fullPacket.WriteByte(static_cast<uint8_t>(type));
    fullPacket.WriteByte(PacketFlags::None);
    fullPacket.WriteUInt32(m_currentTick);
    
    // Append payload
    const uint8_t* payloadData = payload.Data();
    for (size_t i = 0; i < payload.Size(); ++i)
    {
        fullPacket.WriteByte(payloadData[i]);
    }
    
    DebugTrace("[Server] Full packet size: {}\n", fullPacket.Size());
    SendPacketRaw(it->second.peer->GetHost(), fullPacket);
}

void AuthoritativeServer::SendPacketRaw(const net_ip_address_t& dest, const DataWriter& data)
{
    if (m_socket)
    {
        char destIp[16];
        IpToString(dest.sin_addr, destIp);
        DebugTrace("[Server] SendPacketRaw to {}:{}\n", destIp, ntohs(dest.sin_port));
        m_socket->SendUDPPacket(dest, data);
    }
    else
    {
        DebugTrace("[Server] SendPacketRaw: socket is null!\n");
    }
}

//=============================================================================
// Command Handlers
// These process validated commands from clients and execute game actions
//=============================================================================

bool AuthoritativeServer::ProcessCreateUnitCommand(ClientState& client, const CommandPacket& cmd)
{
    // Validate the client owns the team
    if (cmd.teamId != client.teamId)
        return false;
    
    // Get target position
    LegacyVector3 targetPos(
        cmd.targetPosition.GetX(),
        cmd.targetPosition.GetY(),
        cmd.targetPosition.GetZ()
    );
    
    // TODO: Actually spawn the unit through Location::SpawnEntities
    // This would be done by calling into the game simulation
    // For now, we validate and acknowledge
    
    return true;
}

bool AuthoritativeServer::ProcessMoveCommand(ClientState& client, const CommandPacket& cmd)
{
    // Find the entity
    auto it = m_networkedEntities.find(cmd.targetEntityId);
    if (it == m_networkedEntities.end())
        return false;
    
    NetworkedEntity& netEntity = it->second;
    
    // Validate ownership
    if (netEntity.teamId != client.teamId)
        return false;
    
    // Get target position
    LegacyVector3 targetPos(
        cmd.targetPosition.GetX(),
        cmd.targetPosition.GetY(),
        cmd.targetPosition.GetZ()
    );
    
    // Issue move order to entity
    // Entity* entity = netEntity.entity;
    // entity->SetWaypoint(targetPos);
    
    return true;
}

bool AuthoritativeServer::ProcessAttackCommand(ClientState& client, const CommandPacket& cmd)
{
    // Find the attacking entity
    auto attackerIt = m_networkedEntities.find(cmd.targetEntityId);
    if (attackerIt == m_networkedEntities.end())
        return false;
    
    // Validate ownership
    if (attackerIt->second.teamId != client.teamId)
        return false;
    
    // Get target position
    LegacyVector3 targetPos(
        cmd.targetPosition.GetX(),
        cmd.targetPosition.GetY(),
        cmd.targetPosition.GetZ()
    );
    
    // Issue attack command
    // Entity* attacker = attackerIt->second.entity;
    // attacker->Attack(targetPos);
    
    return true;
}

bool AuthoritativeServer::ProcessAimBuildingCommand(ClientState& client, const CommandPacket& cmd)
{
    // Validate team ownership
    if (cmd.teamId != client.teamId)
        return false;
    
    // Get target position
    LegacyVector3 targetPos(
        cmd.targetPosition.GetX(),
        cmd.targetPosition.GetY(),
        cmd.targetPosition.GetZ()
    );
    
    // TODO: Apply to building through game simulation
    // Building* building = g_app->m_location->GetBuilding(cmd.buildingId);
    // if (building && building->m_id.GetTeamId() == cmd.teamId)
    //     building->Aim(targetPos);
    
    return true;
}

bool AuthoritativeServer::ProcessToggleFenceCommand(ClientState& client, const CommandPacket& cmd)
{
    // Fence switches may be toggled by any player
    // TODO: Validate and execute through game simulation
    // FenceSwitch* fs = g_app->m_location->GetBuilding(cmd.buildingId);
    // if (fs) fs->Switch();
    
    return true;
}

bool AuthoritativeServer::ProcessRunProgramCommand(ClientState& client, const CommandPacket& cmd)
{
    // Validate team ownership
    if (cmd.teamId != client.teamId)
        return false;
    
    // TODO: Execute program through task manager
    // g_app->m_taskManager->RunTask(cmd.teamId, cmd.programId);
    
    return true;
}

bool AuthoritativeServer::ProcessTargetProgramCommand(ClientState& client, const CommandPacket& cmd)
{
    // Validate team ownership
    if (cmd.teamId != client.teamId)
        return false;
    
    // Get target position
    LegacyVector3 targetPos(
        cmd.targetPosition.GetX(),
        cmd.targetPosition.GetY(),
        cmd.targetPosition.GetZ()
    );
    
    // TODO: Execute targeted program
    // g_app->m_taskManager->TargetTask(cmd.teamId, cmd.programId, targetPos);
    
    return true;
}
