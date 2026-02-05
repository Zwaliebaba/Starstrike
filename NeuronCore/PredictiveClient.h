#pragma once

#include "NetworkTypes.h"
#include "NetworkPackets.h"
#include "NetSocket.h"
#include "LegacyVector3.h"
#include <unordered_map>
#include <deque>

class Entity;

//=============================================================================
// PredictiveClient - Client for server-authoritative model
//
// Key features:
// - Sends inputs to server, not commands
// - Predicts local player actions immediately
// - Interpolates remote entities between server snapshots
// - Reconciles local prediction with server state
//=============================================================================

class PredictiveClient
{
public:
    PredictiveClient();
    ~PredictiveClient();
    
    void Initialize(const char* serverAddress);
    void Shutdown();
    
    //-------------------------------------------------------------------------
    // Connection
    //-------------------------------------------------------------------------
    
    void Connect();
    void Disconnect();
    bool IsConnected() const { return m_connected; }
    
    //-------------------------------------------------------------------------
    // Main loop
    //-------------------------------------------------------------------------
    
    // Process incoming packets (call frequently)
    void ProcessIncomingPackets();
    
    // Send current input state to server
    void SendInput(const ClientInputPacket& input);
    
    // Send a reliable command
    void SendCommand(const CommandPacket& command);
    
    // Update interpolation (call every frame with delta time)
    void UpdateInterpolation(float deltaTime);
    
    //-------------------------------------------------------------------------
    // Entity state access (for rendering)
    //-------------------------------------------------------------------------
    
    // Get interpolated position for an entity
    bool GetEntityPosition(NetEntityId entityId, LegacyVector3& outPos) const;
    
    // Get interpolated velocity
    bool GetEntityVelocity(NetEntityId entityId, LegacyVector3& outVel) const;
    
    // Check if entity exists
    bool EntityExists(NetEntityId entityId) const;
    
    //-------------------------------------------------------------------------
    // Server info
    //-------------------------------------------------------------------------
    
    uint32_t GetServerTick() const { return m_lastServerTick; }
    uint32_t GetLocalTick() const { return m_localTick; }
    float GetInterpolationDelay() const { return m_interpolationDelay; }
    
    //-------------------------------------------------------------------------
    // Packet handling (public for callback access)
    //-------------------------------------------------------------------------
    
    void HandleServerWelcome(DataReader& reader);
    void HandleStateSnapshot(DataReader& reader);
    void HandleEntitySpawn(DataReader& reader);
    void HandleEntityDestroy(DataReader& reader);
    void HandleCommandAck(DataReader& reader);
    
private:
    //-------------------------------------------------------------------------
    // Internal structures
    //-------------------------------------------------------------------------
    
    struct EntitySnapshot
    {
        uint32_t tick;
        QuantizedPosition position;
        QuantizedVelocity velocity;
        uint16_t health;
        uint8_t state;
        uint8_t orders;
    };
    
    struct InterpolatedEntity
    {
        NetEntityId netId;
        uint8_t entityType;
        uint8_t teamId;
        
        // Snapshot buffer for interpolation (ring buffer of last N snapshots)
        static constexpr size_t SNAPSHOT_BUFFER_SIZE = 32;
        std::deque<EntitySnapshot> snapshots;
        
        // Current interpolated state
        LegacyVector3 interpolatedPos;
        LegacyVector3 interpolatedVel;
        uint16_t currentHealth;
        uint8_t currentState;
    };
    
    struct PendingCommand
    {
        uint32_t commandId;
        CommandPacket command;
        double sentTime;
        uint8_t retryCount;
    };
    
    //-------------------------------------------------------------------------
    // Interpolation
    //-------------------------------------------------------------------------
    
    void InterpolateEntity(InterpolatedEntity& entity, float renderTime);
    
    //-------------------------------------------------------------------------
    // Members
    //-------------------------------------------------------------------------
    
    
    NetLib* m_netLib;
    NetSocket* m_socket;
    net_ip_address_t m_serverAddress;
    
    bool m_connected;
    uint8_t m_clientId;
    uint8_t m_teamId;
    
    uint32_t m_localTick;
    uint32_t m_lastServerTick;
    uint32_t m_inputSequence;
    uint32_t m_nextCommandId;
    
    // Interpolation settings
    float m_interpolationDelay;     // How far behind real-time to render (typically 100ms)
    double m_serverTimeOffset;       // Difference between local and server time
    
    // Connection retry
    double m_lastConnectAttempt;     // Time of last connection attempt
    static constexpr double CONNECT_RETRY_INTERVAL = 0.5;  // Retry every 500ms
    static constexpr int MAX_CONNECT_RETRIES = 10;
    int m_connectRetries;
    
    // Entity states
    std::unordered_map<NetEntityId, InterpolatedEntity> m_entities;
    
    // Pending reliable commands awaiting acknowledgment
    std::unordered_map<uint32_t, PendingCommand> m_pendingCommands;
    
    // Input history for prediction reconciliation
    static constexpr size_t INPUT_HISTORY_SIZE = 64;
    std::deque<ClientInputPacket> m_inputHistory;
};

