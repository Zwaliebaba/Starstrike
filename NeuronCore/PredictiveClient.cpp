#include "pch.h"
#include "PredictiveClient.h"
#include "NetLib.h"
#include "NetworkConst.h"
#include "Packet.h"
#include "hi_res_time.h"

namespace
{
    PredictiveClient* s_instance = nullptr;
    
    void ClientListenCallback(const Packet& packet)
    {
        DebugTrace("[Client] Received packet, size={}\n", packet.GetSize());
        
        if (s_instance)
        {
            DataReader reader(packet.GetBuffer(), packet.GetSize());
            
            // Read header
            uint16_t protocolVersion = reader.Read<uint16_t>();
            if (protocolVersion != NET_PROTOCOL_VERSION)
            {
                DebugTrace("[Client] Protocol version mismatch: got {}, expected {}\n", protocolVersion, NET_PROTOCOL_VERSION);
                return;
            }
            
            NetPacketType type = static_cast<NetPacketType>(reader.Read<uint8_t>());
            uint8_t flags = reader.Read<uint8_t>();
            uint32_t sequenceNum = reader.Read<uint32_t>();
            
            DebugTrace("[Client] Packet type={}, flags={}, seq={}\n", static_cast<int>(type), flags, sequenceNum);
            
            // Handle based on type
            switch (type)
            {
            case NetPacketType::ServerWelcome:
                DebugTrace("[Client] Handling ServerWelcome\n");
                s_instance->HandleServerWelcome(reader);
                break;
            case NetPacketType::StateSnapshot:
                s_instance->HandleStateSnapshot(reader);
                break;
            case NetPacketType::EntitySpawn:
                s_instance->HandleEntitySpawn(reader);
                break;
            case NetPacketType::EntityDestroy:
                s_instance->HandleEntityDestroy(reader);
                break;
            case NetPacketType::CommandAck:
                s_instance->HandleCommandAck(reader);
                break;
            default:
                DebugTrace("[Client] Unknown packet type: {}\n", static_cast<int>(type));
                break;
            }
        }
        else
        {
            DebugTrace("[Client] WARNING: s_instance is null!\n");
        }
    }
}

PredictiveClient::PredictiveClient()
    : m_netLib(nullptr)
    , m_socket(nullptr)
    , m_serverAddress{}
    , m_connected(false)
    , m_clientId(255)
    , m_teamId(255)
    , m_localTick(0)
    , m_lastServerTick(0)
    , m_inputSequence(0)
    , m_nextCommandId(1)
    , m_interpolationDelay(0.1f)  // 100ms interpolation delay
    , m_serverTimeOffset(0.0)
    , m_lastConnectAttempt(0.0)
    , m_connectRetries(0)
{
    s_instance = this;
}

PredictiveClient::~PredictiveClient()
{
    Shutdown();
    s_instance = nullptr;
}

void PredictiveClient::Initialize(const char* serverAddress)
{
    DebugTrace("[Client] Initializing with server address: {}\n", serverAddress);
    
    m_netLib = new NetLib();
    m_netLib->Initialize();
    
    m_socket = new NetSocket();
    
    // Resolve server address
    m_serverAddress = {};
    m_serverAddress.sin_family = AF_INET;
    m_serverAddress.sin_port = htons(SERVER_PORT);
    
    // Use inet_addr for IP address strings, fall back to gethostbyname for hostnames
    unsigned long addr = inet_addr(serverAddress);
    if (addr != INADDR_NONE)
    {
        m_serverAddress.sin_addr.s_addr = addr;
        DebugTrace("[Client] Server address resolved via inet_addr\n");
    }
    else
    {
        HOSTENT* hostent = gethostbyname(serverAddress);
        if (hostent && hostent->h_addr_list[0])
        {
            m_serverAddress.sin_addr.s_addr = *reinterpret_cast<unsigned long*>(hostent->h_addr_list[0]);
            DebugTrace("[Client] Server address resolved via gethostbyname\n");
        }
        else
        {
            DebugTrace("[Client] WARNING: Failed to resolve server address, using 127.0.0.1\n");
            m_serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
        }
    }
    
    char resolvedIp[16];
    IpToString(m_serverAddress.sin_addr, resolvedIp);
    DebugTrace("[Client] Will connect to {}:{}\n", resolvedIp, ntohs(m_serverAddress.sin_port));
    
    // Start listening for server responses
    DebugTrace("[Client] Starting listener thread on port {}\n", CLIENT_PORT);
    Windows::System::Threading::ThreadPool::RunAsync([this](auto&&)
    {
        m_socket->StartListening(ClientListenCallback, CLIENT_PORT);
    });
}

