# Decouple Simulation from Rendering in `GameLogic`

## Goal

Split the `GameLogic` static library into a **pure-simulation** library
(`GameSim`) that compiles without any GPU, rendering, or windowing headers,
and a **render-companion** library (`GameRender`) that holds all visual
representation logic.  After this work:

- `NeuronServer` can link `GameSim` directly and run authoritative game state.
- A headless bot client can link `GameSim` without `NeuronClient`.
- The existing `Starstrike` executable links both `GameSim` + `GameRender`
  and behaves identically to today.

---

## Current State Analysis

### The Coupling

Every game object — `WorldObject`, `Entity`, `Building` — carries a
`virtual void Render(float)` in its base class.  Subclass `Render()`
implementations call OpenGL translation-layer functions (`glBegin`,
`glVertex`, `glEnable`, `glDisable`, `glColor`, `glTexCoord`,
`g_app->m_renderer->SetObjectLighting()`, `Shape::Render()`, etc.)
directly.

| Metric | Value |
|--------|-------|
| Total `GameLogic/*.cpp` files | 77 |
| Files containing direct GL calls or `m_renderer` references | **67** |
| Base-class render virtuals | `WorldObject::Render`, `Entity::Render`, `Building::Render`, `Building::RenderAlphas` |
| Entity types with `Render()` overrides | ~18 (every `Entity` subclass) |
| Building types with `Render()`/`RenderAlphas()` overrides | ~20 |

### Why It Blocks Server-Owned State

1. **`NeuronServer` cannot link `GameLogic`.**
   `GameLogic/pch.h` → `NeuronClient.h` → `opengl_directx.h` → D3D12
   device headers.  A headless server has no GPU.
2. **Entity constructors load rendering assets.**
   e.g. `ArmyAnt::ArmyAnt()` calls `g_app->m_resource->GetShape("armyant.shp")`.
   A headless process has no resource/shape system.
3. **Simulation and rendering share mutable state.**
   `Advance()` and `Render()` both read/write `m_shape`, `m_front`,
   `m_pos`, `m_scale` with no separation of simulation state from
   presentation state.
4. **Side-effects in simulation call rendering APIs.**
   `ArmyAnt::ChangeHealth()` calls `g_explosionManager.AddExplosion(m_shape, transform)`
   and `g_app->m_particleSystem->CreateParticle(...)` — both require GPU
   state.

### PCH Dependency Chain

```
GameLogic/pch.h
  └─ NeuronClient.h
       ├─ NeuronCore.h            ← needed
       ├─ DirectXHelper.h         ← GPU
       ├─ GraphicsCore.h          ← GPU
       ├─ PixProfiler.h           ← GPU
       ├─ GameMain.h              ← client
       └─ opengl_directx.h        ← GL translation layer / GPU
```

Only `NeuronCore.h` is needed for pure simulation.

---

## Target Architecture

```
┌─────────────────────────────────────────────────────┐
│                    GameSim (new lib)                 │
│  Pure simulation: Entity, Building, AI, weapons,    │
│  pathfinding, economy, spirit logic.                │
│  NO GL, NO D3D12, NO Shape*, NO Renderer*.          │
│  pch.h includes NeuronCore.h only.                  │
│  Compiles for server AND client.                    │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
    ┌──────────▼──────────┐  ┌────────▼────────────┐
    │   NeuronServer      │  │  GameRender (new)    │
    │   (headless)        │  │  Render logic for    │
    │   Links: NeuronCore │  │  entities, buildings │
    │         + GameSim   │  │  Links: NeuronClient │
    │                     │  │        + GameSim     │
    └─────────────────────┘  └────────┬────────────┘
                                      │
                             ┌────────▼────────────┐
                             │   Starstrike (exe)   │
                             │   Links: NeuronClient│
                             │        + GameSim     │
                             │        + GameRender  │
                             └─────────────────────┘
```

A **headless bot client** links `NeuronCore` + `GameSim` only (no
`GameRender`, no `NeuronClient`).

---

## Strategy: Incremental Extraction

A clean ECS rewrite would be ideal but would halt all other work for months.
This plan uses an **incremental extraction** approach — each phase leaves the
game shippable.

---

## Phase 0 — Preparation (Zero Code Risk)

### 0.1 — Audit every `Render` method

Categorize every `Render()` / `RenderAlphas()` override in `GameLogic`:

