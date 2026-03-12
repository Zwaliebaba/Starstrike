# Architecture

Starstrike is structured as a Visual Studio solution containing one executable project and four static library projects. Each library owns a clearly-scoped set of responsibilities with no circular dependencies.

## Module Overview

| Module | Type | Responsibility |
|---|---|---|
| `Starstrike` | Executable (MSIX) | Application entry point, main game loop, DirectX 12 renderer, camera, particle system, level management |
| `GameLogic` | Static lib | Game entities, buildings, AI, scripting, menus, input-field UI |
| `NeuronClient` | Static lib | Legacy rendering shim (OpenGL → DX12), input driver stack, 3D audio, legacy UI framework, math types |
| `NeuronCore` | Static lib | Modern C++ utilities: DirectXMath wrappers, binary I/O, file system, networking, threading, timers |
| `NeuronServer` | Static lib | Network server implementation |

## Dependency Graph

```
Starstrike
 ├── NeuronClient ──► NeuronCore
 │                ──► GameLogic
 ├── GameLogic    (no upstream deps)
 ├── NeuronCore   (no upstream deps)
 └── (implicitly) NeuronServer ──► NeuronCore
```

Rules enforced by the solution:
- `GameLogic` must not import `NeuronClient` or `Starstrike`.
- `NeuronCore` must not import any other project.
- `NeuronServer` must not import `GameLogic` or `NeuronClient`.

## Starstrike (Main Application)

**Key files:**

| File | Purpose |
|---|---|
| `WinMain.cpp` | Win32/WinUI 3 entry point |
| `main.h/cpp` | Game initialisation and main loop tick |
| `GameApp.h/cpp` | Application class: window management, state machine |
| `renderer.h/cpp` | Direct3D 12 render coordinator |
| `camera.h/cpp` | 3D camera control |
| `location.h/cpp` | Level/location simulation |
| `global_world.h/cpp` | Persistent world state across locations |
| `entity_grid.h/cpp` | Spatial grid for entity queries |
| `particle_system.h/cpp` | Particle effects |
| `landscape.h/cpp` | Terrain data and rendering |
| `water.h/cpp` | Water simulation |
| `unit.h/cpp` | Unit wrapper |
| `routing_system.h/cpp` | Unit pathfinding |
| `script.h/cpp` | Gameplay scripting |
| `taskmanager.h/cpp` | Task/objective UI |
| `user_input.h/cpp` | Top-level input handling |

**Tick order:**
```
Input → GameApp::Advance() → Location::Advance() → EntityGrid → Entities/Buildings → Renderer
```

## GameLogic

**Responsibilities:** All game-play rules, entity behaviour, building logic, AI, and in-game menu UI.

### Entity Hierarchy

`WorldObject` → `Entity` → (18+ concrete types)

Selected entity types:
`LaserTroop`, `Engineer`, `Virii`, `InsertionSquadie`, `Egg`, `SporeGenerator`, `Lander`, `Tripod`, `Centipede`, `SpaceInvader`, `Spider`, `Darwinian`, `Officer`, `ArmyAnt`, `Armour`, `SoulDestroyer`, `TriffidEgg`, `AI`

### Building Hierarchy

`WorldObject` → `Building` → (57+ concrete types)

Selected building types:
`Factory`, `Cave`, `RadarDish`, `ControlTower`, `GunTurret`, `Bridge`, `Generator`, `Incubator`, `AntHill`, `SpawnPoint`, `BlueprintStore`, `EscapeRocket`, and many more.

Buildings are created through a factory function in `building.cpp` that dispatches on a type enum.

### AI System

`ai.h/cpp` manages autonomous agent behaviour. Individual unit types implement their own `Advance()` methods; the AI system provides coordination and task assignment.

## NeuronClient

**Responsibilities:** All legacy subsystems inherited from the original codebase — rendering shim, input stack, audio, math types, and the Eclipse UI framework.

### OpenGL → DirectX 12 Shim

`opengl_directx.cpp/h` translates legacy OpenGL draw calls (`glVertex3fv`, `glMultMatrixf`, etc.) into Direct3D 12 command-list submissions. This layer exists to avoid rewriting all legacy rendering code at once. New rendering work should target the DX12 API directly via `Starstrike/renderer.h`.

### Input Stack

The input system is layered:

```
Raw device events
  → InputDriver (simple / conjoin / idle / alias / value / withdelta)
  → InputFilter (debounce, axis scaling, dead zones)
  → InputSpec (named binding)
  → ControlBindings (action → binding map)
```

### Audio

`soundsystem.cpp/h` manages all audio playback. `sound_library_3d.cpp/h` provides 3D spatial audio with a DirectSound backend. `sound_parameter.cpp/h` and `sound_filter.cpp/h` handle dynamic audio parameters and DSP filters.

