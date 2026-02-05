# Network Protocol Specification

## Overview

This document specifies the network protocol used by Starstrike for multiplayer communication. The protocol is designed for real-time competitive gameplay with client-server architecture.

## Protocol Version

**Current Version**: 1.0 (Draft)

## Transport Layer

### Connection

- **Protocol**: UDP with custom reliability layer
- **Port Range**: 7777 (default), configurable
- **MTU**: 1200 bytes (safe for most networks)
- **Connection Timeout**: 10 seconds
- **Heartbeat Interval**: 1 second

### Packet Structure

All packets follow this basic structure:

```
┌──────────────────────────────────────────────────────────┐
│ Header (12 bytes)                                         │
├──────────────────────────────────────────────────────────┤
│ Payload (variable length, max 1188 bytes)                │
└──────────────────────────────────────────────────────────┘
```

### Packet Header

```cpp
struct PacketHeader {
    uint32_t protocolId;        // Magic number: 0x53545253 ("STRS")
    uint16_t packetType;        // Packet type enum
    uint16_t sequenceNumber;    // For ordering and ACK
    uint32_t ackBits;           // Bitfield of received packets
};
```

**Fields:**
- `protocolId`: Protocol identifier to reject invalid packets (0x53545253 = "STRS")
- `packetType`: Type of packet (see Packet Types below)
- `sequenceNumber`: Incrementing sequence for ordering and reliability
- `ackBits`: Bitfield indicating which recent packets were received (for reliability)

## Packet Types

### Connection Management

#### 0x0001 - Connection Request

Client → Server

```cpp
struct ConnectionRequest {
    uint8_t protocolVersion;    // Protocol version (currently 1)
    char playerName[32];        // Player name (UTF-8)
    uint64_t clientId;          // Unique client identifier
};
```

#### 0x0002 - Connection Accept

Server → Client

```cpp
struct ConnectionAccept {
    uint32_t assignedPlayerId;  // Server-assigned player ID
    uint32_t serverTick;        // Current server tick
    uint32_t tickRate;          // Server tick rate (Hz)
};
```

#### 0x0003 - Connection Reject

Server → Client

```cpp
struct ConnectionReject {
    uint8_t reason;             // Rejection reason code
    char message[128];          // Human-readable message
};

// Rejection reasons
enum class RejectReason : uint8_t {
    ServerFull = 1,
    VersionMismatch = 2,
    Banned = 3,
    InvalidCredentials = 4
};
```

#### 0x0004 - Disconnect

Either direction

```cpp
struct Disconnect {
    uint8_t reason;             // Disconnect reason
    char message[128];          // Optional message
};

enum class DisconnectReason : uint8_t {
    ClientQuit = 1,
    Timeout = 2,
    Kicked = 3,
    ServerShutdown = 4
};
```

#### 0x0005 - Heartbeat

Either direction (keep-alive)

```cpp
struct Heartbeat {
    uint32_t timestamp;         // Sender's timestamp (ms)
};
```

### Game State Synchronization

#### 0x0100 - Player Input

Client → Server (sent every frame)

```cpp
struct PlayerInput {
    uint32_t tick;              // Client tick number
    float moveX;                // Movement X axis (-1 to 1)
    float moveY;                // Movement Y axis (-1 to 1)
    float lookX;                // Look direction X
    float lookY;                // Look direction Y
    uint32_t buttonState;       // Bitfield of button states
};

// Button state bits
enum class InputButton : uint32_t {
    Fire = 0x01,
    AltFire = 0x02,
    Jump = 0x04,
    Boost = 0x08,
    Shield = 0x10
};
```

#### 0x0101 - World Snapshot

Server → Client (sent at tick rate, e.g., 60 Hz)

```cpp
struct WorldSnapshot {
    uint32_t tick;              // Server tick number
    uint8_t entityCount;        // Number of entities in this packet
    EntityState entities[];     // Array of entity states
};

struct EntityState {
    uint32_t entityId;          // Unique entity identifier
    uint8_t entityType;         // Entity type (player, projectile, etc.)
    Vector3 position;           // World position
    Quaternion rotation;        // World rotation
    Vector3 velocity;           // Linear velocity
    uint8_t health;             // Health (0-255)
    uint8_t flags;              // State flags
};

// Entity types
enum class EntityType : uint8_t {
    Player = 1,
    Projectile = 2,
    Asteroid = 3,
    Powerup = 4
};

// State flags
enum class EntityFlags : uint8_t {
    IsAlive = 0x01,
    IsGrounded = 0x02,
    IsBoosting = 0x04,
    HasShield = 0x08
};
```

#### 0x0102 - Entity Spawn

Server → Client