| Complexity | Description | Examples |
|------------|-------------|---------|
| **Trivial** | Just `m_shape->Render(predTime, mat)` | `Building::Render`, `Egg::Render` |
| **Medium** | Shape + GL state changes (lighting, texture, blend) | `ArmyAnt::Render`, `Engineer::Render` |
| **Complex** | Custom GL geometry, particles, pixel effects | `Virii::Render`, `Darwinian::Render`, `SoulDestroyer::Render` |

This determines migration order: trivial first, complex last.

### 0.2 — Audit `ShapeMarker*` usage in simulation

Some entity types use `ShapeMarker*` for gameplay-visible attachment points
(e.g. `ArmyAnt::GetCarryMarker()` positions a carried spirit).  These need
to be replaced with data-driven offsets before `Shape*` can leave the
simulation classes.

| Entity | Marker Usage | Replacement |
|--------|-------------|-------------|
| `ArmyAnt` | `m_carryMarker` → spirit attach position | Data-driven `LegacyVector3` offset |
| `Building` | `m_lights`, `m_ports` → marker positions | TBD per building type |

### 0.3 — Audit side-effect calls in simulation

Calls from `Advance()` / `ChangeHealth()` / etc. into rendering systems:

| Pattern | Examples | Replacement |
|---------|----------|-------------|
| `g_explosionManager.AddExplosion(shape, transform)` | `ArmyAnt::ChangeHealth` | Event queue |
| `g_app->m_particleSystem->CreateParticle(...)` | `ArmyAnt::AdvanceAttackEnemy` | Event queue |
| `g_app->m_soundSystem->TriggerEntityEvent(...)` | `ArmyAnt::AdvanceAttackEnemy` | Event queue |
| `g_app->m_renderer->SetObjectLighting()` | Various `Render()` bodies | Moves to renderer companion |

---

## Phase 1 — Break the PCH Dependency

Highest-value, lowest-risk change.  Makes `GameLogic` compilable without GPU
headers behind a preprocessor guard.

### 1.1 — Create `GameLogic/GameLogicPlatform.h`

```cpp
// GameLogic/GameLogicPlatform.h
#pragma once

#include "NeuronCore.h"

// Forward declarations for rendering types GameLogic still references
// during the transition period.
class Shape;
class ShapeFragment;
class ShapeMarker;
class Renderer;

#if defined(GAMELOGIC_HAS_RENDERER)
// Client build — full GL translation layer available.
#include "NeuronClient.h"
#else
// Server / headless build — no rendering headers.
// Phase 2+ will eliminate the need for this branch entirely.
#endif
```

### 1.2 — Change `GameLogic/pch.h`

```diff
- #include "NeuronClient.h"
+ #include "GameLogicPlatform.h"
```

### 1.3 — Add preprocessor define to `GameLogic.vcxproj`

Add `GAMELOGIC_HAS_RENDERER` to the preprocessor definitions for both
Debug and Release configurations.  This preserves current behavior — nothing
breaks.

The server build of `GameLogic` (or future `GameSim`) omits this define.

---

## Phase 2 — Extract Rendering into Companion Classes

### Core Pattern: Render Companion

For each game object type, split the class into a simulation half and a
render half.

**Before** (single class):
```
GameLogic/armyant.h    → class ArmyAnt : Entity { Advance(); Render(); }
GameLogic/armyant.cpp  → #includes GL headers, implements both
```

**After** (two classes):
```
GameSim/armyant.h           → class ArmyAnt : Entity { Advance(); }
GameSim/armyant.cpp         → pure simulation, no GL

GameRender/ArmyAntRenderer.h   → class ArmyAntRenderer : EntityRenderer { Render(); }
GameRender/ArmyAntRenderer.cpp → GL calls, Shape::Render, lighting
```

### `EntityRenderer` base class

```cpp
// GameRender/EntityRenderer.h
#pragma once

struct EntityRenderContext
{
    float predictionTime;
    float highDetailFactor;   // 0.0–1.0, computed from camera distance
};

class Entity;

class EntityRenderer
{
public:
    virtual ~EntityRenderer() = default;
    virtual void Render(const Entity& _entity, const EntityRenderContext& _ctx) = 0;
    virtual void RenderAlphas(const Entity& _entity, const EntityRenderContext& _ctx) {}
};
```

`EntityRenderContext` normalizes all render signatures — including
`Darwinian::Render(float, float)` which currently takes an extra
`_highDetail` parameter.

### `EntityRenderRegistry`

Type-indexed table of render companions replaces polymorphic
`Entity::Render()` dispatch:

