# Starstrike Architecture

## Overview

Starstrike is architected as a modern, modular game engine with a focus on high-performance rendering using DirectX 12 and scalable multiplayer networking. The architecture follows industry-standard patterns for game engines while maintaining flexibility for rapid iteration.

## Architectural Principles

1. **Separation of Concerns**: Clear boundaries between engine systems, rendering, game logic, and networking
2. **Data-Oriented Design**: Performance-critical systems use cache-friendly data layouts
3. **Modern C++ Practices**: RAII, smart pointers, move semantics, and compile-time polymorphism where appropriate
4. **Platform Abstraction**: Core systems abstract platform-specific implementations
5. **Deterministic Networking**: Game state synchronization designed for fairness and consistency

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Game Application                         │
│                     (Game-Specific Logic)                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────┴──────────────────────────────────────┐
│                     Core Engine Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │   Scene     │  │   Entity    │  │    Resource         │ │
│  │  Manager    │  │  Component  │  │   Management        │ │
│  │             │  │   System    │  │                     │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────┴──────────────────────────────────────┐
│                   Platform Services                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  Renderer   │  │  Network    │  │      Input          │ │
│  │  (DX12)     │  │   Layer     │  │    System           │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Core Modules

### 1. Renderer Module (DirectX 12)

The renderer is built on DirectX 12 and implements modern rendering techniques.

**Key Components:**
- **Command Queue Manager**: Manages graphics, compute, and copy command queues
- **Descriptor Heap Manager**: Efficient descriptor allocation and management
- **Resource Manager**: Texture, buffer, and shader resource lifecycle management
- **PSO Cache**: Pipeline State Object caching for optimal performance
- **Frame Graph**: Dependency-based rendering pipeline for automatic synchronization

**Ownership Model:**
- GPU resources managed via `ComPtr<>` (Microsoft COM smart pointer)
- CPU-side resources use standard smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Explicit resource barriers for state transitions

**Threading Model:**
- Multi-threaded command list recording
- Per-frame command allocator pools
- Lock-free descriptor allocation using atomic operations

**Performance Considerations:**
- Bindless resource access where supported
- Persistent descriptor heaps
- Upload heap pooling to reduce allocation overhead
- Asynchronous resource uploads via copy queue

### 2. Entity Component System (ECS)

A data-oriented ECS for game object management.

**Design:**
- Entities are lightweight IDs (64-bit handles)
- Components stored in contiguous memory (struct-of-arrays)
- Systems operate on component archetypes for cache efficiency

**Key Systems:**
- Transform System: Hierarchical transforms with dirty flag propagation
- Physics System: Collision detection and response
- Render System: Culling and draw call generation
- Network Replication System: State synchronization

**Component Lifecycle:**
- Components are Plain Old Data (POD) types when possible
- No virtual functions in components
- RAII wrappers for resource-owning components

### 3. Networking Architecture

Multi-user networking for competitive multiplayer gameplay.

**Architecture Pattern:**
- **Client-Server Model**: Authoritative server for game state
- **Client-Side Prediction**: Responsive controls with server reconciliation
- **Server Reconciliation**: Handles client prediction mismatches
- **Entity Interpolation**: Smooth visual representation of network entities

**Network Protocol:**
- UDP-based with custom reliability layer
- Delta compression for bandwidth efficiency
- Priority-based packet scheduling
- Lag compensation for hit detection

**Synchronization:**
- Tick-based simulation (e.g., 60Hz server update rate)
- Snapshot interpolation for remote entities
- Command buffering and replay for reconciliation
- Deterministic physics simulation

**Threading:**
- Dedicated network thread for packet I/O
- Lock-free queues for thread communication
- Minimal locking in main game thread

**Security Considerations:**
- Input validation on server
- Cheat detection for impossible moves
- Rate limiting to prevent DoS attacks
- Encrypted connections (to be implemented)

### 4. Resource Management

Unified resource management system for assets.

**Features:**
- Asynchronous resource loading
- Reference-counted resources with automatic unloading
- Hot-reloading support for rapid iteration
- Resource dependency tracking

**Resource Types:**
- Meshes (vertex and index buffers)
- Textures (2D, cube maps, arrays)
- Shaders (vertex, pixel, compute)
- Materials (shader + texture bindings)
- Audio (sound effects, music)

### 5. Memory Management

**Strategies:**
- **Stack Allocators**: For per-frame temporary allocations
- **Pool Allocators**: For fixed-size objects (entities, particles)
- **GPU Upload Heaps**: Ring buffer for dynamic GPU uploads
- **Custom Allocators**: Profiled and optimized per subsystem