### Legacy Math Types

| Type | Description |
|---|---|
| `LegacyVector3` | 3-component float vector (`x`, `y`, `z`) |
| `Matrix33` | 3×3 rotation/scale matrix |
| `Matrix34` | 4×3 affine transform (right, up, forward, position rows) |
| `Vector2` | 2D float vector |

These types are being migrated to DirectXMath-backed equivalents. See [MathPlan.md](../MathPlan.md).

### Eclipse UI Framework

`eclipse.cpp/h`, `eclbutton.h/cpp`, `eclwindow.h/cpp` implement the legacy windowed UI system used by in-game panels and menus. No new UI should be added to this framework; new UI should use WinUI 3.

## NeuronCore

**Responsibilities:** Modern, platform-independent utilities written to C++20 standards.

| File | Purpose |
|---|---|
| `GameMath.h` | DirectXMath wrappers in the `Neuron::Math` namespace |
| `MathCommon.h` | Bit utilities, constants |
| `DataWriter.h` / `DataReader.h` | Binary serialisation |
| `FileSys.h` | File system operations (ANSI/Wide variants) |
| `Debug.h` | `Neuron::DebugTrace`, `Neuron::Fatal`, assertions |
| `ASyncLoader.h` | Asynchronous resource loading (pending refactor to condition variable) |
| `net_lib.h/cpp` | Platform-independent network interface |
| `net_socket.h/cpp` | Socket abstraction |
| `TimerCore.h` | High-precision timer (`QueryPerformanceCounter`) |
| `Hash.h` | Hash utilities |
| `WndProcManager.h/cpp` | Window-message routing |

All new cross-cutting utilities should be added to NeuronCore.

## NeuronServer

Minimal static library providing the network server. Contains precompiled headers and the server project file. Implementation is in `NeuronServer.vcxproj`.

## Rendering Pipeline

```
Game state
  → Starstrike/renderer.h (DX12 coordinator)
      → Direct3D 12 command lists
      → Compiled HLSL shaders (NeuronClient/CompiledShaders/*.h)
      → GPU resources (GpuBuffer, GpuResource, PipelineState, RootSignature)
      → SpriteBatch (2D/UI quads)

Legacy path (via OpenGL shim):
  → NeuronClient/opengl_directx.cpp
      → Translates gl* calls → DX12 command list
```

New rendering features should target the DX12 path directly. Do not add new functionality to `ImRenderer`; use `SpriteBatch` for new textured-quad work.

## Networking Architecture

The network layer uses a UDP client-server model:

```
NeuronClient/clienttoserver.h  ──► NeuronCore/net_socket.h ──► OS sockets
NeuronClient/servertoclient.h  ──► NeuronCore/net_socket_listener.h

NeuronServer ──► NeuronCore/net_lib.h
```

Network updates are serialised via `NeuronClient/networkupdate.h` and identified by sequence IDs.

## Key Architectural Patterns

### Factory Pattern (Buildings)
`Building::Create(int type)` in `building.cpp` returns a `Building*` to the appropriate subclass. The type enum has 57+ values. New building types must register an entry in this factory.

### Spatial Query (EntityGrid)
`EntityGrid` in `Starstrike/entity_grid.h` partitions entities into a 2D grid for efficient radius queries. All per-frame proximity checks go through this grid, not raw entity iteration.

### Precompiled Headers
Every project uses `pch.h` / `pch.cpp`. All `.cpp` files must include `pch.h` as the first `#include`. When adding new translation units, configure PCH usage in the `.vcxproj` (`Use` for `.cpp`, `Create` for `pch.cpp`).

### COM Smart Pointers
Use `winrt::com_ptr<T>` (not `Microsoft::WRL::ComPtr<T>`) for all COM object lifetime management:
- `.get()` to obtain the raw pointer (not `.Get()`)
- `= nullptr` to release (not `.Reset()`)
- `IID_GRAPHICS_PPV_ARGS(ptr)` when the target is a `com_ptr` (not `IID_PPV_ARGS(&ptr)`)

## Known Technical Debt

See [CI.md](../CI.md) for the full phased improvement plan. Key items:

| Category | Count | Notes |
|---|---|---|
| Unsafe C string functions (`sprintf`, `strcpy`, etc.) | 75+ | Buffer-overflow risk |
| Raw `new`/`delete` without RAII | 100+ | Target for `unique_ptr` migration |
| Plain `enum` (should be `enum class`) | 9 | Type-safety gap |
| `#ifndef` include guards (should be `#pragma once`) | 212+ | ODR risk from typos |
| Commented-out code blocks | 30+ | Should be deleted |
| Stale `#define` branches | 6 | Dead preprocessor paths |