```cpp
// GameRender/EntityRenderRegistry.h
#pragma once

#include "entity.h"

class EntityRenderer;

class EntityRenderRegistry
{
public:
    void Register(int _entityType, EntityRenderer* _renderer);
    EntityRenderer* Get(int _entityType) const;

private:
    EntityRenderer* m_renderers[Entity::NumEntityTypes] = {};
};
```

### Call-site change (`Team::Render` family)

```diff
- entity->Render(_predictionTime);
+ EntityRenderer* renderer = g_entityRenderRegistry.Get(entity->m_type);
+ if (renderer)
+     renderer->Render(*entity, ctx);
```

Same pattern for `BuildingRenderer` / `BuildingRenderRegistry`.

### Migration Order

Each entity type is a self-contained PR:

**Wave 1 — Trivial** (just `Shape::Render`):
- `ArmyAnt`, `Egg`, `Officer`, `TriffidEgg`

**Wave 2 — Medium** (shape + GL state):
- `Engineer`, `Armour`, `Lander`, `Tripod`, `Centipede`, `Spider`

**Wave 3 — Complex** (custom geometry / special signatures):
- `Darwinian` (LOD render), `Virii` (particle rendering),
  `SoulDestroyer` (multi-segment), `LaserTrooper`

**Wave 4 — Buildings**:
- Same pattern with `BuildingRenderer` / `BuildingRenderRegistry`

**Wave 5 — UI / debug windows**:
- Already client-only; move to `GameRender` without logic changes

### Per-Entity Migration Checklist

For each entity type:

- [ ] Create `GameRender/<Type>Renderer.h/.cpp`
- [ ] Move `Render()` body (and `RenderAlphas()` if present) to the renderer
- [ ] Move `Shape*` fields and asset loading to the renderer
- [ ] Replace `ShapeMarker*` simulation usage with data-driven offsets
- [ ] Register the renderer in `EntityRenderRegistry` init
- [ ] Remove GL includes from the simulation `.cpp`
- [ ] Verify `Advance()` has no remaining rendering dependencies
- [ ] Build and test in all configurations

---

## Phase 3 — Introduce `SimEventQueue` for Side Effects

Simulation code currently calls rendering/audio systems directly during
`Advance()` and `ChangeHealth()`.  Replace these with a deferred event
queue that the client drains after simulation and the server discards.

### `SimEvent` struct

```cpp
// GameSim/SimEvents.h
#pragma once

#include "LegacyVector3.h"

struct SimEvent
{
    enum Type
    {
        Explosion,
        ParticleSpawn,
        SoundTrigger,
    };

    Type          type;
    LegacyVector3 pos;
    LegacyVector3 vel;
    int           entityType;    // Renderer uses this to pick shape
    float         scale;
    int           soundEventId;  // For SoundTrigger events
};
```

### `SimEventQueue`

```cpp
// GameSim/SimEventQueue.h
#pragma once

#include "SimEvents.h"

class SimEventQueue
{
public:
    void Push(const SimEvent& _event);
    int  Count() const;
    const SimEvent& Get(int _index) const;
    void Clear();

private:
    // Fixed-size ring buffer — no per-frame allocations.
    static constexpr int MAX_EVENTS = 1024;
    SimEvent m_events[MAX_EVENTS];
    int      m_count = 0;
};
```

### Replacement example — `ArmyAnt::ChangeHealth()`

```diff
- g_explosionManager.AddExplosion(m_shape, transform);
+ SimEvent evt;
+ evt.type       = SimEvent::Explosion;
+ evt.pos        = m_pos;
+ evt.entityType = m_type;
+ evt.scale      = m_scale;
+ g_simEventQueue.Push(evt);
```

### Client drain loop (in `Starstrike` main loop, after `Location::Advance`)

```cpp
for (int i = 0; i < g_simEventQueue.Count(); ++i)
{
    const SimEvent& evt = g_simEventQueue.Get(i);
    switch (evt.type)
    {
    case SimEvent::Explosion:
        // Look up shape from entity type via render registry
        g_explosionManager.AddExplosion(GetShapeForType(evt.entityType),
                                        BuildTransform(evt));
        break;
    case SimEvent::ParticleSpawn:
        g_app->m_particleSystem->CreateParticle(evt.pos, evt.vel,
                                                 Particle::TypeMuzzleFlash);
        break;
    case SimEvent::SoundTrigger:
        // dispatch to sound system
        break;
    }
}
g_simEventQueue.Clear();
```

