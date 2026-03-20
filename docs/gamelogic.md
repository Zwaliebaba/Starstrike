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

## Implementation Status (Updated)

This section tracks what has been completed, what is in progress, and what
remains.  Updated by codebase audit.

### Phase 0 — Preparation

| Task | Status | Notes |
|------|--------|-------|
| 0.1 — Audit every `Render` method | ✅ Done | Documented in tables below |
| 0.2 — Audit `ShapeMarker*` usage | ✅ Done | Documented in tables below |
| 0.3 — Audit side-effect calls | ✅ Done | Documented in tables below |
| 0.4 — Clean `Tree::RenderBranch()` → `CalcBranchRadius()` | ✅ Done | `tree.h` declares `CalcBranchRadius()`; `tree.cpp:327` has the pure-math implementation.  `GenerateBranch()` retained for mesh-filling (no GL). |

### Phase 1 — Break the PCH Dependency

| Task | Status | Notes |
|------|--------|-------|
| 1.1 — Create `GameLogicPlatform.h` | ✅ Done | Forward-declares `ShapeStatic`, `ShapeFragmentData`, `ShapeMarkerData`; `#if GAMELOGIC_HAS_RENDERER` includes `NeuronClient.h` |
| 1.2 — Change `GameLogic/pch.h` | ✅ Done | Now includes `GameLogicPlatform.h` only |
| 1.3 — `GAMELOGIC_HAS_RENDERER` define | ✅ Done | Present in both Debug and Release x64 configurations in `GameLogic.vcxproj` |

### Phase 2 — Extract Rendering into Companion Classes

#### Pre-Steps

| Task | Status | Notes |
|------|--------|-------|
| `ShadowRenderer` extraction | ✅ Done | `GameRender/ShadowRenderer.h/.cpp` — full GL implementation extracted from `Entity::BeginRenderShadow`/`RenderShadow`/`EndRenderShadow` |
| `EntityRenderer` base class | ✅ Done | `GameRender/EntityRenderer.h` — `Render(const Entity&, ctx)` + `RenderAlphas` |
| `BuildingRenderer` base class | ✅ Done | `GameRender/BuildingRenderer.h` — includes `RenderLights`/`RenderPorts` virtuals |
| `EntityRenderRegistry` | ✅ Done | `GameRender/EntityRenderRegistry.h/.cpp` — type-indexed table, `g_entityRenderRegistry` global |
| `BuildingRenderRegistry` | ✅ Done | `GameRender/BuildingRenderRegistry.h/.cpp` — type-indexed table, `g_buildingRenderRegistry` global |
| `IRenderBackend` generalized interface | ✅ Done | `GameLogic/render_backend_interface.h` — `ReleaseEntity(int)` / `ReleaseBuilding(int)` |
| `TreeBuildingRenderer` adapter | ✅ Done | `GameRender/TreeBuildingRenderer.h/.cpp` — `static_cast<const Tree&>` delegates to `TreeRenderer` |
| `Tree::Advance()` owns `Generate()` | ✅ Done | `tree.cpp:83-88` — `Generate()` + `m_meshDirty = false` + `++m_meshVersion` in `Advance()` |
| `TreeRenderer::DrawTree()` takes `const Tree&` | ✅ Done | `tree_renderer.cpp:176` — `const Tree&` signature |
| `InitGameRenderers()` call site | ✅ Done | `Starstrike/main.cpp:544` calls `InitGameRenderers()` at startup |
| `location.cpp` — registry dispatch for buildings | ✅ Done | `RenderBuildings()` (line 976) and `RenderBuildingAlphas()` (lines 1065, 1104) use `g_buildingRenderRegistry.Get()` with fallback |
| `team.cpp` — registry dispatch for entities | ✅ Done | `RenderVirii()` (line 428), `RenderDarwinians()` (line 494), `RenderOthers()` (lines 542, 569) use `g_entityRenderRegistry.Get()` with fallback |
| `unit.cpp` — registry dispatch for entities | ✅ Done | `Unit::Render()` (lines 142, 165) uses `g_entityRenderRegistry.Get()` with fallback |

> **Deviation from plan:** The plan called for companions to be placed in
> `NeuronClient/` during Phase 2 then moved to `GameRender/` in Phase 5.
> Instead, `GameRender/` was created early and all companions were placed
> there from the start.  This is strictly better — it avoids a future
> file-move step.  The plan's Phase 5.2 (`GameRender.vcxproj` creation) is
> effectively complete.

#### Wave 1 — Trivial Entities (just `Shape::Render`)

| Entity | Renderer | Registered | Old `Render()` Removed |
|--------|----------|------------|------------------------|
| `ArmyAnt` | ✅ `GameRender/ArmyAntRenderer` | ✅ `TypeArmyAnt` | ❌ `armyant.cpp:523` still has `ArmyAnt::Render()` |
| `Egg` | ✅ `GameRender/EggRenderer` | ✅ `TypeEgg` | ❌ `egg.cpp:32` still has `Egg::Render()` |
| `Officer` | ✅ `GameRender/OfficerRenderer` | ✅ `TypeOfficer` | ❌ `officer.cpp:96` still has `Officer::Render()` |
| `TriffidEgg` | ✅ `GameRender/TriffidEggRenderer` | ✅ `TypeTriffidEgg` | ❌ `triffid.cpp:636` still has `TriffidEgg::Render()` |

#### Wave 2 — Medium Entities (shape + GL state)

| Entity | Renderer | Registered | Old `Render()` Removed |
|--------|----------|------------|------------------------|
| `Engineer` | ✅ `GameRender/EngineerRenderer` | ✅ `TypeEngineer` | ❌ Still in `engineer.cpp` |
| `Armour` | ✅ `GameRender/ArmourRenderer` | ✅ `TypeArmour` | ❌ Still in `armour.cpp` |
| `Lander` | ✅ `GameRender/LanderRenderer` | ✅ `TypeLander` | ❌ Still in `lander.cpp` |
| `Tripod` | ✅ `GameRender/TripodRenderer` | ✅ `TypeTripod` | ❌ Still in `tripod.cpp:549` |
| `Centipede` | ✅ `GameRender/CentipedeRenderer` | ✅ `TypeCentipede` | ❌ Still in `centipede.cpp` |
| `Spider` | ✅ `GameRender/SpiderRenderer` | ✅ `TypeSpider` | ❌ Still in `spider.cpp:641` |

#### Wave 3 — Complex Entities (custom geometry / special signatures)