```cpp
struct EntitySpawn {
    uint32_t entityId;          // New entity ID
    uint8_t entityType;         // Type of entity
    Vector3 position;           // Initial position
    Quaternion rotation;        // Initial rotation
    uint8_t ownerId;            // Owner player ID (0 if none)
};
```

#### 0x0103 - Entity Destroy

Server → Client

```cpp
struct EntityDestroy {
    uint32_t entityId;          // Entity to destroy
    uint8_t reason;             // Destruction reason
};

enum class DestroyReason : uint8_t {
    Timeout = 1,
    Collision = 2,
    OutOfBounds = 3,
    Killed = 4
};
```

### Game Events

#### 0x0200 - Player Scored

Server → All Clients

```cpp
struct PlayerScored {
    uint32_t playerId;          // Player who scored
    uint32_t points;            // Points awarded
    uint8_t scoreType;          // Type of score
};

enum class ScoreType : uint8_t {
    Kill = 1,
    Assist = 2,
    Objective = 3
};
```

#### 0x0201 - Player Died

Server → All Clients

```cpp
struct PlayerDied {
    uint32_t victimId;          // Player who died
    uint32_t killerId;          // Player who killed (0 if environmental)
    uint8_t deathType;          // Type of death
};

enum class DeathType : uint8_t {
    Weapon = 1,
    Collision = 2,
    OutOfBounds = 3,
    Suicide = 4
};
```

#### 0x0202 - Match State

Server → All Clients

```cpp
struct MatchState {
    uint8_t state;              // Current match state
    uint32_t timeRemaining;     // Time remaining in state (ms)
    uint8_t teamScores[4];      // Scores for up to 4 teams
};

enum class GameState : uint8_t {
    Waiting = 1,
    Starting = 2,
    InProgress = 3,
    Ending = 4,
    Ended = 5
};
```

### Chat and Social

#### 0x0300 - Chat Message

Client → Server or Server → All Clients

```cpp
struct ChatMessage {
    uint32_t senderId;          // Sender player ID
    uint8_t channel;            // Chat channel
    char message[256];          // Message content (UTF-8)
};

enum class ChatChannel : uint8_t {
    All = 1,
    Team = 2,
    Whisper = 3,
    System = 4
};
```

## Reliability System

### Acknowledgment

Every packet includes an acknowledgment bitfield in the header:

- `sequenceNumber`: Current outgoing sequence number
- `ackBits`: 32-bit field where bit N indicates receipt of packet (sequenceNumber - N - 1)

### Reliable Packets

Packets marked as reliable are:
- Stored in a send buffer
- Retransmitted if not acknowledged within timeout (500ms)
- Maximum retransmit attempts: 5
- Connection dropped if too many retransmits fail

**Reliable Packet Types:**
- Connection management (0x0001-0x0005)
- Entity spawn/destroy (0x0102, 0x0103)
- Game events (0x0200-0x0202)
- Chat messages (0x0300)

**Unreliable Packet Types:**
- Player input (0x0100)
- World snapshots (0x0101) - superseded by newer snapshots

## Delta Compression

World snapshots use delta compression to reduce bandwidth:

1. **Baseline**: First snapshot is sent in full
2. **Delta**: Subsequent snapshots only include changed values
3. **Entity Culling**: Only entities near the player are included
4. **Quantization**: Positions and rotations are quantized to reduce precision

### Quantization

```cpp
// Position: 3 floats → 12 bytes
// Quantized: 3 int16 → 6 bytes (±32km range, ~1m precision)
int16_t quantizePosition(float value) {
    return (int16_t)(value * 32.0f);
}

// Rotation: Quaternion (4 floats) → 16 bytes  
// Quantized: 3 int16 → 6 bytes (smallest-three encoding)
// The largest component is dropped and reconstructed
```

## Client-Side Prediction

### Prediction Flow

1. **Client sends input** to server with client tick number
2. **Client predicts** movement locally using same logic as server
3. **Server sends authoritative state** with server tick
4. **Client reconciles** by replaying inputs from acknowledged tick

### Reconciliation

```cpp
void ReconcileState(const WorldSnapshot& serverState) {
    // Find the input that corresponds to server state
    uint32_t serverTick = serverState.tick;
    
    // Rewind to server state
    m_predictedState = serverState;
    
    // Replay all inputs after server tick
    for (auto& input : m_inputBuffer) {
        if (input.tick > serverTick) {
            ApplyInput(m_predictedState, input);
        }
    }
}
```

## Interpolation

Remote entities are interpolated between snapshots for smooth visuals.

### Interpolation Delay

Client maintains a render time `renderTime = currentTime - interpolationDelay`

- `interpolationDelay`: Typically 100ms (2-3 snapshots at 60Hz)
- Provides buffer for jitter and packet loss

### Interpolation Algorithm

