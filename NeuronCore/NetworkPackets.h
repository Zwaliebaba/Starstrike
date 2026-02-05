#pragma once

#include "NetworkTypes.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "LegacyVector3.h"

//=============================================================================
// ClientInputPacket - Sent from client to server every frame (small, unreliable)
// Size: ~20-30 bytes
//=============================================================================

class ClientInputPacket
{
public:
    uint8_t clientId;               // Client's assigned ID
    uint8_t teamId;
    uint32_t inputSequence;         // Client's input sequence number
    uint32_t lastServerTick;        // Last server tick the client processed
    QuantizedPosition aimPosition;  // Where the player is aiming/mouse position
    uint16_t inputFlags;            // Button states (select, attack, etc.)
    
    ClientInputPacket();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
    
    // Input flag helpers
    void SetFlag(uint16_t flag) { inputFlags |= flag; }
    void ClearFlag(uint16_t flag) { inputFlags &= ~flag; }
    bool HasFlag(uint16_t flag) const { return (inputFlags & flag) != 0; }
};

namespace InputFlags
{
    constexpr uint16_t PrimaryAction = 0x0001;    // Left click / select
    constexpr uint16_t SecondaryAction = 0x0002; // Right click / command
    constexpr uint16_t Modifier = 0x0004;         // Shift held
    constexpr uint16_t AltModifier = 0x0008;      // Alt held
    constexpr uint16_t CameraUp = 0x0010;
    constexpr uint16_t CameraDown = 0x0020;
    constexpr uint16_t CameraLeft = 0x0040;
    constexpr uint16_t CameraRight = 0x0080;
}

//=============================================================================
// EntityStateDelta - Compact entity state update
// Size: 4-20 bytes depending on what changed
//=============================================================================

class EntityStateDelta
{
public:
    NetEntityId entityId;
    uint8_t entityType;
    uint8_t stateFlags;             // Which fields are present (EntityStateFlags)
    
    // Optional fields (only present if flag is set)
    QuantizedPosition position;
    QuantizedVelocity velocity;
    uint16_t health;
    uint8_t state;
    NetEntityId targetId;
    uint8_t orders;
    
    EntityStateDelta();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
    
    size_t EstimateSize() const;
};

//=============================================================================
// StateSnapshotPacket - Server -> Client world state (unreliable)
// Contains multiple entity deltas, chunked to fit MTU
//=============================================================================

class StateSnapshotPacket
{
public:
    uint32_t serverTick;            // Server's simulation tick
    uint32_t ackInputSequence;      // Last input sequence server processed
    uint16_t entityCount;           // Number of entities in this packet
    
    std::vector<EntityStateDelta> entities;
    
    StateSnapshotPacket();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
    
    // Check if adding another entity would exceed MTU
    bool CanAddEntity(const EntityStateDelta& delta) const;
    void AddEntity(const EntityStateDelta& delta);
    
    size_t EstimateSize() const;
    
private:
    static constexpr size_t HEADER_SIZE = sizeof(uint32_t) * 2 + sizeof(uint16_t);
};

//=============================================================================
// CommandPacket - Client -> Server game command (reliable)
// Size: ~10-30 bytes
//=============================================================================

class CommandPacket
{
public:
    enum class CommandType : uint8_t
    {
        None,
        CreateUnit,
        SelectUnit,
        MoveUnit,
        AttackTarget,
        AimBuilding,
        ToggleFence,
        RunProgram,
        TargetProgram
    };
    
    uint32_t commandId;             // Unique command ID for acknowledgment
    uint8_t teamId;
    CommandType commandType;
    
    // Command-specific data (union-like, only relevant fields used)
    uint8_t entityType;
    uint16_t count;
    int32_t buildingId;
    NetEntityId targetEntityId;
    QuantizedPosition targetPosition;
    uint8_t programId;
    
    CommandPacket();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
};

//=============================================================================
// EntitySpawnPacket - Server -> Client new entity notification (reliable)
//=============================================================================

class EntitySpawnPacket
{
public:
    NetEntityId entityId;
    uint8_t entityType;
    uint8_t teamId;
    QuantizedPosition position;
    uint16_t initialHealth;
    
    EntitySpawnPacket();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
};

//=============================================================================
// EntityDestroyPacket - Server -> Client entity removal (reliable)
//=============================================================================

class EntityDestroyPacket
{
public:
    NetEntityId entityId;
    uint8_t reason;                 // Death, despawn, etc.
    
    enum class Reason : uint8_t
    {
        Unknown,
        Killed,
        Despawned,
        OutOfBounds
    };
    
    EntityDestroyPacket();
    
    void Write(DataWriter& writer) const;
    void Read(DataReader& reader);
};