| Entity | Renderer | Registered | Old `Render()` Removed |
|--------|----------|------------|------------------------|
| `Darwinian` | ✅ `GameRender/DarwinianRenderer` | ✅ `TypeDarwinian` | ❌ `darwinian.cpp:1935` still has `Darwinian::Render()` |
| `Virii` | ✅ `GameRender/ViriiRenderer` | ✅ `TypeVirii` | ❌ `virii.cpp:708` still has `Virii::Render()` |
| `SoulDestroyer` | ✅ `GameRender/SoulDestroyerRenderer` | ✅ `TypeSoulDestroyer` | ❌ `souldestroyer.cpp:386` still has `SoulDestroyer::Render()` |
| `LaserTrooper` | ✅ `GameRender/LaserTrooperRenderer` | ✅ `TypeLaserTroop` | ❌ Still in `lasertrooper.cpp` |

#### Wave 4 — Buildings

| Building | Renderer | Registered | Old `Render()` Removed |
|----------|----------|------------|------------------------|
| `Tree` | ✅ `TreeBuildingRenderer` adapter | ✅ `TypeTree` | N/A (Tree never had a standard `Building::Render` override) |
| All other buildings (~20 types) | ❌ **Not started** | ❌ | ❌ |

Building types with `Render()`/`RenderAlphas()` overrides still in `GameLogic`:
`Building` (base), `Cave`, `Factory`, `SpawnPoint`, `MasterSpawnPoint`,
`SpawnPopulationLock`, `Teleport`, `RadarDish`, `LaserFence`, `GodDish`,
`Incubator`, `Mine`, `Triffid`, `SpiritReceiver`, `SpiritProcessor`,
`ReceiverBuilding`, `SpiritStore`, `Generator`/`PylonEnd`/`SolarPanel`,
`FeedingTube`, `Spam`, `Rocket`/`FuelBuilding`/`FuelGenerator`/`FuelStation`/`EscapeRocket`,
`StaticShape`, `TrunkPort`, `Wall`, `FenceSwitch`, `Snow`, `ScriptTrigger`,
`ResearchItem`, `SporeGenerator`, `Flag`.

#### Wave 5 — UI / Debug Windows

| Status | Notes |
|--------|-------|
| ❌ Not started | `cheat_window`, `darwinia_window`, `debugmenu`, `network_window`, `prefs_*_window`, `profilewindow`, `reallyquit_window`, `userprofile_window` still in `GameLogic/` |

### Phase 2 — Remaining Gaps (Legacy Code Not Yet Cleaned)

| Item | Status | Notes |
|------|--------|-------|
| `ITreeRenderBackend` still exists as separate interface | ⚠️ Not migrated | `GameLogic/tree_render_interface.h` still has `ITreeRenderBackend` + `g_treeRenderBackend` global; not yet consolidated into `IRenderBackend` |
| `Entity::BeginRenderShadow`/`RenderShadow`/`EndRenderShadow` still in `entity.h` | ⚠️ Legacy remains | Lines 103–105 of `entity.h` — static methods still declared; `ShadowRenderer` in `GameRender/` is the replacement but old methods not deleted |
| Old `Render()` bodies in entity `.cpp` files | ✅ Cleaned | All 14 migrated entity types now have empty stub `Render()` bodies; full GL code deleted. Declarations remain in `.h` for Phase 4. |
| `Shape* m_shape` and `Shape* m_shapes[]` still on entities | ⚠️ Still coupled | `entity.h:66` still has `ShapeStatic* m_shape`; entity constructors still load shapes via `g_app->m_resource->GetShape()` |
| `building.h` still has all render virtuals | ⚠️ Still coupled | `Render()`, `RenderAlphas()`, `RenderLights()`, `RenderPorts()` still declared (lines 118–121); `ShapeStatic* m_shape` on line 98 |
| `worldobject.h` still has `virtual void Render(float)` | ⚠️ Still coupled | Line 88 — base-class virtual remains |

### Phase 3 — SimEventQueue

| Status | Notes |
|--------|-------|
| ❌ Not started | No `SimEvent`, `SimEventQueue`, or event-related files exist. `Tree::Advance()` still calls `g_app->m_particleSystem->CreateParticle()` and `g_app->m_soundSystem->...` directly (tree.cpp:109, 116, etc.) |

### Phase 4 — Remove `Render()` from Base Classes

| Status | Notes |
|--------|-------|
| ❌ Not started | Blocked by: Wave 4 buildings not migrated, old `Render()` bodies not deleted |

### Phase 5 — Create `GameSim` and `GameRender` Projects