Server ignores or logs for replay.

---

## Phase 4 — Remove `Render()` from Base Classes

Once **all** entity and building types have render companions registered:

1. Remove `virtual void Render(float)` from `WorldObject`.
2. Remove `virtual void Render(float)` from `Entity`.
3. Remove `virtual void Render(float)` / `RenderAlphas(float)` from `Building`.
4. Remove `Shape* m_shape` from `Entity` and `Building`.
5. Remove `Shape* m_shapes[]` from entity subclasses (e.g. `ArmyAnt`).
6. Remove `#include "renderer.h"` / `#include "shape.h"` from all
   simulation files.

---

## Phase 5 — Create `GameSim` and `GameRender` Projects

### 5.1 — `GameSim/GameSim.vcxproj` (static library)

- Move all simulation `.cpp/.h` from `GameLogic/` into `GameSim/`.
- `pch.h` includes `NeuronCore.h` only.
- Additional include directories: `$(SolutionDir)NeuronCore`.
- **No** `NeuronClient` dependency.
- **No** `GAMELOGIC_HAS_RENDERER` define.

### 5.2 — `GameRender/GameRender.vcxproj` (static library)

- Contains all `*Renderer.cpp/.h` files.
- Contains `EntityRenderRegistry`, `BuildingRenderRegistry`.
- Contains `SimEventQueue` drain/dispatch logic.
- `pch.h` includes `NeuronClient.h`.
- Additional include directories: `$(SolutionDir)NeuronCore`,
  `$(SolutionDir)NeuronClient`, `$(SolutionDir)GameSim`.

### 5.3 — Update link dependencies

| Project | Links |
|---------|-------|
| `Starstrike` | `NeuronClient`, `NeuronCore`, `GameSim`, `GameRender` |
| `NeuronServer` | `NeuronCore`, `GameSim` |
| `BotClient` (future) | `NeuronCore`, `GameSim` |

### 5.4 — Delete `GameLogic/` project

Once all files are migrated, remove `GameLogic.vcxproj` from the solution.

---

## Phase 6 — Separate Simulation State from Presentation State

### 6.1 — Const access for renderers

Render companions receive `const Entity&` / `const Building&`.  Renderers
never write to simulation state.

### 6.2 — Interpolation buffer (optional, future)

For smooth visuals independent of simulation tick rate:

```cpp
struct EntityPresentationState
{
    LegacyVector3 interpPos;
    LegacyVector3 interpFront;
    float         interpScale;
};
```

The main loop runs simulation at a fixed timestep and interpolates
presentation state for the renderer.  This also enables the server to run
simulation at a different rate than the client framerate.

---

## Phase 7 — Headless Bot Client

With `GameSim` fully decoupled:

1. Create `BotClient/BotClient.vcxproj` (executable).
2. Links: `NeuronCore` + `GameSim` + networking.
3. No `NeuronClient`, no `GameRender`, no window, no GPU.
4. Runs `Location::Advance()` in a loop, sends commands via network.
5. All entity constructors work because `Shape*` has moved to renderers.

---

## Handling `Shape*` in Constructors — The Asset Problem

Currently `ArmyAnt::ArmyAnt()` does:
```cpp
m_shapes[0] = g_app->m_resource->GetShape("armyant.shp");
```

The simulation does not need the shape.  The fix:

1. Move shape loading into the render companion init (or lazy-load on
   first `Render()` call).
2. The entity stores a type enum (it already does: `m_type = TypeArmyAnt`).
   The renderer uses the type to look up the correct shapes.
3. `GetCarryMarker()` (used in simulation for spirit attach position)
   references `m_carryMarker` from the shape.  Replace with a data-driven
   `LegacyVector3` offset loaded from a config file, not derived from a
   mesh marker at runtime.

---

## Handling `g_app` — The God Global

`g_app` reaches everything: renderer, camera, resource loader, location,
sound, particles.  A full split of `g_app` is out of scope for this plan,
but the boundary is:

- **Simulation code** may access `g_app->m_location`, `g_app->m_globalWorld`,
  `g_app->m_clientToServer`, and similar simulation-state members.
- **Simulation code must NOT access** `g_app->m_renderer`, `g_app->m_camera`,
  `g_app->m_resource` (shape/texture loading), `g_app->m_particleSystem`,
  `g_app->m_soundSystem`.  These calls move to render companions or the
  `SimEventQueue`.

A future phase can split `g_app` into `g_sim` (simulation globals) and
`g_client` (rendering/audio globals).