void PredictiveClient::Shutdown()
{
    if (m_connected)
    {
        Disconnect();
    }
    
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
    
    m_entities.clear();
    m_pendingCommands.clear();
    m_inputHistory.clear();
}

void PredictiveClient::Connect()
{
    DebugTrace("[Client] Sending ClientConnect packet (attempt {})\n", m_connectRetries + 1);
    
    m_lastConnectAttempt = GetHighResTime();
    m_connectRetries = 0;
    
    DataWriter packet;
    packet.WriteUInt16(NET_PROTOCOL_VERSION);
    packet.WriteByte(static_cast<uint8_t>(NetPacketType::ClientConnect));
    packet.WriteByte(PacketFlags::None);
    packet.WriteUInt32(0); // Sequence
    
    m_socket->SendUDPPacket(m_serverAddress, packet);
}

void PredictiveClient::Disconnect()
{
    if (!m_connected)
        return;
    
    DataWriter packet;
    packet.WriteUInt16(NET_PROTOCOL_VERSION);
    packet.WriteByte(static_cast<uint8_t>(NetPacketType::ClientDisconnect));
    packet.WriteByte(PacketFlags::None);
    packet.WriteUInt32(m_inputSequence);
    packet.WriteByte(m_clientId);
    
    m_socket->SendUDPPacket(m_serverAddress, packet);
    
    m_connected = false;
    m_entities.clear();
}

void PredictiveClient::SendInput(const ClientInputPacket& input)
{
    if (!m_connected)
        return;
    
    // Store in history for reconciliation
    m_inputHistory.push_back(input);
    if (m_inputHistory.size() > INPUT_HISTORY_SIZE)
    {
        m_inputHistory.pop_front();
    }
    
    // Build packet
    DataWriter packet;
    packet.WriteUInt16(NET_PROTOCOL_VERSION);
    packet.WriteByte(static_cast<uint8_t>(NetPacketType::ClientInput));
    packet.WriteByte(PacketFlags::None);
    packet.WriteUInt32(m_inputSequence++);
    
    // Write input data with our clientId
    ClientInputPacket mutableInput = input;
    mutableInput.clientId = m_clientId;
    mutableInput.inputSequence = m_inputSequence;
    mutableInput.lastServerTick = m_lastServerTick;
    mutableInput.Write(packet);
    
    m_socket->SendUDPPacket(m_serverAddress, packet);
}

void PredictiveClient::SendCommand(const CommandPacket& command)
{
    if (!m_connected)
        return;
    
    // Assign command ID
    uint32_t commandId = m_nextCommandId++;
    
    // Store for retry
    PendingCommand pending;
    pending.commandId = commandId;
    pending.command = command;
    pending.command.commandId = commandId;
    pending.sentTime = GetHighResTime();
    pending.retryCount = 0;
    m_pendingCommands[commandId] = pending;
    
    // Build packet
    DataWriter packet;
    packet.WriteUInt16(NET_PROTOCOL_VERSION);
    packet.WriteByte(static_cast<uint8_t>(NetPacketType::Command));
    packet.WriteByte(PacketFlags::NeedsAck);
    packet.WriteUInt32(commandId);
    
    pending.command.Write(packet);
    
    m_socket->SendUDPPacket(m_serverAddress, packet);
}

