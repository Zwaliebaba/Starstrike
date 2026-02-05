#pragma once

#include <cstdint>

//=============================================================================
// Network Constants for Server-Authoritative Model
//=============================================================================

constexpr uint16_t NET_PROTOCOL_VERSION = 1;
constexpr size_t   NET_MAX_PAYLOAD_SIZE = 1400;  // Safe UDP payload under MTU
constexpr float    NET_TICK_RATE = 20.0f;        // Server ticks per second
constexpr float    NET_TICK_INTERVAL = 1.0f / NET_TICK_RATE;

//=============================================================================
// Packet Types - Split into Reliable and Unreliable channels
//=============================================================================

enum class NetPacketType : uint8_t
{
    // Unreliable (can be lost, latest-wins)
    ClientInput = 0x01,         // Client -> Server: Input state
    StateSnapshot = 0x02,       // Server -> Client: Entity states
    
    // Reliable (must arrive, ordered)
    ClientConnect = 0x10,       // Client -> Server: Connection request
    ClientDisconnect = 0x11,    // Client -> Server: Graceful disconnect
    ServerWelcome = 0x12,       // Server -> Client: Connection accepted
    ServerReject = 0x13,        // Server -> Client: Connection rejected
    TeamAssignment = 0x14,      // Server -> Client: Team assigned
    EntitySpawn = 0x15,         // Server -> Client: New entity created
    EntityDestroy = 0x16,       // Server -> Client: Entity removed
    Command = 0x17,             // Client -> Server: Game command (create unit, etc.)
    CommandAck = 0x18,          // Server -> Client: Command acknowledged
};

inline bool IsReliablePacket(NetPacketType type)
{
    return static_cast<uint8_t>(type) >= 0x10;
}

//=============================================================================
// Packet Header - Common to all packets (8 bytes)
//=============================================================================

#pragma pack(push, 1)
struct NetPacketHeader
{
    uint16_t protocolVersion;   // Protocol version for compatibility
    NetPacketType type;         // Packet type
    uint8_t flags;              // Packet flags (fragmented, compressed, etc.)
    uint32_t sequenceNum;       // Sequence number for ordering/ack
};
#pragma pack(pop)

static_assert(sizeof(NetPacketHeader) == 8, "NetPacketHeader must be 8 bytes");

//=============================================================================
// Packet Flags
//=============================================================================

namespace PacketFlags
{
    constexpr uint8_t None = 0x00;
    constexpr uint8_t Compressed = 0x01;
    constexpr uint8_t Fragmented = 0x02;
    constexpr uint8_t NeedsAck = 0x04;
}

//=============================================================================
// Entity State Flags - Which fields are present in delta update
//=============================================================================

namespace EntityStateFlags
{
    constexpr uint8_t Position = 0x01;
    constexpr uint8_t Velocity = 0x02;
    constexpr uint8_t Health = 0x04;
    constexpr uint8_t State = 0x08;
    constexpr uint8_t Target = 0x10;
    constexpr uint8_t Orders = 0x20;
}

//=============================================================================
// Quantized Position - Compressed 3D position (6 bytes vs 12)
//=============================================================================

#pragma pack(push, 1)
struct QuantizedPosition
{
    int16_t x, y, z;  // World units * 10 (0.1 unit precision)
    
    QuantizedPosition() : x(0), y(0), z(0) {}
    
    QuantizedPosition(float fx, float fy, float fz)
        : x(static_cast<int16_t>(fx * 10.0f))
        , y(static_cast<int16_t>(fy * 10.0f))
        , z(static_cast<int16_t>(fz * 10.0f))
    {}
    
    float GetX() const { return x / 10.0f; }
    float GetY() const { return y / 10.0f; }
    float GetZ() const { return z / 10.0f; }
};
#pragma pack(pop)

static_assert(sizeof(QuantizedPosition) == 6, "QuantizedPosition must be 6 bytes");

//=============================================================================
// Quantized Velocity - Compressed velocity (4 bytes vs 12)
//=============================================================================

#pragma pack(push, 1)
struct QuantizedVelocity
{
    int8_t x, y, z;   // Velocity * 10, clamped to [-12.7, 12.7]
    uint8_t padding;
    
    QuantizedVelocity() : x(0), y(0), z(0), padding(0) {}
    
    QuantizedVelocity(float fx, float fy, float fz)
        : x(static_cast<int8_t>(std::clamp(fx * 10.0f, -127.0f, 127.0f)))
        , y(static_cast<int8_t>(std::clamp(fy * 10.0f, -127.0f, 127.0f)))
        , z(static_cast<int8_t>(std::clamp(fz * 10.0f, -127.0f, 127.0f)))
        , padding(0)
    {}
};
#pragma pack(pop)

//=============================================================================
// Network Entity ID - Unique identifier for networked entities
//=============================================================================

using NetEntityId = uint16_t;
constexpr NetEntityId INVALID_NET_ENTITY_ID = 0xFFFF;