---

## File Change Summary (All Phases)

| Phase | File | Action |
|-------|------|--------|
| 1 | `GameLogic/GameLogicPlatform.h` | **New** — conditional include shim |
| 1 | `GameLogic/pch.h` | Replace `NeuronClient.h` with `GameLogicPlatform.h` |
| 1 | `GameLogic/GameLogic.vcxproj` | Add `GAMELOGIC_HAS_RENDERER` define |
| 2 | `GameRender/EntityRenderer.h` | **New** — base renderer interface |
| 2 | `GameRender/EntityRenderRegistry.h/.cpp` | **New** — type-indexed dispatch table |
| 2 | `GameRender/<Type>Renderer.h/.cpp` | **New** — one per entity/building type (~40 files) |
| 2 | `Starstrike/team.cpp` | Switch from `entity->Render()` to registry dispatch |
| 3 | `GameSim/SimEvents.h` | **New** — event types |
| 3 | `GameSim/SimEventQueue.h/.cpp` | **New** — deferred side-effect queue |
| 3 | All entity `.cpp` with `g_explosionManager`, `m_particleSystem`, etc. | Replace with event queue pushes |
| 4 | `GameLogic/worldobject.h` | Remove `virtual Render()` and `RenderPixelEffect()` |
| 4 | `GameLogic/entity.h` | Remove `virtual Render()`, `Shape* m_shape`, shadow statics |
| 4 | `GameLogic/building.h` | Remove `virtual Render()` / `RenderAlphas()`, `Shape* m_shape` |
| 5 | `GameSim/GameSim.vcxproj` | **New** — pure simulation static lib |
| 5 | `GameRender/GameRender.vcxproj` | **New** — render companion static lib |
| 5 | `Starstrike/Starstrike.vcxproj` | Update link deps |
| 5 | `NeuronServer/NeuronServer.vcxproj` | Add `GameSim` link |
| 5 | `GameLogic/GameLogic.vcxproj` | **Deleted** |
| 7 | `BotClient/BotClient.vcxproj` | **New** — headless executable |

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Huge scope — 67 files to touch | Incremental: one entity type per PR, game stays shippable at every step |
| `Shape*` used in simulation (e.g. `GetCarryMarker`) | Data-driven offsets replace mesh-marker lookups; audit all `ShapeMarker*` usage in Phase 0 |
| `g_app` god global reaches everywhere | Keep `g_app` for now; simulation code accesses simulation members only; split later |
| Render signature differences (`Darwinian::Render(float, float)`) | `EntityRenderContext` struct normalizes all signatures |
| Build time regression during transition | Both old and new render paths coexist temporarily; `#if` guards allow toggle |
| `entity_leg.cpp` / `entity.cpp` contain GL calls for shadows | Static shadow helpers (`BeginRenderShadow` etc.) move to a shared `GameRender/ShadowRenderer` utility |

---

## What Stays Coupled (Intentionally)

- **`NeuronCore`** — math, memory, file I/O, timers, logging, strings.
  Both simulation and rendering need these.
- **`LegacyVector3`, `Matrix34`** — math types used everywhere.  These are
  in `NeuronCore`, not rendering.
- **`WorldObjectId`** — pure data, no rendering dependency.

---

## Estimated Timeline

| Phase | Effort | Risk | Prerequisite |
|-------|--------|------|--------------|
| 0 — Audit | 1–2 days | None | — |
| 1 — PCH split | 1 day | Low | Phase 0 |
| 2 — Trivial entities (Wave 1) | 1 week | Low | Phase 1 |
| 2 — Medium entities (Wave 2) | 1–2 weeks | Medium | Wave 1 |
| 2 — Complex entities (Wave 3) | 1–2 weeks | Medium | Wave 2 |
| 2 — Buildings (Wave 4) | 1–2 weeks | Medium | Wave 1 |
| 2 — UI/debug (Wave 5) | 2–3 days | Low | Wave 1 |
| 3 — SimEventQueue | 1 week | Medium | Phase 2 in progress |
| 4 — Remove base-class Render | 1 day | Low | Phase 2 + 3 complete |
| 5 — Project split | 1–2 days | Low | Phase 4 |
| 6 — Presentation state separation | 1 week | Low | Phase 5 |
| 7 — Bot client | 1–2 days | Low | Phase 5 |

Phase 1 alone is enough to start server-side simulation experiments
immediately.  Phases 2–4 can run in parallel with other feature work.