void PredictiveClient::UpdateInterpolation(float deltaTime)
{
    double currentTime = GetHighResTime();
    
    // Retry connection if not connected
    if (!m_connected && m_connectRetries < MAX_CONNECT_RETRIES)
    {
        if (currentTime - m_lastConnectAttempt > CONNECT_RETRY_INTERVAL)
        {
            m_connectRetries++;
            m_lastConnectAttempt = currentTime;
            
            DebugTrace("[Client] Connection retry {} of {}\n", m_connectRetries, MAX_CONNECT_RETRIES);
            
            DataWriter packet;
            packet.WriteUInt16(NET_PROTOCOL_VERSION);
            packet.WriteByte(static_cast<uint8_t>(NetPacketType::ClientConnect));
            packet.WriteByte(PacketFlags::None);
            packet.WriteUInt32(0); // Sequence
            
            m_socket->SendUDPPacket(m_serverAddress, packet);
        }
        return;  // Don't do interpolation until connected
    }
    
    if (!m_connected && m_connectRetries >= MAX_CONNECT_RETRIES)
    {
        static bool warnedOnce = false;
        if (!warnedOnce)
        {
            DebugTrace("[Client] WARNING: Max connection retries reached, giving up\n");
            warnedOnce = true;
        }
        return;
    }
    
    m_localTick++;
    
    // Calculate render time (current time minus interpolation delay)
    double renderTime = currentTime - m_interpolationDelay - m_serverTimeOffset;
    uint32_t renderTick = static_cast<uint32_t>(renderTime / NET_TICK_INTERVAL);
    
    // Interpolate all entities
    for (auto& [entityId, entity] : m_entities)
    {
        InterpolateEntity(entity, static_cast<float>(renderTime));
    }
    
    // Retry pending commands that haven't been acked
    for (auto& [cmdId, pending] : m_pendingCommands)
    {
        if (currentTime - pending.sentTime > 0.5 && pending.retryCount < 3) // Retry after 500ms
        {
            pending.retryCount++;
            pending.sentTime = currentTime;
            
            DataWriter packet;
            packet.WriteUInt16(NET_PROTOCOL_VERSION);
            packet.WriteByte(static_cast<uint8_t>(NetPacketType::Command));
            packet.WriteByte(PacketFlags::NeedsAck);
            packet.WriteUInt32(cmdId);
            pending.command.Write(packet);
            
            m_socket->SendUDPPacket(m_serverAddress, packet);
        }
    }
}

void PredictiveClient::InterpolateEntity(InterpolatedEntity& entity, float renderTime)
{
    if (entity.snapshots.size() < 2)
    {
        // Not enough snapshots, use latest
        if (!entity.snapshots.empty())
        {
            const EntitySnapshot& latest = entity.snapshots.back();
            entity.interpolatedPos.x = latest.position.GetX();
            entity.interpolatedPos.y = latest.position.GetY();
            entity.interpolatedPos.z = latest.position.GetZ();
        }
        return;
    }
    
    // Find two snapshots to interpolate between
    const EntitySnapshot* from = nullptr;
    const EntitySnapshot* to = nullptr;
    
    for (size_t i = 0; i < entity.snapshots.size() - 1; ++i)
    {
        float fromTime = entity.snapshots[i].tick * NET_TICK_INTERVAL;
        float toTime = entity.snapshots[i + 1].tick * NET_TICK_INTERVAL;
        
        if (renderTime >= fromTime && renderTime <= toTime)
        {
            from = &entity.snapshots[i];
            to = &entity.snapshots[i + 1];
            break;
        }
    }
    
    if (!from || !to)
    {
        // Render time outside snapshot range, use latest
        const EntitySnapshot& latest = entity.snapshots.back();
        entity.interpolatedPos.x = latest.position.GetX();
        entity.interpolatedPos.y = latest.position.GetY();
        entity.interpolatedPos.z = latest.position.GetZ();
        return;
    }
    
    // Calculate interpolation factor
    float fromTime = from->tick * NET_TICK_INTERVAL;
    float toTime = to->tick * NET_TICK_INTERVAL;
    float t = (renderTime - fromTime) / (toTime - fromTime);
    t = std::clamp(t, 0.0f, 1.0f);
    
    // Lerp position
    entity.interpolatedPos.x = from->position.GetX() + t * (to->position.GetX() - from->position.GetX());
    entity.interpolatedPos.y = from->position.GetY() + t * (to->position.GetY() - from->position.GetY());
    entity.interpolatedPos.z = from->position.GetZ() + t * (to->position.GetZ() - from->position.GetZ());
    
    // Lerp velocity
    float fromVelX = from->velocity.x / 10.0f;
    float fromVelY = from->velocity.y / 10.0f;
    float fromVelZ = from->velocity.z / 10.0f;
    float toVelX = to->velocity.x / 10.0f;
    float toVelY = to->velocity.y / 10.0f;
    float toVelZ = to->velocity.z / 10.0f;
    
    entity.interpolatedVel.x = fromVelX + t * (toVelX - fromVelX);
    entity.interpolatedVel.y = fromVelY + t * (toVelY - fromVelY);
    entity.interpolatedVel.z = fromVelZ + t * (toVelZ - fromVelZ);
}

bool PredictiveClient::GetEntityPosition(NetEntityId entityId, LegacyVector3& outPos) const
{
    auto it = m_entities.find(entityId);
    if (it == m_entities.end())
        return false;
    
    outPos = it->second.interpolatedPos;
    return true;
}

