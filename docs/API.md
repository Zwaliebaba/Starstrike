# Starstrike API Reference

## Overview

This document provides a reference for the public API of the Starstrike game engine. The API is designed to be clean, modern C++, with clear ownership semantics and minimal coupling between subsystems.

## API Design Principles

1. **RAII**: All resources managed through RAII wrappers
2. **Explicit Ownership**: Clear ownership via unique_ptr/shared_ptr
3. **Minimal Coupling**: Subsystems communicate through well-defined interfaces
4. **Type Safety**: Leverage C++ type system for compile-time safety
5. **Performance**: Zero-cost abstractions where possible

## Namespace Structure

```cpp
namespace Starstrike {
    namespace Engine {
        // Core engine systems
    }
    namespace Renderer {
        // Rendering subsystem
    }
    namespace Network {
        // Networking subsystem
    }
    namespace Game {
        // Game-specific systems
    }
    namespace Math {
        // Math library
    }
}
```

## Core Engine API

### Engine Initialization

```cpp
namespace Starstrike::Engine {

// Engine configuration
struct EngineConfig {
    uint32_t windowWidth = 1920;
    uint32_t windowHeight = 1080;
    bool fullscreen = false;
    bool vsync = true;
    bool enableDebugLayer = false;
    std::string windowTitle = "Starstrike";
};

// Main engine class
class Engine {
public:
    // Initialize engine with configuration
    static std::unique_ptr<Engine> Create(const EngineConfig& config);
    
    // Run the main game loop
    void Run();
    
    // Shutdown engine and cleanup resources
    void Shutdown();
    
    // Get subsystem interfaces
    Renderer::IRenderer* GetRenderer();
    Network::INetworkManager* GetNetworkManager();
    Input::IInputSystem* GetInputSystem();
    
    ~Engine();
    
private:
    Engine() = default;
    // Implementation details hidden
};

} // namespace Starstrike::Engine
```

### Entity Component System

```cpp
namespace Starstrike::Engine {

// Entity handle (lightweight ID)
struct Entity {
    uint64_t id;
    
    bool operator==(const Entity& other) const { return id == other.id; }
    bool IsValid() const { return id != 0; }
};

// World manages all entities and components
class World {
public:
    // Create a new entity
    Entity CreateEntity();
    
    // Destroy entity and all its components
    void DestroyEntity(Entity entity);
    
    // Add component to entity
    template<typename ComponentType, typename... Args>
    ComponentType& AddComponent(Entity entity, Args&&... args);
    
    // Get component from entity (returns nullptr if not present)
    template<typename ComponentType>
    ComponentType* GetComponent(Entity entity);
    
    // Remove component from entity
    template<typename ComponentType>
    void RemoveComponent(Entity entity);
    
    // Check if entity has component
    template<typename ComponentType>
    bool HasComponent(Entity entity) const;
    
    // Iterate over all entities with specific components
    template<typename... ComponentTypes>
    void Each(std::function<void(Entity, ComponentTypes&...)> callback);
    
    // Update all systems
    void Update(float deltaTime);
};

} // namespace Starstrike::Engine
```

### Common Components

```cpp
namespace Starstrike::Engine {

// Transform component
struct TransformComponent {
    Math::Vector3 position = {0.0f, 0.0f, 0.0f};
    Math::Quaternion rotation = Math::Quaternion::Identity();
    Math::Vector3 scale = {1.0f, 1.0f, 1.0f};
    
    Math::Matrix4x4 GetWorldMatrix() const;
};

// Mesh renderer component
struct MeshRendererComponent {
    uint64_t meshId = 0;
    uint64_t materialId = 0;
    bool castShadows = true;
    bool receiveShadows = true;
};

// Rigidbody component (for physics)
struct RigidbodyComponent {
    Math::Vector3 velocity = {0.0f, 0.0f, 0.0f};
    Math::Vector3 angularVelocity = {0.0f, 0.0f, 0.0f};
    float mass = 1.0f;
    float drag = 0.1f;
    float angularDrag = 0.1f;
    bool useGravity = true;
    bool isKinematic = false;
};

// Network identity component
struct NetworkIdentityComponent {
    uint32_t networkId = 0;
    bool isServer = false;
    bool isLocalPlayer = false;
};

} // namespace Starstrike::Engine
```

## Renderer API

### Renderer Interface

```cpp
namespace Starstrike::Renderer {

// Rendering statistics
struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t trianglesRendered = 0;
    uint32_t textureBinds = 0;
    float frameTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
};

// Main renderer interface
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // Begin frame
    virtual void BeginFrame() = 0;
    
    // End frame and present
    virtual void EndFrame() = 0;
    
    // Submit mesh for rendering
    virtual void Submit(const MeshRenderData& data) = 0;
    
    // Get rendering statistics
    virtual RenderStats GetStats() const = 0;
    
    // Resize render targets
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};

// Mesh data
struct MeshRenderData {
    Math::Matrix4x4 worldMatrix;
    uint64_t meshId;
    uint64_t materialId;
    uint32_t instanceCount = 1;
};

} // namespace Starstrike::Renderer
```

