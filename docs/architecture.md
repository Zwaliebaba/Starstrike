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

**Current limitation:** The three server-related source files in `NeuronClient` (`server.cpp`, `servertoclient.cpp`, `preferences.cpp`) directly `#include "GameApp.h"`, which transitively pulls in the full rendering, audio, and input stack. This prevents `NeuronServer` from being linked into a standalone headless process. See [Server.md](../Server.md) for the `SERVER_BUILD` preprocessor guard fix and the accompanying `Dockerfile` for Windows Server Core container deployment.

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

### TreeRenderer — Reference Implementation for Extracted Renderers

`NeuronClient/tree_renderer.h/.cpp` is the first building type to have its rendering fully extracted from `GameLogic`. It owns a dedicated DirectX 12 PSO, per-tree GPU buffer management, and an inversion-of-control interface (`ITreeRenderBackend`) for GPU resource cleanup when simulation objects are destroyed. The call site in `Starstrike/location.cpp` dispatches to `TreeRenderer::Get().DrawTree(...)` directly.

A follow-on plan ([docs/tree.md](../docs/tree.md)) decouples `TreeRenderer` from `d3d12_backend.h` and `opengl_directx.h` so that it can eventually move to a dedicated `GameRender` project.

The broader simulation–rendering decoupling work (covering all 67 remaining `GameLogic` files) uses `TreeRenderer` as its reference implementation. See [docs/gamelogic.md](../docs/gamelogic.md).

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

See [CI.md](../CI.md) for the full phased improvement plan and links to each detailed plan document. Key standing debt items:

| Category | Count | Notes |
|---|---|---|
| Unsafe C string functions (`sprintf`, `strcpy`, etc.) | 75+ | Buffer-overflow risk; replace with `snprintf` / `std::format` in new or modified code |
| Raw `new`/`delete` without RAII | 100+ | Target for `unique_ptr` migration, starting with `NeuronCore` and `Starstrike` |
| Plain `enum` (should be `enum class`) | 9 | Type-safety gap; straightforward per-file fix |
| `#ifndef` include guards (should be `#pragma once`) | 212+ | ODR risk from guard-name typos; mechanical replacement |
| Commented-out code blocks | 30+ | Should be deleted; rely on git history |
| Stale `#define` branches | 6 | Dead preprocessor paths; delete unreachable branches |

Active modernisation initiatives (math migration, matrix convention, server separation, simulation–rendering decoupling) are tracked in [CI.md](../CI.md).

---

## Architecture Decision Records

Significant design decisions made during the modernisation programme are recorded here for future reference.

### ADR-001 — Row-major, row-vector matrix convention (2024)

**Decision:** Standardise all matrix operations on the DirectX row-major, row-vector convention (`mul(v, M)` in HLSL). Do not introduce a column-vector convention or transpose shims.

**Rationale:** The legacy `Matrix34` data layout (`r`, `u`, `f`, `pos` rows packed sequentially) is already byte-compatible with a DirectX row-major `XMFLOAT4X4` — no memory transposition is needed. `ConvertToOpenGLFormat` was performing a conceptual relabeling, not an actual data transformation. The DX12 HLSL shaders already use `row_major float4x4` and `mul(v, M)`. Standardising on this avoids a global transpose and lets native `XMMatrixMultiply` be used without wrapper gymnastics.

**Consequence:** `Matrix33 * vec` applies the inverse rotation (used intentionally for world-to-local queries). `vec * Matrix33` applies the forward rotation. See [MatrixConv.md](../MatrixConv.md) for the full operator table and migration phases.

### ADR-002 — Incremental extraction over ECS rewrite (2024)

**Decision:** Decouple simulation from rendering using incremental per-type extraction (one PR per entity/building type) rather than a wholesale ECS rewrite.

**Rationale:** A clean ECS rewrite would halt all other development for months and introduce significant regression risk across 77 files. Incremental extraction keeps the game shippable at every step. `TreeRenderer` proves the approach works. The `BuildingRenderer`/`EntityRenderer` base-class + registry pattern provides a uniform dispatch mechanism that scales to 40+ types without per-type `if/else` chains.

**Consequence:** A transition period where old virtual-dispatch rendering and new registry rendering coexist. The fallback `else` path in `BuildingRenderRegistry` / `EntityRenderRegistry` handles unmigrated types transparently.

### ADR-003 — `winrt::com_ptr<T>` over `Microsoft::WRL::ComPtr<T>` (project start)

**Decision:** Use `winrt::com_ptr<T>` for all COM object lifetime management throughout the codebase.

**Rationale:** The project uses C++/WinRT for WinUI 3. Mixing `WRL::ComPtr` and `winrt::com_ptr` in the same codebase creates friction at API boundaries and inconsistent usage patterns. `winrt::com_ptr` integrates naturally with the WinUI 3 programming model.

**API surface:** `.get()` (not `.Get()`), `= nullptr` (not `.Reset()`), `IID_GRAPHICS_PPV_ARGS(ptr)` (not `IID_PPV_ARGS(&ptr)`).