**Tools:**
- Memory tracking for leak detection (debug builds)
- Allocation profiling hooks
- GPU memory budget management

### 6. Input System

**Features:**
- Multi-device support (keyboard, mouse, gamepad)
- Input action mapping (configurable key bindings)
- Input buffering for network synchronization
- Dead zone and sensitivity configuration

## Threading Model

**Main Threads:**
1. **Main Thread**: Game logic, ECS updates, high-level rendering commands
2. **Render Thread**: Command list recording, PSO management
3. **Network Thread**: Socket I/O, packet processing
4. **Resource Thread**: Asynchronous asset loading and decompression

**Synchronization:**
- Lock-free queues between threads where possible
- Minimal mutex usage, prefer atomic operations
- Double/triple buffering for render data
- Fence-based GPU synchronization

## Build System

(To be determined - CMake recommended for cross-IDE support)

**Planned Structure:**
- Separate static libraries per module
- Unit tests integrated with build
- Automated shader compilation
- Asset pipeline integration

## Platform Abstraction

**Abstracted Interfaces:**
- Window creation and event handling
- File I/O and path handling
- Network sockets (Windows Sockets with potential cross-platform layer)
- Threading primitives

**Platform-Specific Code:**
- DirectX 12 renderer (Windows-only)
- Windows-specific optimizations (IOCP for networking)

## Performance Considerations

**CPU:**
- Cache-friendly data layouts
- Minimize virtual function calls in hot paths
- Compile-time polymorphism via templates where appropriate
- SIMD intrinsics for math operations

**GPU:**
- Minimize state changes and PSO switches
- Batch draw calls by material and mesh
- GPU-driven rendering where supported (ExecuteIndirect)
- Async compute for post-processing

**Memory:**
- Minimize allocations in per-frame code
- Reuse command allocators and descriptor heaps
- Pool frequently allocated objects

## Code Organization

**Namespace Structure:**
```cpp
namespace Starstrike {
    namespace Engine {
        // Core engine systems
    }
    namespace Renderer {
        namespace DX12 {
            // DirectX 12 implementation
        }
    }
    namespace Network {
        // Networking layer
    }
    namespace Game {
        // Game-specific code
    }
}
```

**Header Organization:**
- Public API headers: `include/starstrike/`
- Private implementation: `src/`
- Platform-specific: `src/platform/`

## API Boundaries

**Public API:**
- Core engine interfaces
- Entity/component creation
- Resource loading
- Input querying

**Private Implementation:**
- DirectX 12 details
- Network protocol internals
- Memory allocators
- Platform-specific code

**ABI Considerations:**
- C++17/20 features used throughout (no C API layer planned)
- Static linking recommended for deployment
- No plugin system initially (may be added later)

## Compile-Time vs Runtime Polymorphism

**Compile-Time (Templates):**
- Math library (vectors, matrices)
- Allocator policies
- Component system type dispatch

**Runtime (Virtual Functions):**
- High-level game states
- UI systems
- Platform abstraction interfaces

## Error Handling

**Strategy:**
- Exceptions for initialization failures
- Error codes/return values for runtime operations
- Assertions for programming errors (debug builds)
- Logging system for diagnostics

**Graphics Errors:**
- DirectX Debug Layer in debug builds
- GPU validation (PIX, RenderDoc)
- Graceful degradation where possible

## Testing Strategy

**Planned Tests:**
- Unit tests for core systems (entity system, math library)
- Integration tests for subsystem interactions
- Networking simulation tests (packet loss, latency)
- Rendering validation tests (reference images)

## Security Considerations

**Client Security:**
- Input sanitization
- Bounds checking on all network data
- Memory safety via RAII and smart pointers

**Server Security:**
- Rate limiting per client
- Input validation and sanity checks
- Anti-cheat heuristics
- DoS prevention

## Open Questions / Future Considerations

- **Build System**: CMake vs Visual Studio solution
- **Scripting**: Potential integration of Lua or similar for gameplay
- **Audio System**: Choice of audio middleware (FMOD, Wwise, or custom)
- **Physics Engine**: Custom vs third-party (PhysX, Bullet)
- **Platform Support**: Future Linux/Vulkan port?
- **Profiling**: Integration with external profilers (Tracy, Superluminal, PIX)

## References

- DirectX 12 Best Practices: Microsoft DX12 Programming Guide
- Network Architecture: "Networked Physics" by Glenn Fiedler
- ECS Design: "Data-Oriented Design" by Richard Fabian
- Game Engine Architecture: "Game Engine Architecture" by Jason Gregory

---

**Document Status**: Initial architectural design - subject to revision as implementation progresses.