### Resource Management

```cpp
namespace Starstrike::Renderer {

// Resource handle (opaque ID)
using ResourceHandle = uint64_t;

// Texture description
struct TextureDesc {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels = 1;
    enum class Format {
        RGBA8,
        RGBA16F,
        RGBA32F,
        Depth24Stencil8,
        // ... more formats
    } format = Format::RGBA8;
    bool generateMips = false;
};

// Resource manager interface
class IResourceManager {
public:
    virtual ~IResourceManager() = default;
    
    // Load texture from file
    virtual ResourceHandle LoadTexture(const std::string& path) = 0;
    
    // Create texture from description
    virtual ResourceHandle CreateTexture(const TextureDesc& desc, const void* data = nullptr) = 0;
    
    // Load mesh from file
    virtual ResourceHandle LoadMesh(const std::string& path) = 0;
    
    // Load shader from file
    virtual ResourceHandle LoadShader(const std::string& path) = 0;
    
    // Release resource
    virtual void ReleaseResource(ResourceHandle handle) = 0;
};

} // namespace Starstrike::Renderer
```

## Network API

### Network Manager

```cpp
namespace Starstrike::Network {

// Network configuration
struct NetworkConfig {
    uint16_t port = 7777;
    uint32_t maxClients = 16;
    uint32_t tickRate = 60; // Updates per second
    bool enableEncryption = false;
};

// Connection state
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};

// Network event types
enum class NetworkEventType {
    ClientConnected,
    ClientDisconnected,
    DataReceived,
    ConnectionFailed
};

// Network event
struct NetworkEvent {
    NetworkEventType type;
    uint32_t clientId;
    std::vector<uint8_t> data;
};

// Network manager interface
class INetworkManager {
public:
    virtual ~INetworkManager() = default;
    
    // Start server
    virtual bool StartServer(const NetworkConfig& config) = 0;
    
    // Connect to server as client
    virtual bool Connect(const std::string& address, uint16_t port) = 0;
    
    // Disconnect
    virtual void Disconnect() = 0;
    
    // Send data to server (client) or specific client (server)
    virtual void Send(const void* data, size_t size, uint32_t clientId = 0) = 0;
    
    // Poll network events
    virtual std::vector<NetworkEvent> PollEvents() = 0;
    
    // Get connection state
    virtual ConnectionState GetState() const = 0;
    
    // Get network statistics
    virtual NetworkStats GetStats() const = 0;
};

// Network statistics
struct NetworkStats {
    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;
    uint32_t packetsLost = 0;
    float pingMs = 0.0f;
};

} // namespace Starstrike::Network
```

### Network Serialization

```cpp
namespace Starstrike::Network {

// Packet writer
class PacketWriter {
public:
    void WriteByte(uint8_t value);
    void WriteInt16(int16_t value);
    void WriteInt32(int32_t value);
    void WriteInt64(int64_t value);
    void WriteFloat(float value);
    void WriteDouble(double value);
    void WriteString(const std::string& value);
    void WriteBytes(const void* data, size_t size);
    
    const std::vector<uint8_t>& GetData() const;
};

// Packet reader
class PacketReader {
public:
    PacketReader(const void* data, size_t size);
    
    uint8_t ReadByte();
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();
    float ReadFloat();
    double ReadDouble();
    std::string ReadString();
    void ReadBytes(void* buffer, size_t size);
    
    bool HasMore() const;
    size_t GetPosition() const;
};

} // namespace Starstrike::Network
```

## Input API

```cpp
namespace Starstrike::Input {

// Key codes
enum class KeyCode {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Arrow keys
    Left, Right, Up, Down,
    
    // Special keys
    Space, Enter, Escape, Tab, Shift, Control, Alt,
    
    // Mouse buttons
    MouseLeft, MouseRight, MouseMiddle
};

// Input system interface
class IInputSystem {
public:
    virtual ~IInputSystem() = default;
    
    // Keyboard input
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0; // This frame only
    virtual bool IsKeyReleased(KeyCode key) const = 0; // This frame only
    
    // Mouse input
    virtual Math::Vector2 GetMousePosition() const = 0;
    virtual Math::Vector2 GetMouseDelta() const = 0;
    virtual float GetMouseWheel() const = 0;
    
    // Gamepad input
    virtual bool IsGamepadConnected(uint32_t index = 0) const = 0;
    virtual Math::Vector2 GetGamepadLeftStick(uint32_t index = 0) const = 0;
    virtual Math::Vector2 GetGamepadRightStick(uint32_t index = 0) const = 0;
    virtual float GetGamepadLeftTrigger(uint32_t index = 0) const = 0;
    virtual float GetGamepadRightTrigger(uint32_t index = 0) const = 0;
};

} // namespace Starstrike::Input
```

## Math API