```cpp
EntityState Interpolate(uint32_t entityId, float renderTime) {
    // Find two snapshots that bracket the render time
    Snapshot& from = FindSnapshotBefore(renderTime);
    Snapshot& to = FindSnapshotAfter(renderTime);
    
    float t = (renderTime - from.time) / (to.time - from.time);
    
    // Linear interpolation of position and velocity
    state.position = Lerp(from.position, to.position, t);
    state.velocity = Lerp(from.velocity, to.velocity, t);
    
    // Spherical interpolation of rotation
    state.rotation = Slerp(from.rotation, to.rotation, t);
    
    return state;
}
```

## Lag Compensation

Server performs lag compensation for hit detection:

1. **Client fires weapon** with client timestamp
2. **Server rewinds** entity states to client's view timestamp
3. **Server performs hit detection** in the rewound state
4. **Server applies damage** if hit confirmed

```cpp
bool CheckHit(uint32_t shooterId, Ray ray, float clientTime) {
    // Calculate player's latency
    float latency = GetClientLatency(shooterId);
    float rewindTime = serverTime - latency;
    
    // Rewind entity states
    auto rewindedState = RewindState(rewindTime);
    
    // Perform hit detection
    return RaycastAgainstState(ray, rewindedState);
}
```

## Bandwidth Optimization

### Techniques

1. **Delta Compression**: Only send changed values
2. **Quantization**: Reduce precision of floating-point values
3. **Entity Culling**: Only send nearby entities (area of interest)
4. **Priority System**: Send important updates more frequently
5. **Update Rate Scaling**: Reduce update rate for distant entities

### Bandwidth Budget

**Target Bandwidth** (per client):
- Upstream (client → server): 5-10 KB/s
- Downstream (server → client): 20-30 KB/s

**Breakdown:**
- Player input: ~50 bytes @ 60 Hz = 3 KB/s
- World snapshots: ~1000 bytes @ 20 Hz = 20 KB/s (with compression)
- Events and chat: Variable, typically <1 KB/s

## Security Considerations

### Server Validation

Server must validate all client input:

```cpp
bool ValidateInput(const PlayerInput& input) {
    // Check input ranges
    if (abs(input.moveX) > 1.0f || abs(input.moveY) > 1.0f)
        return false;
    
    // Check for impossible values
    if (input.tick > serverTick + MAX_PREDICTION_TICKS)
        return false;
    
    // Rate limiting
    if (GetInputRate(clientId) > MAX_INPUT_RATE)
        return false;
    
    return true;
}
```

### Anti-Cheat

1. **Server Authority**: All game state changes are authoritative on server
2. **Input Validation**: Bounds checking on all inputs
3. **Sanity Checks**: Detect impossible movements (teleporting, speed hacks)
4. **Rate Limiting**: Prevent packet flooding
5. **Encrypted Connections**: (Future) Prevent packet inspection and manipulation

### DOS Prevention

1. **Connection Rate Limiting**: Max connections per IP per time period
2. **Packet Rate Limiting**: Max packets per second per connection
3. **Bandwidth Limiting**: Disconnect clients exceeding bandwidth budget
4. **Blacklist**: Block known malicious IPs

## Testing and Simulation

### Network Conditions

Test with simulated network conditions:

```cpp
struct NetworkSimulation {
    float latency;              // Artificial latency (ms)
    float jitter;               // Latency variation (ms)
    float packetLoss;           // Packet loss percentage (0-1)
    float duplication;          // Packet duplication rate (0-1)
};
```

### Common Test Scenarios

1. **Low Latency**: 10-30ms (LAN)
2. **Medium Latency**: 50-100ms (regional)
3. **High Latency**: 150-250ms (intercontinental)
4. **Packet Loss**: 1-5% loss
5. **Jitter**: ±20ms variation

## Protocol Evolution

### Version Negotiation

During connection handshake:
1. Client sends protocol version
2. Server checks compatibility
3. Server accepts (same version) or rejects (incompatible)

### Future Enhancements

- Encryption (TLS/DTLS)
- Voice chat integration
- Spectator mode support
- Replay recording format
- Match statistics

## Reference Implementation

See implementation in:
- `src/network/protocol.h` - Protocol definitions
- `src/network/client.cpp` - Client networking
- `src/network/server.cpp` - Server networking
- `src/network/reliability.cpp` - Reliability layer

## References

- [Gaffer on Games - Networked Physics](https://gafferongames.com/post/networked_physics/)
- [Valve - Source Multiplayer Networking](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking)
- [Gabriel Gambetta - Fast-Paced Multiplayer](https://www.gabrielgambetta.com/client-server-game-architecture.html)

---

**Document Status**: Protocol specification (draft) - subject to change during implementation.