| Task | Status | Notes |
|------|--------|-------|
| 5.1 — `GameSim.vcxproj` | ❌ Not started | `GameLogic` still the sole simulation project |
| 5.2 — `GameRender.vcxproj` | ✅ Done (early) | Created with all companions from the start; static library, pch includes `NeuronClient.h`, includes `NeuronCore/NeuronClient/GameLogic/Starstrike` |
| 5.3 — Update link deps | ⚠️ Partial | `Starstrike` links `GameRender`; `NeuronServer` does not yet link `GameSim` (doesn't exist) |
| 5.4 — Delete `GameLogic/` | ❌ Not started | Still in solution |

### Phase 6 — Presentation State Separation

| Status | Notes |
|--------|-------|
| ⚠️ Partially achieved | New companions receive `const Entity&` / `const Building&` — correct layering. But `ArmyAntRenderer` reads `ant.m_shape->Render()` (the renderer reads `Shape*` from the simulation object, not from its own state). True const separation requires moving `Shape*` ownership to renderers. |

### Phase 7 — Headless Bot Client

| Status | Notes |
|--------|-------|
| ❌ Not started | Requires Phase 5 (`GameSim`) |

---

### Summary: What's Next

The highest-impact remaining work items, in recommended order:

1. ~~**Create `LanderRenderer` and `LaserTrooperRenderer`**~~ ✅ Done —
   Waves 2 and 3 entity renderers are now complete (15 of 15 entity types).
2. **Wave 4 — Building renderers** — ~20 building types still dispatch via
   the fallback `building->Render(predTime)` path.  Each is an independent
   PR following the established `BuildingRenderer` pattern.
3. ~~**Delete old `Render()` bodies**~~ ✅ Done — all 14 migrated entity
   `.cpp` files now have empty stubs; full GL code removed.
4. **Migrate `ITreeRenderBackend` → `IRenderBackend`** — consolidate the
   tree-specific cleanup global into the generalized interface.
5. **Phase 3 — `SimEventQueue`** — can start with `Tree::Advance()` as pilot
   while building migration proceeds in parallel.
6. **Phase 4 — Remove base-class `Render()`** — after all types are migrated.
7. **Phase 5.1 — `GameSim` project split** — after Phase 4.

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

## Prior Art: `TreeRenderer` as Reference Implementation

`Tree` is the first building type to have its rendering fully extracted.
The work already completed validates the core architecture of this plan and
establishes patterns that subsequent migrations should follow.

### What Was Done

| Aspect | Implementation |
|--------|---------------|
| Renderer class | `NeuronClient/tree_renderer.h/.cpp` — singleton `TreeRenderer` with dedicated DX12 PSO |
| Mesh generation | Moved to `TreeMeshData` / `GenerateBranch()` — CPU-side, no GL.  Mesh is uploaded lazily in `EnsureUploaded()`. |
| GPU cleanup on destruction | `ITreeRenderBackend` interface (`GameLogic/tree_render_interface.h`) + global `g_treeRenderBackend`.  `Tree::~Tree()` calls `g_treeRenderBackend->ReleaseTree(uniqueId)`. |
| Call-site dispatch | `Starstrike/location.cpp:1082` — manual `if (building->m_type == Building::TypeTree)` dispatch to `TreeRenderer::Get().DrawTree(...)` |
| Simulation file | `GameLogic/tree.cpp` — still includes `soundsystem.h`, `particle_system.h`, `GameApp.h` for side-effect calls in `Advance()` |

### Patterns to Adopt from TreeRenderer

1. **`ITreeRenderBackend` inversion-of-control for GPU cleanup.**
   `Tree` defines an abstract interface in `GameLogic` (no DX12 headers)
   and the renderer implements it in `NeuronClient`.  This should be
   generalized for any entity/building that owns GPU resources on the
   renderer side — see Phase 2 below.

2. **Mesh data as a pure-data struct in `GameLogic`.**
   `TreeMeshData` / `TreeVertex` (`GameLogic/tree_mesh_data.h`) are GPU-free
   data buffers that the simulation populates and the renderer consumes.
   This is the model for decoupling procedural geometry from rendering.

3. **Renderer lives in `NeuronClient/` during transition.**
   `TreeRenderer` was placed in `NeuronClient/`, not a separate `GameRender`
   project.  ~~During Phase 2, new render companions should follow this
   placement to minimize churn.  The `GameRender` project is created later
   in Phase 5 and all companions are moved then.~~
   **UPDATE:** `GameRender/` was created early; all new companions were
   placed there from the start.  `TreeRenderer` (the DX12 pipeline) remains
   in `NeuronClient/`; `TreeBuildingRenderer` (the adapter) lives in
   `GameRender/`.

### Gaps That Remain for Tree

| Gap | Status | Resolution |
|-----|--------|------------|
| `Tree::Advance()` calls `g_app->m_particleSystem->CreateParticle(...)` and `g_app->m_soundSystem->...` directly | ❌ Open | Migrate to `SimEventQueue` in Phase 3 (Tree is the pilot) |
| `Tree::RenderBranch()` contains dead GL code paths (`glTexCoord2f`, `glVertex3fv`) | ✅ Done | Renamed to `CalcBranchRadius()`; GL paths deleted |
| `Tree` inherits `Building::m_shape` (never used, always `nullptr`) | ❌ Open | Resolved when Phase 4 removes `m_shape` from `Building` base class |
| Call-site uses manual `if` type check instead of registry dispatch | ✅ Done | `BuildingRenderRegistry` in `location.cpp` with fallback |
| `TreeRenderer` has no `BuildingRenderer` base class — cannot be stored in registry | ✅ Done | `TreeBuildingRenderer : BuildingRenderer` adapter in `GameRender/` |
| `TreeRenderer::DrawTree()` takes concrete `Tree*`, not `const Building&` | ✅ Done | `DrawTree` takes `const Tree&`; adapter `static_cast`s from `const Building&` |
| `TreeRenderer::EnsureUploaded()` mutates `Tree` (`m_meshDirty = false`, calls `Generate()`) | ✅ Done | `Generate()` now in `Tree::Advance()`; renderer uses `m_meshVersion` for GPU-side staleness |
| `ITreeRenderBackend` is tree-specific; one per type doesn't scale | ⚠️ Partial | `IRenderBackend` generalized interface created (`render_backend_interface.h`); `ITreeRenderBackend` not yet migrated to it |

### Architectural Assessment: Why TreeRenderer Should Not Be Replicated Verbatim

`TreeRenderer` proved that rendering can live outside `GameLogic` and that
an inversion-of-control interface (`ITreeRenderBackend`) can handle GPU
cleanup.  However, the current implementation has four structural problems
that prevent it from being the template for 40+ subsequent migrations.

#### Problem 1 — No shared base class prevents uniform dispatch

`TreeRenderer` inherits from `ITreeRenderBackend` (for GPU cleanup) but
**not** from any `BuildingRenderer` base.  This means there is no common
`Render(const Building&, ctx)` signature, no way to store `TreeRenderer`
alongside other building renderers in a registry, and each renderer would
invent its own calling convention.  If replicated for 20 building types,
`location.cpp` would accumulate 20 `if/else if` branches — the same
maintenance problem as the current virtual dispatch but without type safety.

#### Problem 2 — Concrete `Tree*` coupling

```cpp
void TreeRenderer::DrawTree(Tree* _tree, float _predictionTime,
                            unsigned int _textureGLId, bool _skipLeaves);
```

`DrawTree` takes a raw `Tree*` and reads ~10 fields directly (`m_front`,
`m_pos`, `m_branchMesh`, `m_leafMesh`, `m_branchColourArray`,
`m_leafColourArray`, `m_meshDirty`, `m_id`, `GetActualHeight()`).  The call
site must know the concrete type, which defeats the purpose of registry
dispatch.  The fix is a thin adapter that receives `const Building&` and
performs an internal `static_cast<const Tree&>` — safe because the registry
guarantees the type → renderer mapping.

#### Problem 3 — The renderer mutates simulation state

```cpp
// TreeRenderer::EnsureUploaded()
if (_tree->m_meshDirty)
{
    _tree->Generate();         // mutates Tree's mesh data
    _tree->m_meshDirty = false; // writes to sim state
}
```

This is a layering violation.  It works today because client and renderer
run single-threaded, but it blocks:

- **Phase 6's `const` access goal** — renderers must receive `const Building&`
- **Future double-buffered state** — the renderer cannot mutate the sim buffer
  if simulation runs ahead of rendering
- **Server parity** — the server would need to call `Generate()` independently
  if mesh generation affects simulation-visible state (it does —
  `CalcBranchRadius` uses the generated geometry)

**Fix:** `Tree::Advance()` calls `Generate()` when `m_meshDirty` is true.
By the time the renderer sees the tree, the mesh is always current.  The
renderer becomes a pure consumer.

#### Problem 4 — `ITreeRenderBackend` is tree-specific

If every building type gets its own `I<Type>RenderBackend` plus its own
global pointer, the codebase would accumulate 20+ one-method interfaces and
20+ globals.  The generalized `IRenderBackend` with
`ReleaseBuilding(int)` / `ReleaseEntity(int)` (Phase 2 pre-step) is the
scalable replacement.

#### Conclusion

The existing `TreeRenderer` is a valid proof-of-concept, not a template.
The standardized pattern is:

1. **Abstract base class** (`BuildingRenderer` / `EntityRenderer`) for
   uniform dispatch via registry.
2. **Thin adapter** (`TreeBuildingRenderer : BuildingRenderer`) wraps the
   existing `TreeRenderer` DX12 pipeline — no rewrite required.
3. **Simulation owns mesh generation** — `Generate()` runs in `Advance()`,
   not in the renderer.
4. **Single `IRenderBackend`** for GPU cleanup, not per-type interfaces.

`TreeRenderer` itself keeps its DX12 PSO and per-tree GPU buffer management.
Only the calling convention and ownership boundary change.

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

Search every `.cpp` in `GameLogic/` for `GetMarkerData`,
`GetMarkerWorldMatrix`, `m_carryMarker`, `m_lights`, `m_ports`.  These are
the `ShapeMarkerData*` usages that block removing `Shape*` from simulation.

| Entity/Building | Marker Usage | Simulation? | Replacement |
|-----------------|-------------|-------------|-------------|
| `ArmyAnt` | `m_carryMarker` → spirit attach position via `GetCarryMarker()` | **Yes** — positions carried spirit in `Advance()` | Data-driven `LegacyVector3` offset (fixed relative to entity frame) |
| `Building` | `m_lights` → ownership light positions | **Rendering only** — used in `RenderLights()` | Moves to `BuildingRenderer` companion |
| `Building` | `m_ports` → Darwinian port positions | **Both** — `EvaluatePorts()` uses port positions for simulation (pathfinding); `RenderPorts()` uses them for display | Port positions need a data-driven replacement for simulation; visual rendering moves to companion |
| Other entities | TBD — full audit required | TBD | TBD |

### 0.3 — Audit side-effect calls in simulation

Calls from `Advance()` / `ChangeHealth()` / etc. into rendering systems:

| Pattern | Examples | Replacement |
|---------|----------|-------------|
| `g_explosionManager.AddExplosion(shape, transform)` | `ArmyAnt::ChangeHealth` | Event queue |
| `g_app->m_particleSystem->CreateParticle(...)` | `ArmyAnt::AdvanceAttackEnemy`, `Tree::Advance` (fire, debris, leaves) | Event queue |
| `g_app->m_soundSystem->TriggerEntityEvent(...)` | `ArmyAnt::AdvanceAttackEnemy`, `Tree::Damage`, `Tree::Advance` (burn sounds) | Event queue |
| `g_app->m_renderer->SetObjectLighting()` | Various `Render()` bodies | Moves to renderer companion |

### 0.4 — Clean up `Tree::RenderBranch()` dead GL paths

`Tree::RenderBranch()` (`tree.cpp:311–393`) contains `glTexCoord2f` /
`glVertex3fv` calls inside conditional branches (`_renderBranch`,
`_renderLeaf`).  These paths are **dead code** — all actual rendering now
goes through `GenerateBranch()` → `TreeMeshData` → `TreeRenderer`.
`RenderBranch` is only called with `_calcRadius = true, _renderBranch = false,
_renderLeaf = false` for the hit-check radius pass.

**Action:** Rename to `CalcBranchRadius()` keeping only the `_calcRadius`
math path.  Delete the GL-emitting branches entirely.  This is a safe
refactor that removes GL calls from one more simulation file with zero
behavioral change.

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

### Renderer Placement Strategy

> **STATUS: ✅ Implemented as described.**

New render companions are placed in **`GameRender/`** from the start.
`GameRender` is a static library that depends on `NeuronClient` (for the GL
translation layer, `ShapeStatic`, `TreeRenderer`, etc.) and `GameLogic`
(for entity/building types).  `Starstrike` links `GameRender` and calls
`InitGameRenderers()` at startup to register all companions.

> `TreeRenderer` (the DX12 pipeline) remains in `NeuronClient/` because it
> is low-level GPU infrastructure.  `TreeBuildingRenderer` (the thin adapter
> that wraps it behind `BuildingRenderer`) lives in `GameRender/`.

### Pre-Step: Extract `ShadowRenderer`

> **STATUS: ✅ Done.** `GameRender/ShadowRenderer.h/.cpp` — placed directly
> in `GameRender/` (not `NeuronClient/` as originally planned).

`Entity::BeginRenderShadow()` / `RenderShadow()` / `EndRenderShadow()`
(`entity.cpp:496+`) are static methods containing GL calls used by many
entity `Render()` overrides across Waves 1–3.  Extract these into
`GameRender/ShadowRenderer.h/.cpp` **before** Wave 1 so that all entity
render companions can call `ShadowRenderer::Begin()` / `Render()` /
`End()` instead of `Entity::BeginRenderShadow()`.

### Pre-Step: Introduce `BuildingRenderRegistry` (with `Tree` as first entry)

> **STATUS: ✅ Done.** `GameRender/BuildingRenderRegistry.h/.cpp` +
> `location.cpp` uses registry dispatch with fallback for all building
> render passes.

`Starstrike/location.cpp:1082` already does manual type dispatch for Tree:

```cpp
if (building->m_type == Building::TypeTree)
    TreeRenderer::Get().DrawTree(tree, predTime, s_treeTextureId, skipLeaves);
else
    building->RenderAlphas(predTime);
```

Introduce `BuildingRenderRegistry` immediately with `TreeRenderer`
registered as the first entry and a fallback `else` path for unmigrated
types:

```cpp
BuildingRenderer* renderer = g_buildingRenderRegistry.Get(building->m_type);
if (renderer)
    renderer->RenderAlphas(*building, ctx);
else
    building->RenderAlphas(predTime); // fallback during transition
```

Same pattern for `Location::RenderBuildings()` (opaque pass at line 960).
This prevents the dispatch site from accumulating per-type `if` branches as
more buildings are migrated.

### Pre-Step: Adapt `TreeRenderer` to `BuildingRenderer` Interface

> **STATUS: ✅ Done.** All four sub-tasks completed: `Generate()` in
> `Tree::Advance()`, `TreeBuildingRenderer` adapter in `GameRender/`,
> registered in `BuildingRenderRegistry`, `DrawTree` takes `const Tree&`.

`TreeRenderer` predates the `BuildingRenderer` base class and uses a
concrete `Tree*` interface.  Before `BuildingRenderRegistry` can include
Tree, two changes are needed:

1. **Move `Generate()` out of the renderer.**  Currently
   `TreeRenderer::EnsureUploaded()` calls `_tree->Generate()` and clears
   `m_meshDirty`.  Move this to `Tree::Advance()` so the renderer is a
   pure consumer and can receive `const Tree&`:

   ```cpp
   // GameLogic/tree.cpp — Tree::Advance()
   bool Tree::Advance()
   {
       // ...existing code...
       if (m_meshDirty)
       {
           Generate();
           m_meshDirty = false;
       }
       // ...existing code...
   }
   ```

   `TreeRenderer::EnsureUploaded()` then only checks the GPU-side dirty
   flag (mesh data changed since last upload) and uploads — no mutation of
   `Tree`.

2. **Create a thin adapter:**

   ```cpp
   // NeuronClient/TreeBuildingRenderer.h
   #pragma once
   #include "BuildingRenderer.h"

   class TreeBuildingRenderer : public BuildingRenderer
   {
   public:
       void Render(const Building& _building,
                   const BuildingRenderContext& _ctx) override;
       void RenderAlphas(const Building& _building,
                         const BuildingRenderContext& _ctx) override;
   };
   ```

   The adapter `static_cast`s to `const Tree&` internally and delegates to
   `TreeRenderer::Get().DrawTree(...)`.  This cast is safe because the
   registry guarantees the type → renderer mapping.  `TreeRenderer` itself
   keeps its DX12 PSO and per-tree GPU buffer management unchanged.

3. **Register in `BuildingRenderRegistry` at init:**

   ```cpp
   static TreeBuildingRenderer s_treeBuildingRenderer;
   g_buildingRenderRegistry.Register(Building::TypeTree, &s_treeBuildingRenderer);
   ```

4. **Remove the manual `if (TypeTree)` branch in `location.cpp`.**  The
   registry fallback handles it automatically.

### Pre-Step: Generalize `ITreeRenderBackend` as a Destruction-Cleanup Pattern

> **STATUS: ⚠️ Partial.** `IRenderBackend` interface created
> (`render_backend_interface.h`).  `ITreeRenderBackend` still exists
> separately in `tree_render_interface.h` and has not been migrated.

`Tree::~Tree()` calls `g_treeRenderBackend->ReleaseTree(uniqueId)` to
notify the renderer when a simulation object is destroyed.  Any future
entity/building type whose render companion owns per-instance GPU resources
will need the same pattern.

Generalize into a lightweight interface:

```cpp
// GameLogic/render_backend_interface.h
#pragma once

// Abstract interface for GPU resource cleanup on simulation object
// destruction.  Defined in GameLogic (no DX12 headers), implemented
// in NeuronClient.
class IRenderBackend
{
  public:
    virtual ~IRenderBackend() = default;
    virtual void ReleaseEntity(int _uniqueId) {}
    virtual void ReleaseBuilding(int _uniqueId) {}
};

extern IRenderBackend* g_renderBackend;
```

`ITreeRenderBackend` can be kept as-is for now or migrated to this when
`TreeRenderer` is wrapped into the `BuildingRenderRegistry`.  New companions
use `g_renderBackend->ReleaseEntity(uniqueId)` in their destructors.

### Core Pattern: Render Companion

For each game object type, split the class into a simulation half and a
render half.

**Before** (single class):
```
GameLogic/armyant.h    → class ArmyAnt : Entity { Advance(); Render(); }
GameLogic/armyant.cpp  → #includes GL headers, implements both
```

**After** (two classes, actual placement):
```
GameLogic/armyant.h              → class ArmyAnt : Entity { Advance(); }
GameLogic/armyant.cpp            → pure simulation, no GL

GameRender/ArmyAntRenderer.h    → class ArmyAntRenderer : EntityRenderer { Render(); }
GameRender/ArmyAntRenderer.cpp  → GL calls, Shape::Render, lighting
```

### `EntityRenderer` base class

> **STATUS: ✅ Done.** `GameRender/EntityRenderer.h`

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

### `BuildingRenderer` base class

> **STATUS: ✅ Done.** `GameRender/BuildingRenderer.h`

```cpp
// GameRender/BuildingRenderer.h
#pragma once

struct BuildingRenderContext
{
    float predictionTime;
};

class Building;

class BuildingRenderer
{
public:
    virtual ~BuildingRenderer() = default;
    virtual void Render(const Building& _building, const BuildingRenderContext& _ctx) = 0;
    virtual void RenderAlphas(const Building& _building, const BuildingRenderContext& _ctx) {}
    virtual void RenderLights(const Building& _building) {}
    virtual void RenderPorts(const Building& _building) {}
};
```

`RenderLights()` and `RenderPorts()` are included here because they are
currently on `Building` but contain only GL calls (team-colour starbursts,
port control-pad rendering).  They always execute in the rendering pass and
never affect simulation state.

### `EntityRenderRegistry`

> **STATUS: ✅ Done.** `GameRender/EntityRenderRegistry.h/.cpp`

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

Same pattern for `BuildingRenderRegistry` with `Building::NumBuildingTypes`
slots.

### Call-site change (`Team::Render` family)

> **STATUS: ✅ Done.** `team.cpp` (`RenderVirii`, `RenderDarwinians`,
> `RenderOthers`), `unit.cpp` (`Unit::Render`), and `location.cpp`
> (`RenderBuildings`, `RenderBuildingAlphas`) all use registry dispatch
> with fallback to legacy `entity->Render()` / `building->Render()`.

```diff
- entity->Render(_predictionTime);
+ EntityRenderer* renderer = g_entityRenderRegistry.Get(entity->m_type);
+ if (renderer)
+     renderer->Render(*entity, ctx);
```

Same pattern for `BuildingRenderer` / `BuildingRenderRegistry`.

**Darwinian special case:** `Team::RenderDarwinians()` still has its own
loop (computes `highDetail` from camera distance), but now routes through
`EntityRenderRegistry` with `EntityRenderContext.highDetailFactor`.  The
fallback path uses `static_cast<Darwinian*>(entity)->Render()`.  Unifying
into `RenderOthers()` is optional cleanup — not blocking.

### Migration Order

Each entity type is a self-contained PR:

**Wave 1 — Trivial** (just `Shape::Render`): ✅ All companions created
- `ArmyAnt`, `Egg`, `Officer`, `TriffidEgg`

**Wave 2 — Medium** (shape + GL state): ✅ All companions created
- `Engineer`, `Armour`, `Lander`, `Tripod`, `Centipede`, `Spider`

**Wave 3 — Complex** (custom geometry / special signatures): ✅ All companions created
- `Darwinian` (LOD render), `Virii` (particle rendering),
  `SoulDestroyer` (multi-segment), `LaserTrooper` (billboard quads)

**Wave 4 — Buildings**: ⚠️ Only Tree done
- ✅ `Tree` — `TreeBuildingRenderer` adapter
- ❌ Remaining ~20 building types not started

**Wave 5 — UI / debug windows**: ❌ Not started
- Already client-only; move to render companions without logic changes

### Per-Entity Migration Checklist

For each entity type:

- [ ] Create `GameRender/<Type>Renderer.h/.cpp`
- [ ] Move `Render()` body (and `RenderAlphas()` if present) to the renderer
- [ ] Move `Shape*` fields and asset loading to the renderer
- [ ] Replace `ShapeMarker*` simulation usage with data-driven offsets
- [ ] If the type owns per-instance GPU resources, implement cleanup via
      `g_renderBackend->ReleaseEntity(uniqueId)` (following the
      `ITreeRenderBackend` pattern)
- [ ] Register the renderer in `EntityRenderRegistry` init
- [ ] Remove GL includes from the simulation `.cpp`
- [ ] Delete the old `Render()` body from the simulation `.cpp`
- [ ] Verify `Advance()` has no remaining rendering dependencies
- [ ] Build and test in all configurations

### Per-Building Migration Checklist

For each building type:

- [ ] Create `GameRender/<Type>BuildingRenderer.h/.cpp`
- [ ] Move `Render()`, `RenderAlphas()`, `RenderLights()`, and
      `RenderPorts()` bodies to the renderer
- [ ] Move `Shape*` fields and asset loading to the renderer
- [ ] Replace `ShapeMarkerData*` simulation usage with data-driven
      positions (audit `EvaluatePorts()` for simulation-visible usages)
- [ ] If the type owns per-instance GPU resources, implement cleanup via
      `g_renderBackend->ReleaseBuilding(uniqueId)`
- [ ] Register the renderer in `BuildingRenderRegistry` init
- [ ] Remove GL includes from the simulation `.cpp`
- [ ] Build and test in all configurations

---

## Phase 3 — Introduce `SimEventQueue` for Side Effects

> **STATUS: ❌ Not started.**  No `SimEvent` / `SimEventQueue` files exist.

Simulation code currently calls rendering/audio systems directly during
`Advance()` and `ChangeHealth()`.  Replace these with a deferred event
queue that the client drains after simulation and the server discards.

### Pilot: `Tree::Advance()`

`Tree::Advance()` is the ideal first candidate — it is self-contained and
exercises all three event types:

| Call site | Current code | Event type |
|-----------|-------------|------------|
| `tree.cpp:97` | `g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeFire, fireSize)` | `ParticleSpawn` |
| `tree.cpp:104` | `g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeExplosionDebris)` | `ParticleSpawn` |
| `tree.cpp:140` | `g_app->m_soundSystem->StopAllSounds(m_id, "Tree Burn")` | `SoundTrigger` |
| `tree.cpp:141` | `g_app->m_soundSystem->TriggerBuildingEvent(this, "Create")` | `SoundTrigger` |
| `tree.cpp:161` | `g_app->m_particleSystem->CreateParticle(fireSpawn, g_zeroVector, Particle::TypeLeaf, ...)` | `ParticleSpawn` |
| `tree.cpp:260–261` (`Damage`) | `g_app->m_soundSystem->StopAllSounds(...)` / `TriggerBuildingEvent(...)` | `SoundTrigger` |

Migrate Tree first, verify the pattern works end-to-end, then apply to the
remaining entity/building types.

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

> **STATUS: ❌ Not started.**  Blocked by: Wave 2 missing `Lander`, Wave 3
> missing `LaserTrooper`, Wave 4 buildings (~20 types) not migrated, and old
> `Render()` bodies still present in all 12 migrated entity `.cpp` files.

Once **all** entity and building types have render companions registered:

1. Remove `virtual void Render(float)` from `WorldObject`.
2. Remove `virtual void Render(float)` from `Entity`.
3. Remove `virtual void Render(float)` / `RenderAlphas(float)` /
   `RenderLights()` / `RenderPorts()` from `Building`.
4. Remove `Shape* m_shape` from `Entity` and `Building`.
5. Remove `Shape* m_shapes[]` from entity subclasses (e.g. `ArmyAnt`).
6. Remove `ShapeMarkerData* m_carryMarker` and similar from entity
   subclasses (replaced by data-driven offsets in Phase 2).
7. Remove static shadow helpers (`BeginRenderShadow` etc.) from `Entity`
   (replaced by `ShadowRenderer` in Phase 2 pre-step).
8. Remove `#include "renderer.h"` / `#include "shape.h"` from all
   simulation files.

---

## Phase 5 — Create `GameSim` and `GameRender` Projects

> **STATUS: ⚠️ Partial.**  5.2 (`GameRender.vcxproj`) completed early.
> 5.1 (`GameSim`), 5.3 (link deps), and 5.4 (delete `GameLogic`) not
> started — blocked by Phases 2–4.

### 5.1 — `GameSim/GameSim.vcxproj` (static library)

> **STATUS: ❌ Not started.**  `GameLogic` is still the sole simulation
> project.

- Move all simulation `.cpp/.h` from `GameLogic/` into `GameSim/`.
- `pch.h` includes `NeuronCore.h` only.
- Additional include directories: `$(SolutionDir)NeuronCore`.
- **No** `NeuronClient` dependency.
- **No** `GAMELOGIC_HAS_RENDERER` define.

### 5.2 — `GameRender/GameRender.vcxproj` (static library)

> **STATUS: ✅ Done (created early).**  All companions placed in
> `GameRender/` from the start.  `pch.h` includes `NeuronClient.h`;
> additional include dirs cover `NeuronCore`, `NeuronClient`, `GameLogic`,
> `Starstrike`.  `TreeRenderer` (the DX12 pipeline) remains in
> `NeuronClient/`.

- ~~Move all `*Renderer.cpp/.h` files from `NeuronClient/` into `GameRender/`.~~
  Companions were placed directly in `GameRender/` — no move needed.
- Contains `EntityRenderRegistry`, `BuildingRenderRegistry`.
- Contains `SimEventQueue` drain/dispatch logic (future — not yet created).
- `pch.h` includes `NeuronClient.h`.
- Additional include directories: `$(SolutionDir)NeuronCore`,
  `$(SolutionDir)NeuronClient`, `$(SolutionDir)GameLogic`,
  `$(SolutionDir)Starstrike`.

### 5.3 — Update link dependencies

> **STATUS: ⚠️ Partial.**  `Starstrike` links `GameRender`.  `NeuronServer`
> does not yet link `GameSim` (project doesn't exist).

| Project | Links |
|---------|-------|
| `Starstrike` | `NeuronClient`, `NeuronCore`, `GameSim`, `GameRender` |
| `NeuronServer` | `NeuronCore`, `GameSim` |
| `BotClient` (future) | `NeuronCore`, `GameSim` |

### 5.4 — Delete `GameLogic/` project

> **STATUS: ❌ Not started.**

Once all files are migrated, remove `GameLogic.vcxproj` from the solution.

---

## Phase 6 — Separate Simulation State from Presentation State

> **STATUS: ⚠️ Partially achieved.**  New companions receive `const Entity&`
> / `const Building&` — correct layering.  However, renderers still read
> `Shape*` from simulation objects (e.g. `ant.m_shape->Render()`), so true
> const separation requires moving `Shape*` ownership to renderers (Phase 4
> prerequisite).

### 6.1 — Const access for renderers

Render companions receive `const Entity&` / `const Building&`.  Renderers
never write to simulation state.

**Prerequisite (completed in Phase 2 pre-step):** `TreeRenderer` is the
only existing renderer that mutates simulation state
(`EnsureUploaded()` writes `m_meshDirty` and calls `Generate()`).  Phase 2
pre-step moves `Generate()` into `Tree::Advance()`, eliminating the
mutation before any new companions are written.  All subsequent companions
are written against `const` references from the start.

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

> **STATUS: ❌ Not started.**  Requires Phase 5 (`GameSim` project).

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

| Phase | File | Action | Status |
|-------|------|--------|--------|
| 0 | `GameLogic/tree.cpp` | Rename `RenderBranch()` → `CalcBranchRadius()`; delete dead GL paths | ✅ |
| 0 | `GameLogic/tree.h` | Update method declaration | ✅ |
| 1 | `GameLogic/GameLogicPlatform.h` | **New** — conditional include shim | ✅ |
| 1 | `GameLogic/pch.h` | Replace `NeuronClient.h` with `GameLogicPlatform.h` | ✅ |
| 1 | `GameLogic/GameLogic.vcxproj` | Add `GAMELOGIC_HAS_RENDERER` define | ✅ |
| 2 (pre) | `GameRender/ShadowRenderer.h/.cpp` | **New** — extracted from `Entity::BeginRenderShadow` etc. | ✅ |
| 2 (pre) | `GameRender/BuildingRenderRegistry.h/.cpp` | **New** — type-indexed dispatch; Tree registered first | ✅ |
| 2 (pre) | `GameRender/EntityRenderRegistry.h/.cpp` | **New** — type-indexed dispatch table | ✅ |
| 2 (pre) | `GameRender/EntityRenderer.h` | **New** — base renderer interface | ✅ |
| 2 (pre) | `GameRender/BuildingRenderer.h` | **New** — base building renderer interface (incl. `RenderLights`/`RenderPorts`) | ✅ |
| 2 (pre) | `GameLogic/render_backend_interface.h` | **New** — generalized `IRenderBackend` for GPU cleanup on destruction | ✅ |
| 2 (pre) | `GameRender/TreeBuildingRenderer.h/.cpp` | **New** — thin adapter: `TreeBuildingRenderer : BuildingRenderer`, delegates to `TreeRenderer` | ✅ |
| 2 (pre) | `GameLogic/tree.cpp` | Move `Generate()` + `m_meshDirty = false` from `TreeRenderer::EnsureUploaded()` into `Tree::Advance()` | ✅ |
| 2 (pre) | `NeuronClient/tree_renderer.cpp` | Remove `_tree->Generate()` / `m_meshDirty = false` from `EnsureUploaded()`; change `DrawTree` to take `const Tree&` | ✅ |
| 2 (pre) | `Starstrike/location.cpp` | Replace `if (TypeTree)` dispatch with `BuildingRenderRegistry` + fallback | ✅ |
| 2 | `GameRender/<Type>Renderer.h/.cpp` | **New** — one per entity/building type (~40 files) | ⚠️ 15 of ~40 (all entities done; buildings remain) |
| 2 | `Starstrike/team.cpp` | Switch from `entity->Render()` to registry dispatch | ✅ |
| 2 | `Starstrike/unit.cpp` | Switch from `entity->Render()` to registry dispatch | ✅ |
| 2 | `Starstrike/location.cpp` | Switch from `building->Render()` to registry dispatch | ✅ |
| 2 | `GameLogic/*.cpp` (migrated entities) | Delete old `Render()` bodies | ✅ All 14 entity types stubbed |
| 3 | `GameLogic/SimEvents.h` | **New** — event types (moves to `GameSim/` in Phase 5) | ❌ |
| 3 | `GameLogic/SimEventQueue.h/.cpp` | **New** — deferred side-effect queue (moves to `GameSim/` in Phase 5) | ❌ |
| 3 | `GameLogic/tree.cpp` (pilot) | Replace particle/sound calls with event queue pushes | ❌ |
| 3 | All entity `.cpp` with `g_explosionManager`, `m_particleSystem`, etc. | Replace with event queue pushes | ❌ |
| 4 | `GameLogic/worldobject.h` | Remove `virtual Render()` | ❌ |
| 4 | `GameLogic/entity.h` | Remove `virtual Render()`, `Shape* m_shape`, shadow statics | ❌ |
| 4 | `GameLogic/entity.cpp` | Remove `BeginRenderShadow` / `RenderShadow` / `EndRenderShadow` | ❌ |
| 4 | `GameLogic/building.h` | Remove `virtual Render()` / `RenderAlphas()` / `RenderLights()` / `RenderPorts()`, `Shape* m_shape` | ❌ |
| 5 | `GameSim/GameSim.vcxproj` | **New** — pure simulation static lib | ❌ |
| 5 | `GameRender/GameRender.vcxproj` | **New** — render companion static lib | ✅ (early) |
| 5 | `Starstrike/Starstrike.vcxproj` | Update link deps | ⚠️ Partial |
| 5 | `NeuronServer/NeuronServer.vcxproj` | Add `GameSim` link | ❌ |
| 5 | `GameLogic/GameLogic.vcxproj` | **Deleted** | ❌ |
| 7 | `BotClient/BotClient.vcxproj` | **New** — headless executable | ❌ |

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Huge scope — 67 files to touch | Incremental: one entity type per PR, game stays shippable at every step |
| `Shape*` used in simulation (e.g. `GetCarryMarker`) | Data-driven offsets replace mesh-marker lookups; audit all `ShapeMarkerData*` usage in Phase 0.2 |
| `g_app` god global reaches everywhere | Keep `g_app` for now; simulation code accesses simulation members only; split later |
| Render signature differences (`Darwinian::Render(float, float)`) | `EntityRenderContext` struct normalizes all signatures; Darwinian dispatch unified into `EntityRenderRegistry` |
| Build time regression during transition | Both old and new render paths coexist temporarily; `#if` guards allow toggle |
| `entity_leg.cpp` / `entity.cpp` contain GL calls for shadows | Static shadow helpers (`BeginRenderShadow` etc.) extracted to `ShadowRenderer` in Phase 2 pre-step |
| GPU resource leaks when simulation objects are destroyed | `IRenderBackend` destruction-cleanup pattern (proven by `ITreeRenderBackend`) generalized for all companions |
| `Building::RenderLights()` / `RenderPorts()` missed during migration | Explicitly included in `BuildingRenderer` base class and per-building checklist |
| `location.cpp` accumulates per-type `if` branches | `BuildingRenderRegistry` introduced early with fallback `else` for unmigrated types |
| `Building::m_ports` used in both simulation (`EvaluatePorts`) and rendering (`RenderPorts`) | Phase 0.2 audit identifies simulation-visible port positions; data-driven replacement required before `Shape*` removal |
| `TreeRenderer::EnsureUploaded()` mutates `Tree` (writes `m_meshDirty`, calls `Generate()`) | Move `Generate()` to `Tree::Advance()` in Phase 2 pre-step; renderer becomes pure consumer before any new companions are written |
| `TreeRenderer` has no `BuildingRenderer` base, concrete `Tree*` coupling | `TreeBuildingRenderer` thin adapter wraps existing DX12 pipeline; no rewrite needed |
| Replicating TreeRenderer verbatim for 40 types creates 40 `if/else if` branches and 40 per-type cleanup interfaces | Standardized `BuildingRenderer`/`EntityRenderer` base + registry dispatch; single `IRenderBackend` for cleanup |

---

## What Stays Coupled (Intentionally)

- **`NeuronCore`** — math, memory, file I/O, timers, logging, strings.
  Both simulation and rendering need these.
- **`LegacyVector3`, `Matrix34`** — math types used everywhere.  These are
  in `NeuronCore`, not rendering.
- **`WorldObjectId`** — pure data, no rendering dependency.
- **`TreeMeshData`, `TreeVertex`** — pure CPU-side data buffers used by
  `Tree` (simulation) and consumed by `TreeRenderer` (rendering).  These
  are GPU-free and belong in `GameSim`.

---

## Estimated Timeline

| Phase | Effort | Risk | Prerequisite | Status |
|-------|--------|------|--------------|--------|
| 0 — Audit + `RenderBranch` cleanup | 1–2 days | None | — | ✅ Done |
| 1 — PCH split | 1 day | Low | Phase 0 | ✅ Done |
| 2 pre — `ShadowRenderer` + registries + `IRenderBackend` | 2–3 days | Low | Phase 1 | ✅ Done |
| 2 — Trivial entities (Wave 1) | 1 week | Low | Phase 2 pre | ✅ Done |
| 2 — Medium entities (Wave 2) | 1–2 weeks | Medium | Wave 1 | ✅ Done |
| 2 — Complex entities (Wave 3, incl. Darwinian unification) | 1–2 weeks | Medium | Wave 2 | ✅ Done |
| 2 — Buildings (Wave 4, Tree already done) | 1–2 weeks | Medium | Wave 1 | ⚠️ 1/~21 (Tree only) |
| 2 — UI/debug (Wave 5) | 2–3 days | Low | Wave 1 | ❌ Not started |
| 3 — SimEventQueue (Tree pilot first, then remaining types) | 1 week | Medium | Phase 2 in progress | ❌ Not started |
| 4 — Remove base-class Render | 1 day | Low | Phase 2 + 3 complete | ❌ Blocked |
| 5 — Project split (create GameSim + GameRender, move files) | 1–2 days | Low | Phase 4 | ⚠️ 5.2 done early |
| 6 — Presentation state separation | 1 week | Low | Phase 5 | ⚠️ Partial |
| 7 — Bot client | 1–2 days | Low | Phase 5 | ❌ Not started |

Phase 1 alone is enough to start server-side simulation experiments
immediately.  Phases 2–4 can run in parallel with other feature work.

### Recommended Execution Order (Updated)

1. ~~**Phase 0**: Audit + clean `RenderBranch` (0.4) + expand marker audit (0.2)~~ ✅
2. ~~**Phase 1**: PCH split~~ ✅
3. ~~**Phase 2 pre**: `ShadowRenderer`, `BuildingRenderRegistry` (Tree as
   first entry), `EntityRenderRegistry`, `IRenderBackend`~~ ✅
4. ~~**Phase 2 Wave 1**: Trivial entity companions (in `GameRender/`)~~ ✅
5. ~~**Create `LanderRenderer` and `LaserTrooperRenderer`** — complete Waves 2/3~~ ✅
6. **Wave 4 — Building renderers** — ~20 building types, independent PRs
7. ~~**Delete old `Render()` bodies** from migrated entity `.cpp` files~~ ✅
8. **Migrate `ITreeRenderBackend` → `IRenderBackend`** — consolidate globals
9. **Phase 3 pilot**: `Tree::Advance()` side-effects → `SimEventQueue`
10. **Phase 2 Wave 5** (UI/debug) and remaining **Phase 3** in parallel
11. **Phase 4–7** sequentially once Phase 2 + 3 are complete