```cpp
namespace Starstrike::Math {

// Vector types
struct Vector2 {
    float x, y;
    
    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator*(float scalar) const;
    float Dot(const Vector2& other) const;
    float Length() const;
    Vector2 Normalized() const;
};

struct Vector3 {
    float x, y, z;
    
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(float scalar) const;
    float Dot(const Vector3& other) const;
    Vector3 Cross(const Vector3& other) const;
    float Length() const;
    Vector3 Normalized() const;
};

struct Vector4 {
    float x, y, z, w;
    
    Vector4 operator+(const Vector4& other) const;
    Vector4 operator-(const Vector4& other) const;
    Vector4 operator*(float scalar) const;
    float Dot(const Vector4& other) const;
    float Length() const;
    Vector4 Normalized() const;
};

// Quaternion for rotations
struct Quaternion {
    float x, y, z, w;
    
    static Quaternion Identity();
    static Quaternion FromEuler(float pitch, float yaw, float roll);
    static Quaternion FromAxisAngle(const Vector3& axis, float angle);
    
    Quaternion operator*(const Quaternion& other) const;
    Vector3 RotateVector(const Vector3& v) const;
    Matrix4x4 ToMatrix() const;
    Quaternion Normalized() const;
};

// 4x4 Matrix
struct Matrix4x4 {
    float m[16]; // Column-major order
    
    static Matrix4x4 Identity();
    static Matrix4x4 Translation(const Vector3& position);
    static Matrix4x4 Rotation(const Quaternion& rotation);
    static Matrix4x4 Scale(const Vector3& scale);
    static Matrix4x4 Perspective(float fov, float aspect, float near, float far);
    static Matrix4x4 Orthographic(float left, float right, float bottom, float top, float near, float far);
    static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Vector4 operator*(const Vector4& v) const;
    Matrix4x4 Transposed() const;
    Matrix4x4 Inverted() const;
};

// Utility functions
float Lerp(float a, float b, float t);
float Clamp(float value, float min, float max);
float Deg2Rad(float degrees);
float Rad2Deg(float radians);

} // namespace Starstrike::Math
```

## Threading Utilities

```cpp
namespace Starstrike::Threading {

// Job system for parallel task execution
class JobSystem {
public:
    // Initialize with number of worker threads (0 = hardware concurrency)
    static void Initialize(uint32_t numThreads = 0);
    
    // Submit job for execution
    template<typename Func>
    static void Submit(Func&& func);
    
    // Wait for all jobs to complete
    static void WaitForAll();
    
    // Shutdown job system
    static void Shutdown();
};

} // namespace Starstrike::Threading
```

## Error Handling

```cpp
namespace Starstrike {

// Exception base class
class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& message);
};

// Specific exception types
class InitializationException : public Exception { using Exception::Exception; };
class ResourceException : public Exception { using Exception::Exception; };
class NetworkException : public Exception { using Exception::Exception; };
class RenderException : public Exception { using Exception::Exception; };

} // namespace Starstrike
```

## Usage Examples

### Creating a Game

```cpp
#include <starstrike/Engine.h>

int main() {
    using namespace Starstrike;
    
    // Configure engine
    Engine::EngineConfig config;
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    config.windowTitle = "My Space Game";
    config.vsync = true;
    
    // Create engine
    auto engine = Engine::Engine::Create(config);
    
    // Get world for entity management
    auto* world = engine->GetWorld();
    
    // Create player entity
    auto player = world->CreateEntity();
    auto& transform = world->AddComponent<Engine::TransformComponent>(player);
    transform.position = {0.0f, 0.0f, 0.0f};
    
    // Run game loop
    engine->Run();
    
    return 0;
}
```

### Network Server Example

```cpp
#include <starstrike/Network.h>

void RunServer() {
    using namespace Starstrike::Network;
    
    auto* networkMgr = GetNetworkManager();
    
    NetworkConfig config;
    config.port = 7777;
    config.maxClients = 16;
    config.tickRate = 60;
    
    if (!networkMgr->StartServer(config)) {
        // Handle error
        return;
    }
    
    // Game loop
    while (running) {
        // Poll network events
        auto events = networkMgr->PollEvents();
        
        for (const auto& event : events) {
            if (event.type == NetworkEventType::ClientConnected) {
                // Handle new client
            } else if (event.type == NetworkEventType::DataReceived) {
                // Handle received data
            }
        }
        
        // Update game state
        // Send state to clients
    }
}
```

## API Stability

**Current Status**: Pre-alpha - API is subject to change

Once the project reaches stable release:
- **Major version**: Breaking changes allowed
- **Minor version**: Backward-compatible additions
- **Patch version**: Bug fixes only

## Contributing to API

When proposing API changes:
1. Consider backward compatibility
2. Provide clear use cases
3. Include example usage
4. Document performance implications
5. Consider cross-platform implications

---

**Note**: This API documentation will evolve as the implementation progresses. Check back regularly for updates.