bool PredictiveClient::GetEntityVelocity(NetEntityId entityId, LegacyVector3& outVel) const
{
    auto it = m_entities.find(entityId);
    if (it == m_entities.end())
        return false;
    
    outVel = it->second.interpolatedVel;
    return true;
}

bool PredictiveClient::EntityExists(NetEntityId entityId) const
{
    return m_entities.find(entityId) != m_entities.end();
}

void PredictiveClient::HandleServerWelcome(DataReader& reader)
{
    m_clientId = reader.Read<uint8_t>();
    m_lastServerTick = reader.Read<uint32_t>();
    m_connected = true;
    
    // Calculate server time offset
    m_serverTimeOffset = GetHighResTime() - (m_lastServerTick * NET_TICK_INTERVAL);
    
    DebugTrace("[Client] *** CONNECTED *** clientId={}, serverTick={}\n", m_clientId, m_lastServerTick);
}

void PredictiveClient::HandleStateSnapshot(DataReader& reader)
{
    StateSnapshotPacket snapshot;
    snapshot.Read(reader);
    
    // Update server tick
    if (snapshot.serverTick > m_lastServerTick)
    {
        m_lastServerTick = snapshot.serverTick;
    }
    
    // Remove acknowledged inputs from history
    while (!m_inputHistory.empty() && m_inputHistory.front().inputSequence <= snapshot.ackInputSequence)
    {
        m_inputHistory.pop_front();
    }
    
    // Apply entity updates
    for (const auto& delta : snapshot.entities)
    {
        auto it = m_entities.find(delta.entityId);
        if (it == m_entities.end())
            continue; // Entity not known yet, wait for spawn
        
        InterpolatedEntity& entity = it->second;
        
        // Add snapshot to buffer
        EntitySnapshot snap;
        snap.tick = snapshot.serverTick;
        
        if (delta.stateFlags & EntityStateFlags::Position)
            snap.position = delta.position;
        else if (!entity.snapshots.empty())
            snap.position = entity.snapshots.back().position;
        
        if (delta.stateFlags & EntityStateFlags::Velocity)
            snap.velocity = delta.velocity;
        else if (!entity.snapshots.empty())
            snap.velocity = entity.snapshots.back().velocity;
        
        if (delta.stateFlags & EntityStateFlags::Health)
            snap.health = delta.health;
        else if (!entity.snapshots.empty())
            snap.health = entity.snapshots.back().health;
        
        entity.snapshots.push_back(snap);
        
        // Trim old snapshots
        while (entity.snapshots.size() > InterpolatedEntity::SNAPSHOT_BUFFER_SIZE)
        {
            entity.snapshots.pop_front();
        }
    }
}

void PredictiveClient::HandleEntitySpawn(DataReader& reader)
{
    EntitySpawnPacket spawn;
    spawn.Read(reader);
    
    DebugTrace("[Client] EntitySpawn: id={}, type={}, team={}, pos=({},{},{})\n",
        spawn.entityId, spawn.entityType, spawn.teamId,
        spawn.position.GetX(), spawn.position.GetY(), spawn.position.GetZ());
    
    InterpolatedEntity& entity = m_entities[spawn.entityId];
    entity.netId = spawn.entityId;
    entity.entityType = spawn.entityType;
    entity.teamId = spawn.teamId;
    entity.interpolatedPos.x = spawn.position.GetX();
    entity.interpolatedPos.y = spawn.position.GetY();
    entity.interpolatedPos.z = spawn.position.GetZ();
    entity.currentHealth = spawn.initialHealth;
    entity.currentState = 0;
    entity.snapshots.clear();
    
    // Add initial snapshot
    EntitySnapshot snap;
    snap.tick = m_lastServerTick;
    snap.position = spawn.position;
    snap.velocity = QuantizedVelocity();
    snap.health = spawn.initialHealth;
    snap.state = 0;
    entity.snapshots.push_back(snap);
}

void PredictiveClient::HandleEntityDestroy(DataReader& reader)
{
    EntityDestroyPacket destroy;
    destroy.Read(reader);
    
    m_entities.erase(destroy.entityId);
}

void PredictiveClient::HandleCommandAck(DataReader& reader)
{
    uint32_t commandId = reader.Read<uint32_t>();
    uint8_t success = reader.Read<uint8_t>();
    
    m_pendingCommands.erase(commandId);
    
    // If command failed, you might want to notify the UI or rollback prediction
}
