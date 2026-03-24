# Decouple Simulation from Rendering in `GameLogic`

## Goal

Clean the `GameLogic` static library into a **pure-simulation** library
that compiles without any GPU, rendering, or windowing headers, and
maintain the separate **render-companion** library (`GameRender`) that
holds all visual representation logic.  After this work:

- `NeuronServer` can link `GameLogic` directly and run authoritative game state.
- A headless bot client can link `GameLogic` without `NeuronClient`.
- The existing `Starstrike` executable links both `GameLogic` + `GameRender`
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
| 1.1 — Create `GameLogicPlatform.h` | ✅ Done | Forward-declared `ShapeStatic`, `ShapeFragmentData`, `ShapeMarkerData`; `#if GAMELOGIC_HAS_RENDERER` included `NeuronClient.h`.  **Later deleted in Phase 5.1** — `pch.h` now includes `NeuronCore.h` directly + forward declarations. |
| 1.2 — Change `GameLogic/pch.h` | ✅ Done | Originally changed to include `GameLogicPlatform.h`.  **Phase 5.1 update:** now includes `NeuronCore.h` directly + forward declarations for `ShapeStatic`, `ShapeFragmentData`, `ShapeMarkerData`. |
| 1.3 — `GAMELOGIC_HAS_RENDERER` define | ✅ Done | Originally added to both Debug and Release x64 configurations.  **Removed in Phase 5.1** — no longer needed after `GameApp.h` → `GameAppSim.h` migration. |

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
| `ArmyAnt` | ✅ `GameRender/ArmyAntRenderer` | ✅ `TypeArmyAnt` | ✅ Stubbed |
| `Egg` | ✅ `GameRender/EggRenderer` | ✅ `TypeEgg` | ✅ Stubbed |
| `Officer` | ✅ `GameRender/OfficerRenderer` | ✅ `TypeOfficer` | ✅ Stubbed |
| `TriffidEgg` | ✅ `GameRender/TriffidEggRenderer` | ✅ `TypeTriffidEgg` | ✅ Stubbed |

#### Wave 2 — Medium Entities (shape + GL state)

| Entity | Renderer | Registered | Old `Render()` Removed |
|--------|----------|------------|------------------------|
| `Engineer` | ✅ `GameRender/EngineerRenderer` | ✅ `TypeEngineer` | ✅ Stubbed |
| `Armour` | ✅ `GameRender/ArmourRenderer` | ✅ `TypeArmour` | ✅ Stubbed |
| `Lander` | ✅ `GameRender/LanderRenderer` | ✅ `TypeLander` | ✅ Stubbed |
| `Tripod` | ✅ `GameRender/TripodRenderer` | ✅ `TypeTripod` | ✅ Stubbed |
| `Centipede` | ✅ `GameRender/CentipedeRenderer` | ✅ `TypeCentipede` | ✅ Stubbed |
| `Spider` | ✅ `GameRender/SpiderRenderer` | ✅ `TypeSpider` | ✅ Stubbed |

#### Wave 3 — Complex Entities (custom geometry / special signatures)

| Entity | Renderer | Registered | Old `Render()` Removed |
|--------|----------|------------|------------------------|
| `Darwinian` | ✅ `GameRender/DarwinianRenderer` | ✅ `TypeDarwinian` | ✅ Stubbed |
| `Virii` | ✅ `GameRender/ViriiRenderer` | ✅ `TypeVirii` | ✅ Stubbed |
| `SoulDestroyer` | ✅ `GameRender/SoulDestroyerRenderer` | ✅ `TypeSoulDestroyer` | ✅ Stubbed |
| `LaserTrooper` | ✅ `GameRender/LaserTrooperRenderer` | ✅ `TypeLaserTroop` | ✅ Stubbed |

#### Wave 4 — Buildings

> **STATUS: ✅ Complete.**  All 57 building types (every entry in the
> `Building::Type` enum from `TypeFactory` through `TypeFeedingTube`) are
> registered in `InitGameRenderers()`.  The fallback
> `building->Render(predTime)` path in `location.cpp` is no longer hit for
> any known type.

| Category | Types | Renderer |
|----------|-------|----------|
| Tree (DX12 pipeline adapter) | `TypeTree` | `TreeBuildingRenderer` → `TreeRenderer` |
| Default (base `Building::Render`) | `TypeFactory`, `TypeCave`, `TypeBridge`, `TypePowerstation`, `TypeLibrary`, `TypeIncubator`, `TypeAntHill`, `TypeSafeArea`, `TypeUpgradePort`, `TypePrimaryUpgradePort`, `TypeAISpawnPoint`, `TypeDynamicHub` | `DefaultBuildingRenderer` |
| Null (invisible) | `TypeSpawnPopulationLock` | `NullBuildingRenderer` |
| Custom renderers | `TypeWall`, `TypeStaticShape`, `TypeScriptTrigger`, `TypeGodDish`, `TypeGunTurret`, `TypeControlTower`, `TypeRadarDish`, `TypeLaserFence`, `TypeFenceSwitch`, `TypeTriffid`, `TypeSpam`, `TypeResearchItem`, `TypeTrunkPort`, `TypeFeedingTube` | Dedicated `*BuildingRenderer` per type |
| Power family | `TypeGenerator`, `TypePylon`, `TypePylonStart`, `TypePylonEnd`, `TypeSolarPanel` | `GeneratorBuildingRenderer`, `PowerBuildingRenderer`, `PylonEndBuildingRenderer`, `SolarPanelBuildingRenderer` |
| Spawn family | `TypeSpawnLink`, `TypeSpawnPoint`, `TypeSpawnPointMaster` | `SpawnBuildingRenderer`, `SpawnPointBuildingRenderer`, `MasterSpawnPointBuildingRenderer` |
| Spirit receiver family | `TypeSpiritReceiver`, `TypeReceiverLink`, `TypeReceiverSpiritSpawner`, `TypeSpiritProcessor` | `SpiritReceiverBuildingRenderer`, `ReceiverBuildingRenderer`, `SpiritProcessorBuildingRenderer` |
| Blueprint family | `TypeBlueprintStore`, `TypeBlueprintConsole`, `TypeBlueprintRelay` | `BlueprintStoreBuildingRenderer`, `BlueprintConsoleBuildingRenderer`, `BlueprintRelayBuildingRenderer` |
| Mine/Track family | `TypeTrackLink`, `TypeTrackStart`, `TypeTrackEnd`, `TypeTrackJunction`, `TypeRefinery`, `TypeMine` | `MineBuildingRenderer`, `TrackJunctionBuildingRenderer`, `RefineryBuildingRenderer`, `MineShaftBuildingRenderer` |
| Fuel/Rocket family | `TypeFuelPipe`, `TypeFuelGenerator`, `TypeFuelStation`, `TypeEscapeRocket` | `FuelBuildingRenderer`, `FuelGeneratorBuildingRenderer`, `FuelStationBuildingRenderer`, `EscapeRocketBuildingRenderer` |
| Construction | `TypeYard`, `TypeDisplayScreen` | `ConstructionYardBuildingRenderer`, `DisplayScreenBuildingRenderer` |
| AI | `TypeAITarget` | `AITargetBuildingRenderer` |
| Dynamic | `TypeDynamicNode` | `DynamicNodeBuildingRenderer` |

> **Note:** `Teleport` is an intermediate base class for `RadarDish`, not a
> standalone building type — it has no enum entry.  `TeleportBuildingRenderer`
> serves as the base class for `RadarDishBuildingRenderer`, not a directly
> registered renderer.

#### Wave 5 — UI / Debug Windows

| Status | Notes |
|--------|-------|
| ✅ Done | All 17 UI/debug window file pairs (34 files) moved from `GameLogic/` to `GameRender/`.  `GameLogic.vcxproj` entries removed; `GameRender.vcxproj` entries added.  `OTHER_*` preference key macros extracted to `GameLogic/prefs_keys.h` (shared across `NeuronClient`, `Starstrike`, `GameRender`).  `prefs_other_window.h` now `#include`s `prefs_keys.h` instead of defining macros inline.  Files moved: `cheat_window`, `darwinia_window`, `debugmenu`, `drop_down_menu`, `input_field`, `mainmenus`, `message_dialog`, `network_window`, `prefs_graphics_window`, `prefs_keybindings_window`, `prefs_other_window`, `prefs_screen_window`, `prefs_sound_window`, `profilewindow`, `reallyquit_window`, `scrollbar`, `userprofile_window`. |

#### Wave 6 — Remaining WorldObject Subclass Renderers

> **STATUS: ✅ Done.**  All 5 source files have render companions in
> `GameRender/`.  `WorldObjectRenderer` base class and
> `WorldObjectRenderRegistry` provide type-indexed dispatch (matching the
> entity/building registry pattern).  All renderers registered in
> `InitGameRenderers()`.  Call sites updated (`Location::RenderWeapons()`
> uses `g_worldObjectRenderRegistry`; lasers use
> `WeaponRenderer::RenderLaser()`; flags use `FlagRenderer::Render()`).
> `WorldObject::Render(float)` virtual removed from `worldobject.h`.
> No `#if defined(GAMELOGIC_HAS_RENDERER)` guards remain in any
> `GameLogic` `.cpp`/`.h` files.  No rendering includes remain in the 5
> source files.  Dead declarations (`Virii::Render`, `SporeGenerator::RenderTail`)
> and dead commented-out render code (`virii.cpp` `RenderLowDetail`/`RenderGlow`)
> cleaned up.

| Source File | WorldObject Subclass(es) | Renderer | Status |
|-------------|-------------------------|----------|--------|
| `flag.cpp` | `Flag` | `GameRender/FlagRenderer` (static methods `Render`/`RenderText`) | ✅ |
| `snow.cpp` | `Snow` | `GameRender/SnowRenderer` (`WorldObjectRenderer` subclass) | ✅ |
| `airstrike.cpp` | `SpaceInvader` | `GameRender/SpaceInvaderRenderer` (`EntityRenderer` subclass) | ✅ |
| `darwinian.cpp` | `BoxKite` | `GameRender/BoxKiteRenderer` (`WorldObjectRenderer` subclass) | ✅ |
| `weapons.cpp` | `ThrowableWeapon`, `Rocket`, `Laser`, `Shockwave`, `MuzzleFlash`, `Missile`, `TurretShell` | `GameRender/WeaponRenderer` (single `WorldObjectRenderer` with internal type dispatch + static `RenderLaser()`) | ✅ |

**Implementation notes:**
- `FlagRenderer` uses static methods rather than inheriting `WorldObjectRenderer`
  because `Flag` is a standalone class (not a `WorldObject` subclass).
- `SpaceInvaderRenderer` inherits `EntityRenderer` (not `WorldObjectRenderer`)
  because `SpaceInvader` is an `Entity` subclass.
- `WeaponRenderer` uses a single class with internal type dispatch — chosen
  over 7 separate classes because the 7 weapon types share `renderer.h` and
  `particle_system.h` patterns.
- `WorldObjectRenderRegistry` provides the same registry-with-fallback
  pattern used by entities and buildings.

**`GAMELOGIC_HAS_RENDERER` removal: ✅ Done (Phase 5.1).**  The macro
was removed after extracting `GameAppSim` base class from `GameApp`,
which broke the `GameApp.h` → `GameMain` → `NeuronClient.h` dependency
chain.  59 `GameLogic` `.cpp` files now include `GameAppSim.h` (which
requires only `NeuronCore.h`).  `GameLogicPlatform.h` was also deleted.

### Phase 2 — Remaining Gaps (Legacy Code Not Yet Cleaned)

| Item | Status | Notes |
|------|--------|-------|
| `ITreeRenderBackend` consolidated into `IRenderBackend` | ✅ Done | `tree_render_interface.h` deleted; `TreeRenderer` now inherits `IRenderBackend` and implements `ReleaseBuilding(int)`; `Tree::~Tree()` calls `g_renderBackend->ReleaseBuilding()` |
| `Entity::BeginRenderShadow`/`RenderShadow`/`EndRenderShadow` | ✅ Done | All three static shadow helpers removed from `entity.h` and `entity.cpp`.  `ShadowRenderer` in `GameRender/` is the sole implementation. |
| Old `Render()` bodies in entity `.cpp` files | ✅ Cleaned | All 14 migrated entity types now have empty stub `Render()` bodies; full GL code deleted. Declarations remain in `.h` for Phase 4. |
| Old `Render()` bodies in building `.cpp` files | ✅ Cleaned | All building subclass `Render()`/`RenderAlphas()` bodies removed from `.cpp` files.  Declarations removed from `.h` files. |
| `Shape* m_shape` and `Shape* m_shapes[]` still on entities | ⚠️ Still coupled | `entity.h:66` still has `ShapeStatic* m_shape`; entity constructors now load shapes via `GetShape()` (delegates through `ShapeStore` — returns `nullptr` on headless).  `Resource::GetShapeStatic()` fully removed from GameLogic (Phase 6.1). |
| `building.h` render virtuals removed | ✅ Done | `Render()`, `RenderAlphas()`, `RenderLights()`, `RenderPorts()` removed from `building.h` and `building.cpp`.  All subclass overrides also removed.  `ShapeStatic* m_shape` still on line 98 — deferred to Phase 6. |
| `worldobject.h` `virtual void Render(float)` removed | ✅ Done | Wave 6 created companions for all remaining `WorldObject` subclass `Render()` overrides.  `WorldObject::Render(float)` virtual deleted from `worldobject.h`.  No `Render()` declarations remain on any `WorldObject` subclass in `GameLogic`. |

### Phase 3 — SimEventQueue

| Status | Notes |
|--------|-------|
| ✅ Complete | **Architecture:** generic `SimEventQueue<TEvent, Capacity>` template in `NeuronCore/SimEventQueue.h`; game-specific `GameSimEvent` (`std::variant` with 6 alternatives) in `GameLogic/GameSimEvent.h`; typedef + global in `GameLogic/GameSimEventQueue.h/.cpp`; `NeuronCore/Overloaded.h` provides `std::visit` lambda helper.  `DrainSimEvents()` in `Starstrike/main.cpp` uses `std::visit` + `Overloaded` lambdas (no `switch`/`case`).  All simulation-side `g_app->m_particleSystem`, `g_app->m_soundSystem`, and `g_explosionManager` calls migrated to `g_simEventQueue.Push()` with factory methods (`MakeParticle`, `MakeExplosion`, `MakeSoundStop`, `MakeSoundEntity`, `MakeSoundBuilding`, `MakeSoundOther`).  Late discovery: `wall.cpp` had one remaining direct `m_particleSystem->CreateParticle()` call — migrated to `SimEvent::MakeParticle`.  Stale `#include` directives removed.  Only `weapons.cpp` (rendering-side) retains `particle_system.h`.  `sizeof(GameSimEvent) <= 88` enforced by `static_assert`.  Vector/matrix fields use `Neuron::Math::GameVector3` / `Neuron::Math::GameMatrix` (not legacy types).  See `docs/SimEvent.md` for full implementation details including the 3-phase migration history (split/relocate → DirectXMath migration → `std::variant` conversion). |

### Phase 4 — Remove `Render()` from Base Classes

| Status | Notes |
|--------|-------|
| ✅ Done | All render declarations/bodies removed from all subclass `.h`/`.cpp` files (entities, buildings, and WorldObject subclasses).  `Building` virtuals (`Render`/`RenderAlphas`/`RenderLights`/`RenderPorts`) removed.  `Entity::Render` override removed.  `WorldObject::Render(float)` virtual removed (unblocked by Wave 6).  Entity shadow statics removed.  Dead building render helpers removed from 7 files.  All stale rendering includes removed from `GameLogic` simulation files — no `renderer.h`, `text_renderer.h`, `3d_sprite.h`, `ogl_extensions.h`, or `particle_system.h` includes remain.  Dead declarations (`Virii::Render`, `SporeGenerator::RenderTail`) and dead commented-out render code (`virii.cpp` 130-line `RenderLowDetail`/`RenderGlow` block) cleaned up.  `Shape* m_shape` on `Entity`/`Building` deferred to Phase 6. |

### Phase 5 — Clean `GameLogic` for Headless Use + `GameRender` Project

| Task | Status | Notes |
|------|--------|-------|
| 5.1 — Headless-clean `GameLogic` | ✅ Done | `GameLogic` cleaned in-place — no separate `GameSim` project needed.  `GameAppSim` base class extracted; `GAMELOGIC_HAS_RENDERER` removed; `GameLogicPlatform.h` deleted; `pch.h` includes `NeuronCore.h` only.  Headless compilation proven. |
| 5.2 — `GameRender.vcxproj` | ✅ Done (early) | Created with all companions from the start; static library, pch includes `NeuronClient.h`, includes `NeuronCore/NeuronClient/GameLogic/Starstrike` |
| 5.3 — Update link deps | ✅ Done | `Starstrike` links `GameRender` + `GameLogic`.  `NeuronServer` links `NeuronCore` + `GameLogic`.  `AdditionalIncludeDirectories` added to both Debug and Release configs (also fixed pre-existing bug: Release was missing include dirs entirely).  `ProjectReference` to `GameLogic.vcxproj` added. |

### Phase 6 — Presentation State Separation

| Task | Status | Notes |
|------|--------|-------|
| 6.0 — Audit `Shape*` usage | ✅ Done | 87 `m_shape` references across ~27 files cataloged into 5 categories: collision, per-frame markers, init-time markers, animation swap, explosion events.  Key finding: `ShapeStatic.h` has **zero GPU dependencies**; only `Render()` methods in `.cpp` use GL. |
| ~~6.1 — Move `ShapeStatic` to NeuronCore~~ | ❌ Cancelled | `ShapeStatic` is an asset/mesh type — it belongs in `NeuronClient`, not `NeuronCore`.  `NeuronCore` is strictly shared server/client infrastructure (math, networking, timers, logging).  The current compile-time setup (forward declarations in `pch.h`, full include via NeuronClient include path) is correct.  Decoupling happens at **runtime** via `ShapeStore` (6.1) + null-safety (6.2). |
| 6.1 — Shape loading via `ShapeStore` | ✅ Done | Abstract `ShapeStore` interface in `GameLogic/ShapeStore.h` with virtual `Get(const char*)`; `ClientShapeStore` in `GameRender/` delegates to `Resource::GetShapeStatic()`.  Global `g_shapeStore` set in `InitGameRenderers()` (`nullptr` on headless).  Inline `GetShape()` free function wraps null-check.  93 `Resource::GetShapeStatic()` calls across 46 files migrated to `GetShape()`.  50 stale `resource.h` includes removed (only `entity.cpp` retains it for `GetTextReader`). |
| 6.2 — Null-safety for headless `Shape*` | ❌ Not started | Add null guards to ~12 simulation-side `m_shape` dereferences (`EntityLeg::GetLegRootPos`, `Squadie::Attack`, `ArmyAnt::GetCarryMarker`, `Armour::GetEntrance`, `LaserFence::GetTopPosition`, etc.). |
| 6.3 — Decouple `SimEvent::Explosion` from `Shape*` | ❌ Not started | Replace `const ShapeStatic*` in `Explosion` variant with entity/building type ID; drain function looks up shape from renderer cache.  Removes last simulation→shape pointer crossing. |
| 6.4 — Move `m_shape` ownership to renderers (optional) | ❌ Not started | Remove `ShapeStatic* m_shape` from `Entity`/`Building` base classes; renderers own shapes.  Lower priority — headless works after 6.1–6.2. |

### Phase 7 — Headless Bot Client

| Status | Notes |
|--------|-------|
| ❌ Not started | Phase 5.3 prerequisite now met.  Requires `Shape*` ownership moved to renderers (Phase 6) for entity constructors to work headless. |

---

### Summary: What's Next

The highest-impact remaining work items, in recommended order:

1. ~~**Create `LanderRenderer` and `LaserTrooperRenderer`**~~ ✅ Done —
   Waves 2 and 3 entity renderers are now complete (15 of 15 entity types).
2. ~~**Wave 4 — Building renderers**~~ ✅ Done — all 57 building types
   registered in `InitGameRenderers()`.  No fallback path is hit.
3. ~~**Delete old entity `Render()` bodies**~~ ✅ Done — all 14 migrated
   entity `.cpp` files now have empty stubs; full GL code removed.
4. ~~**Migrate `ITreeRenderBackend` → `IRenderBackend`**~~ ✅ Done —
   `tree_render_interface.h` deleted; `TreeRenderer` inherits
   `IRenderBackend`; single `g_renderBackend` global.
5. ~~**Phase 3 — `SimEventQueue` pilot**~~ ✅ Done — `GameSimEvent.h`,
   `GameSimEventQueue.h/.cpp`, `NeuronCore/SimEventQueue.h` created;
   `Tree::Advance()` / `Damage()` fully migrated; `DrainSimEvents()` in
   `main.cpp`.
6. ~~**Phase 3 — remaining entity/building types**~~ ✅ Done — all
   simulation-side `g_explosionManager`, `m_particleSystem`,
   `m_soundSystem` calls migrated.  Stale includes removed.
   `std::variant`-based `GameSimEvent` with 6 alternatives;
   `DrainSimEvents()` uses `std::visit` + `Overloaded` lambdas.
   See `docs/SimEvent.md` for full details.
7. ~~**Phase 2 Wave 5 — UI/debug windows**~~ ✅ Done — all 17 UI/debug
   window file pairs moved to `GameRender/`; `OTHER_*` macros extracted
   to `GameLogic/prefs_keys.h`.
8. ~~**Phase 4 — Remove base-class `Render()`**~~ ✅ Done —
   All entity/building render declarations, bodies, shadow statics, and
   dead render helpers removed.  25 stale rendering includes removed
   (16× `renderer.h`, 8× `text_renderer.h`, 1× `ogl_extensions.h`).
   Only 5 files retain live rendering includes (all `WorldObject`
   subclasses with `Render()`: `SpaceInvader`, `BoxKite`, 7 weapons,
   `Flag`, `Snow`).  `WorldObject::Render` intentionally kept.
   `Shape* m_shape` deferred to Phase 6.
9. ~~**Phase 2 Wave 6 — WorldObject subclass renderers**~~ ✅ Done —
   `FlagRenderer`, `SnowRenderer`, `BoxKiteRenderer`,
   `SpaceInvaderRenderer`, `WeaponRenderer` all in `GameRender/`;
   `WorldObjectRenderer` base + `WorldObjectRenderRegistry` for dispatch;
   all registered in `InitGameRenderers()`.  `WorldObject::Render(float)`
   removed from `worldobject.h`.  No `#if GAMELOGIC_HAS_RENDERER` guards
   remain in GameLogic source files.  Dead declarations
   (`Virii::Render`, `SporeGenerator::RenderTail`) and dead commented-out
   code cleaned up.  ~~`GAMELOGIC_HAS_RENDERER` removal blocked by
   `GameApp.h` → `GameMain` dependency (Phase 5 scope).~~ Resolved in
   Phase 5.1.
10. ~~**Phase 5.1 — Headless-clean `GameLogic`**~~ ✅ Done —
    `GameAppSim` base class extracted from `GameApp` (breaks the
    `GameApp.h` → `GameMain` → `NeuronClient.h` dependency chain);
    59 GameLogic `.cpp` files switched from `GameApp.h` to `GameAppSim.h`;
    `GAMELOGIC_HAS_RENDERER` removed from `GameLogic.vcxproj`;
    `GameLogicPlatform.h` deleted; `pch.h` simplified to `NeuronCore.h`
    + forward declarations; `rgb_colour.h/.cpp` moved from NeuronClient
    to NeuronCore.  **GameLogic IS the simulation library** — no separate
    `GameSim` project needed.  Headless compilation proven.
11. ~~**Phase 6.1 — Shape loading via `ShapeStore`**~~ ✅ Done —
    abstract `ShapeStore` interface in `GameLogic/ShapeStore.h`;
    `ClientShapeStore` in `GameRender/` delegates to
    `Resource::GetShapeStatic()`.  93 call sites across 46 files
    migrated to `GetShape()`.  50 stale `resource.h` includes removed.
12. **Phase 6.2 — Null-safety for headless `Shape*`** — null guards on
    ~12 simulation-side `m_shape` dereferences.  **After this step,
    headless compilation and execution are fully unblocked for Phase 7.**
13. **Phase 6.3 — Decouple `SimEvent::Explosion`** — replace
    `const ShapeStatic*` with type ID in the `Explosion` variant.
14. **Phase 6.4 — Move `m_shape` ownership to renderers** (optional).
15. **Phase 7** — Headless bot client.

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

1. ~~**`NeuronServer` cannot link `GameLogic`.**~~
   ~~`GameLogic/pch.h` → `NeuronClient.h` → `opengl_directx.h` → D3D12
   device headers.  A headless server has no GPU.~~
   **✅ Resolved (Phase 5.1):** `pch.h` now includes `NeuronCore.h` only.
   `GameAppSim.h` replaces `GameApp.h` in all 59 `.cpp` files.
2. ~~**Entity constructors load rendering assets.**~~
   ~~e.g. `ArmyAnt::ArmyAnt()` calls `Resource::GetShapeStatic("armyant.shp")`.
   A headless process has no resource/shape system.~~
   **✅ Resolved (Phase 6.1):** All 93 `Resource::GetShapeStatic()` calls
   migrated to `GetShape()` which returns `nullptr` on headless.
   *(Phase 6.2 adds null guards to ~12 simulation-side `m_shape` dereferences.)*
3. ~~**Simulation and rendering share mutable state.**~~
   ~~`Advance()` and `Render()` both read/write `m_shape`, `m_front`,
   `m_pos`, `m_scale` with no separation of simulation state from
   presentation state.~~
   **✅ Resolved (Phase 4):** All `Render()` methods removed from simulation
   classes.  Renderers receive `const` references.
4. ~~**Side-effects in simulation call rendering APIs.**~~
   ~~`ArmyAnt::ChangeHealth()` calls `g_explosionManager.AddExplosion(m_shape, transform)`
   and `g_app->m_particleSystem->CreateParticle(...)` — both require GPU
   state.~~
   **✅ Resolved (Phase 3):** All such calls migrated to `SimEventQueue`.

### PCH Dependency Chain

**Original** (before Phase 1):
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

**Current** (after Phase 5.1):
```
GameLogic/pch.h
  └─ NeuronCore.h                 ← only dependency
  + forward declarations for ShapeStatic, ShapeFragmentData, ShapeMarkerData
```

Only `NeuronCore.h` is needed for pure simulation.  `GameAppSim.h` (included
by individual `.cpp` files, not the PCH) also requires only `NeuronCore.h`.

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
| GPU cleanup on destruction | `IRenderBackend` interface (`GameLogic/render_backend_interface.h`) + global `g_renderBackend`.  `Tree::~Tree()` calls `g_renderBackend->ReleaseBuilding(uniqueId)`.  ~~Previously `ITreeRenderBackend` / `g_treeRenderBackend` — migrated and deleted.~~ |
| Call-site dispatch | `Starstrike/location.cpp:1082` — manual `if (building->m_type == Building::TypeTree)` dispatch to `TreeRenderer::Get().DrawTree(...)` |
| Simulation file | `GameLogic/tree.cpp` — ~~still includes `soundsystem.h`, `particle_system.h`~~ removed after Phase 3; side-effects migrated to `GameSimEvent` / `g_simEventQueue`; includes `GameAppSim.h` for `g_app->m_location` (fire spread) |

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
| `Tree::Advance()` calls `g_app->m_particleSystem->CreateParticle(...)` and `g_app->m_soundSystem->...` directly | ✅ Done | Migrated to `SimEventQueue` — 5 particle + 4 sound calls replaced with event pushes; `DrainSimEvents()` in `main.cpp` processes them |
| `Tree::RenderBranch()` contains dead GL code paths (`glTexCoord2f`, `glVertex3fv`) | ✅ Done | Renamed to `CalcBranchRadius()`; GL paths deleted |
| `Tree` inherits `Building::m_shape` (never used, always `nullptr`) | ❌ Open | Resolved when Phase 4 removes `m_shape` from `Building` base class |
| Call-site uses manual `if` type check instead of registry dispatch | ✅ Done | `BuildingRenderRegistry` in `location.cpp` with fallback |
| `TreeRenderer` has no `BuildingRenderer` base class — cannot be stored in registry | ✅ Done | `TreeBuildingRenderer : BuildingRenderer` adapter in `GameRender/` |
| `TreeRenderer::DrawTree()` takes concrete `Tree*`, not `const Building&` | ✅ Done | `DrawTree` takes `const Tree&`; adapter `static_cast`s from `const Building&` |
| `TreeRenderer::EnsureUploaded()` mutates `Tree` (`m_meshDirty = false`, calls `Generate()`) | ✅ Done | `Generate()` now in `Tree::Advance()`; renderer uses `m_meshVersion` for GPU-side staleness |
| `ITreeRenderBackend` is tree-specific; one per type doesn't scale | ✅ Done | `tree_render_interface.h` deleted; `TreeRenderer` inherits `IRenderBackend` directly and implements `ReleaseBuilding(int)`; `Tree::~Tree()` calls `g_renderBackend->ReleaseBuilding()` |

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
│                  GameLogic (cleaned)                 │
│  Pure simulation: Entity, Building, AI, weapons,    │
│  pathfinding, economy, spirit logic.                │
│  NO GL, NO D3D12, NO Renderer*.                     │
│  pch.h includes NeuronCore.h only.                  │
│  Compiles for server AND client.                    │
└──────────────┬──────────────────────┬───────────────┘
               │                      │
    ┌──────────▼──────────┐  ┌────────▼────────────┐
    │   NeuronServer      │  │  GameRender          │
    │   (headless)        │  │  Render logic for    │
    │   Links: NeuronCore │  │  entities, buildings │
    │        + GameLogic  │  │  Links: NeuronClient │
    │                     │  │       + GameLogic    │
    └─────────────────────┘  └────────┬────────────┘
                                      │
                             ┌────────▼────────────┐
                             │   Starstrike (exe)   │
                             │   Links: NeuronClient│
                             │       + GameLogic    │
                             │       + GameRender   │
                             └─────────────────────┘
```

A **headless bot client** links `NeuronCore` + `GameLogic` only (no
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
headers.  Originally used a preprocessor guard (`GAMELOGIC_HAS_RENDERER`);
this was later removed in Phase 5.1 after `GameAppSim` extraction.

### 1.1 — Create `GameLogic/GameLogicPlatform.h`

> **Deleted in Phase 5.1.**  Original content shown for historical reference.

```cpp
// GameLogic/GameLogicPlatform.h  [DELETED]
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

**Final state** (after Phase 5.1):
```cpp
#pragma once
#include "NeuronCore.h"
class ShapeStatic;
struct ShapeFragmentData;
struct ShapeMarkerData;
```

### 1.3 — Add preprocessor define to `GameLogic.vcxproj`

Add `GAMELOGIC_HAS_RENDERER` to the preprocessor definitions for both
Debug and Release configurations.  This preserves current behavior — nothing
breaks.

~~The server build of `GameLogic` (or future `GameSim`) omits this define.~~
**UPDATE (Phase 5.1):** This define has been removed entirely.  `GameLogic`
now compiles without it after `GameAppSim` extraction.

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

> **STATUS: ✅ Done.** `IRenderBackend` interface in
> `render_backend_interface.h` is now the sole cleanup interface.
> `tree_render_interface.h` has been deleted.  `TreeRenderer` inherits
> `IRenderBackend` directly and implements `ReleaseBuilding(int)`.
> `Tree::~Tree()` calls `g_renderBackend->ReleaseBuilding(uniqueId)`.

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

**Wave 4 — Buildings**: ✅ Complete (57/57 types registered)
- All building types from `TypeFactory` through `TypeFeedingTube` have
  registered renderers.  See Wave 4 table above for full breakdown.

**Wave 5 — UI / debug windows**: ✅ Complete
- All 17 UI/debug window file pairs moved from `GameLogic/` to `GameRender/`

**Wave 6 — Remaining WorldObject subclasses** (not `Entity` / `Building`): ✅ Complete
- `Flag` → `FlagRenderer`, `Snow` → `SnowRenderer`, `BoxKite` →
  `BoxKiteRenderer`, `SpaceInvader` → `SpaceInvaderRenderer`,
  7 weapon types → `WeaponRenderer` (single class with internal dispatch)
- `WorldObjectRenderer` base class + `WorldObjectRenderRegistry` for
  type-indexed dispatch
- `WorldObject::Render(float)` removed from `worldobject.h`
- No `#if GAMELOGIC_HAS_RENDERER` guards remain in GameLogic source files

### Per-WorldObject Migration Checklist

For each remaining `WorldObject` subclass with a `Render()` override:

- [x] Create `GameRender/<Type>Renderer.h/.cpp` (or a combined
      `WeaponRenderer` for the 7 weapon types)
- [x] Move `Render()` body from the `#if defined(GAMELOGIC_HAS_RENDERER)`
      block to the companion
- [x] Move rendering-only `#include` directives to the companion
- [x] Update the call site (e.g. `Location::RenderWeapons()`) to call
      the companion instead of `worldObject->Render(predTime)`
- [x] Remove the `#if` / `#endif` guards from the simulation `.cpp`
- [x] Delete `Render()` declaration from the class header
- [x] Build and test in all configurations

Once all WorldObject subclass renderers are extracted:

- [x] Remove `virtual void Render(float)` from `WorldObject`
- [x] Delete `GAMELOGIC_HAS_RENDERER` from all `.vcxproj` files
      (**Done** — removed in Phase 5.1 after `GameAppSim` extraction)
- [x] Delete `GameLogicPlatform.h` (**Done** — `pch.h` now includes
      `NeuronCore.h` directly + forward declarations)

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

> **STATUS: ✅ Complete.**  Generic `SimEventQueue<TEvent, Capacity>`
> template in `NeuronCore/SimEventQueue.h`; game-specific `GameSimEvent`
> (`std::variant` with 6 alternatives: `ParticleSpawn`, `Explosion`,
> `SoundStop`, `SoundEntityEvent`, `SoundBuildingEvent`,
> `SoundOtherEvent`) in `GameLogic/GameSimEvent.h`; typedef + global in
> `GameLogic/GameSimEventQueue.h/.cpp`.  `DrainSimEvents()` in
> `Starstrike/main.cpp` uses `std::visit` + `Overloaded` lambdas
> (`NeuronCore/Overloaded.h`).  All simulation-side particle, sound, and
> explosion calls across all entity/building `.cpp` files migrated to
> `g_simEventQueue.Push()` with factory methods.  Stale includes removed.
> `sizeof(GameSimEvent) <= 88` enforced by `static_assert`.  Vector/matrix
> fields use `Neuron::Math::GameVector3` / `Neuron::Math::GameMatrix`.
> See `docs/SimEvent.md` for the full 3-phase implementation history.

Simulation code called rendering/audio systems directly during
`Advance()` and `ChangeHealth()`.  These have been replaced with a deferred
event queue that the client drains after simulation and the server discards.

> **Full implementation details** are in `docs/SimEvent.md`, which documents
> the 3-phase migration: Phase 1 (split & relocate), Phase 2 (DirectXMath
> migration), Phase 3 (`std::variant` conversion).

### Architecture

| Component | File | Purpose |
|-----------|------|---------|
| Generic queue template | `NeuronCore/SimEventQueue.h` | `SimEventQueue<TEvent, Capacity>` — fixed-capacity flat array, no per-frame allocations |
| Game event type | `GameLogic/GameSimEvent.h` | `std::variant`-based tagged union with 6 alternatives + `Visit()` helper + factory methods |
| Typedef + global | `GameLogic/GameSimEventQueue.h/.cpp` | `using GameSimEventQueue = SimEventQueue<GameSimEvent, 1024>; extern GameSimEventQueue g_simEventQueue;` |
| Visitor helper | `NeuronCore/Overloaded.h` | Aggregate template for `std::visit` lambda dispatch |
| Client drain | `Starstrike/main.cpp` | `DrainSimEvents()` — `std::visit` + `Overloaded` lambdas after `Location::Advance()` |

### `GameSimEvent` variant alternatives

```cpp
struct GameSimEvent
{
    struct ParticleSpawn   { GameVector3 pos, vel; int particleType; float particleSize; RGBAColour particleColour; };
    struct Explosion       { const ShapeStatic* shape; GameMatrix transform; float fraction; };
    struct SoundStop       { WorldObjectId objectId; const char* eventName; };   // nullptr = stop all
    struct SoundEntityEvent   { WorldObjectId objectId; const char* eventName; };
    struct SoundBuildingEvent { WorldObjectId objectId; const char* eventName; };
    struct SoundOtherEvent    { GameVector3 pos; WorldObjectId objectId; int soundSourceType; const char* eventName; };

    using Variant = std::variant<ParticleSpawn, Explosion, SoundStop,
                                 SoundEntityEvent, SoundBuildingEvent, SoundOtherEvent>;
    Variant data;

    template<typename Visitor> decltype(auto) Visit(Visitor&& _vis) const;

    // Factory helpers
    [[nodiscard]] static GameSimEvent MakeParticle(...);
    [[nodiscard]] static GameSimEvent MakeExplosion(...);
    [[nodiscard]] static GameSimEvent MakeSoundStop(...);
    [[nodiscard]] static GameSimEvent MakeSoundEntity(...);
    [[nodiscard]] static GameSimEvent MakeSoundBuilding(...);
    [[nodiscard]] static GameSimEvent MakeSoundOther(...);
};
static_assert(sizeof(GameSimEvent) <= 88);
using SimEvent = GameSimEvent; // back-compat alias
```

Vector/matrix fields use `Neuron::Math::GameVector3` /
`Neuron::Math::GameMatrix` (not legacy types).  Implicit conversions in
`LegacyVector3` and `Matrix34` allow seamless interop at call sites.

### Replacement example — `ArmyAnt::ChangeHealth()`

```diff
- g_explosionManager.AddExplosion(m_shape, transform, fraction);
+ g_simEventQueue.Push(SimEvent::MakeExplosion(m_shape, transform, fraction));
```

### Client drain loop (in `Starstrike` main loop, after `Location::Advance`)

```cpp
for (int i = 0; i < g_simEventQueue.Count(); ++i)
{
    g_simEventQueue.Get(i).Visit(Overloaded{
        [](const GameSimEvent::ParticleSpawn& e) {
            g_app->m_particleSystem->CreateParticle(e.pos, e.vel, e.particleType, e.particleSize);
        },
        [](const GameSimEvent::Explosion& e) {
            g_explosionManager.AddExplosion(e.shape, e.transform, e.fraction);
        },
        [](const GameSimEvent::SoundStop& e)          { /* sound system dispatch */ },
        [](const GameSimEvent::SoundEntityEvent& e)   { /* sound system dispatch */ },
        [](const GameSimEvent::SoundBuildingEvent& e)  { /* sound system dispatch */ },
        [](const GameSimEvent::SoundOtherEvent& e)     { /* sound system dispatch */ },
    });
}
g_simEventQueue.Clear();
```

Server ignores or logs for replay.

---

## Phase 4 — Remove `Render()` from Base Classes

> **STATUS: ✅ Done.**  All entity, building, and WorldObject subclass
> render declarations/bodies removed.  `WorldObject::Render(float)` virtual
> removed.  `Building` virtuals removed.  `Entity::Render` override removed.
> Entity shadow statics removed.  Dead render helpers, stale includes, and
> dead declarations cleaned up.  `Shape* m_shape` deferred to Phase 6.

All entity and building types have render companions registered (Waves 1–4).
All WorldObject subclass renderers extracted (Wave 6).

1. ~~Remove `virtual void Render(float)` from `WorldObject`.~~ ✅
2. ~~Remove `virtual void Render(float)` from `Entity`.~~ ✅
3. ~~Remove `virtual void Render(float)` / `RenderAlphas(float)` /
   `RenderLights()` / `RenderPorts()` from `Building`.~~ ✅
4. Remove `Shape* m_shape` from `Entity` and `Building`. — deferred to Phase 6
5. Remove `Shape* m_shapes[]` from entity subclasses. — deferred to Phase 6
6. Remove `ShapeMarkerData* m_carryMarker` and similar. — deferred to Phase 6
7. ~~Remove static shadow helpers (`BeginRenderShadow` etc.) from `Entity`.~~ ✅
8. ~~Remove `#include "renderer.h"` / `#include "shape.h"` from all
   simulation files.~~ ✅

---

## Phase 5 — Clean `GameLogic` for Headless Use + `GameRender` Project

> **STATUS: ✅ Complete.**  5.1 (`GameLogic` headless cleanup)
> done via in-place refactoring — no separate `GameSim` project needed.
> 5.2 (`GameRender.vcxproj`) completed early.  5.3 (link deps) done.
> 5.4 is N/A.

### 5.1 — Headless-clean `GameLogic` (in-place)

> **STATUS: ✅ Done.**  `GameLogic` is now the pure-simulation library.
> Headless compilation proven.

**Original plan** called for creating a separate `GameSim.vcxproj` and
moving all simulation files there.  After completing Phases 1–4 and Wave 6,
`GameLogic` was already clean enough to serve as the simulation library
directly.  Creating a separate project would have been unnecessary churn.

**What was done instead:**

1. **Extracted `GameAppSim` base class** (`GameLogic/GameAppSim.h`):
   - Contains ALL data members from `GameApp` (with default member
     initializers), the `GameMode` enum, virtual lifecycle methods with
     empty defaults, and static utility methods.
   - Requires only `NeuronCore.h` + `rgb_colour.h` — no `NeuronClient`
     dependency.
   - Declares `extern GameAppSim* g_app;`.

2. **Refactored `GameApp`** (`Starstrike/GameApp.h`):
   - Now `class GameApp : public GameAppSim, public GameMain` — only
     framework overrides and lifecycle method overrides, no data members.
   - `Starstrike/GameApp.cpp`: `GameAppSim* g_app = nullptr;` (was
     `GameApp*`); member initializer list removed (DMIs handle it);
     3 static methods qualified as `GameAppSim::`.

3. **Migrated 59 GameLogic `.cpp` files**: `#include "GameApp.h"` →
   `#include "GameAppSim.h"`.

4. **Moved `rgb_colour.h/.cpp`** from `NeuronClient` to `NeuronCore`
   (pure data type, zero rendering dependencies).

5. **Removed `GAMELOGIC_HAS_RENDERER`** from `GameLogic.vcxproj` (both
   Debug and Release configurations).

6. **Deleted `GameLogicPlatform.h`** and simplified `GameLogic/pch.h` to:
   ```cpp
   #pragma once
   #include "NeuronCore.h"
   class ShapeStatic;
   struct ShapeFragmentData;
   struct ShapeMarkerData;
   ```

7. **Updated `Starstrike/main.cpp`**: 4 call sites use
   `static_cast<GameApp*>(g_app)->IsActive()`/`IsSuspended()` (these are
   `GameMain` methods, not simulation concepts).

**Remaining include dependencies** (utility headers, not GPU):
GameLogic `.cpp` files still include NeuronClient utility headers
(1× `resource.h` — only `entity.cpp` for `GetTextReader`;
44× `ShapeStatic.h`, 44× `math_utils.h`,
25× `text_stream_readers.h`, 25× `file_writer.h`, etc.) and Starstrike
infrastructure headers (35× `team.h`, 20× `main.h`, 11× `unit.h`, etc.).
None of these are GPU/rendering headers.  `resource.h` was reduced from
51 to 1 include by Phase 6.1 (`ShapeStore` migration).  Full removal of
`ShapeStatic.h` is Phase 6.4 scope (moving `Shape*` ownership to renderers).

### 5.2 — `GameRender/GameRender.vcxproj` (static library)

> **STATUS: ✅ Done (created early).**  All companions placed in
> `GameRender/` from the start.  `pch.h` includes `NeuronClient.h`;
> additional include dirs cover `NeuronCore`, `NeuronClient`, `GameLogic`,
> `Starstrike`.  `TreeRenderer` (the DX12 pipeline) remains in
> `NeuronClient/`.

- ~~Move all `*Renderer.cpp/.h` files from `NeuronClient/` into `GameRender/`.~~
  Companions were placed directly in `GameRender/` — no move needed.
- Contains `EntityRenderRegistry`, `BuildingRenderRegistry`.
- `DrainSimEvents()` consumer logic lives in `Starstrike/main.cpp` (not `GameRender`).
- `pch.h` includes `NeuronClient.h`.
- Additional include directories: `$(SolutionDir)NeuronCore`,
  `$(SolutionDir)NeuronClient`, `$(SolutionDir)GameLogic`,
  `$(SolutionDir)Starstrike`.

### 5.3 — Update link dependencies

> **STATUS: ✅ Done.**  `Starstrike` links `GameRender` + `GameLogic`.
> `NeuronServer` links `NeuronCore` + `GameLogic`.

| Project | Links | Status |
|---------|-------|--------|
| `Starstrike` | `NeuronClient`, `NeuronCore`, `GameLogic`, `GameRender` | ✅ |
| `NeuronServer` | `NeuronCore`, `GameLogic` | ✅ |
| `BotClient` (future) | `NeuronCore`, `GameLogic` | ❌ Not yet created |

### 5.4 — Delete `GameLogic/` project

> **STATUS: N/A.**  `GameLogic` IS the simulation library — no separate
> `GameSim` project was created, so there is nothing to delete.  The original
> plan assumed a rename/move to `GameSim/`; the in-place cleanup approach
> (Phase 5.1) made this unnecessary.

---

## Phase 6 — Separate Simulation State from Presentation State

> **STATUS: 6.1 ✅ Done; 6.2–6.4 ❌ Not started.**  Phase 6.0 audit
> complete.  ~~Phase 6.1 (move `ShapeStatic` to NeuronCore) cancelled~~ —
> `ShapeStatic` stays in `NeuronClient`.  **Phase 6.1 (`ShapeStore`)
> complete:** abstract `ShapeStore` in `GameLogic/ShapeStore.h`;
> `ClientShapeStore` in `GameRender/` delegates to
> `Resource::GetShapeStatic()`.  93 call sites migrated; 50 stale
> `resource.h` includes removed.  Null-safety guards (6.2) protect ~12
> dereference sites; explosion events (6.3) replace `ShapeStatic*` with
> type IDs.

### 6.0 — Audit `Shape*` usage across GameLogic

> **STATUS: ✅ Done.**  87 `m_shape` references across ~27 files.

#### Key finding

`NeuronClient/ShapeStatic.h` includes **only NeuronCore types**
(`llist.h`, `matrix34.h`, `rgb_colour.h`, `LegacyVector3.h`).  In
`ShapeStatic.cpp`, 95% of the code is pure CPU — `Load()`, `RayHit()`,
`SphereHit()`, `ShapeHit()`, `CalculateCenter()`, `CalculateRadius()`,
`GetMarkerData()`, `GetMarkerWorldMatrix()`.  Only two `Render()`
overloads (+ `ShapeFragmentData::Render`/`RenderSlow`) use GL calls.
`ShapeStatic` stays in `NeuronClient` (it's an asset/mesh type, not shared
server/client infrastructure).  The compile-time setup already works:
`GameLogic/pch.h` forward-declares the types; `.cpp` files include
`ShapeStatic.h` via the NeuronClient include path.  The decoupling problem
is **runtime** — entity constructors call `Resource::GetShapeStatic()`
which requires the full client resource system.  The fix is `ShapeStore`
(Phase 6.1).

#### Usage categories

| Category | Description | Call sites | Headless requirement |
|----------|-------------|------------|---------------------|
| **Collision** | `CalculateCenter`/`CalculateRadius` for bounding; `RayHit`/`SphereHit`/`ShapeHit` for intersection | `Entity::Begin`, `Building::Initialise`, `Building::SetDetail`, `Building::DoesRayHit`, `Building::DoesSphereHit`, `Building::DoesShapeHit`, `Entity::RayHit`, `LaserFence::DoesRayHit` | Shape data must be available (loaded from file) |
| **Per-frame markers** | `GetMarkerWorldMatrix()` every frame | `EntityLeg::GetLegRootPos()` (Spider/Tripod IK), `ArmyAnt::GetCarryMarker()`, `Squadie::Attack()`/`FireSecondaryWeapon()`, `Armour::GetEntrance()`, `LaserFence::GetTopPosition()` | Shape data must be available |
| **Init-time markers** | `GetMarkerWorldMatrix()` or `GetMarkerData()` in constructors / `Initialise()` | `EntityLeg` constructor (thigh/shin lengths), `Building::SetShapePorts()` (port positions), `Building::SetShapeLights()` (light positions), `SporeGenerator` (tail markers), `Triffid` (launch/stem) | Shape data must be available |
| **Animation swap** | Swap `m_shape` between pre-loaded static shapes | `ArmyAnt` (3 shapes), `Centipede` (head/body), `SoulDestroyer` (head/tail) | Shapes must be loaded (or swap becomes no-op) |
| **Explosion events** | Pass `m_shape` to `SimEvent::MakeExplosion()` | 8+ call sites: `ArmyAnt`, `Armour`, `Spider`, `Tripod`, `SporeGenerator`, `Squadie`, `Building::Destroy()` ×3 | Can be replaced with type ID lookup |

#### Entities with marker fields (simulation-visible)

| Entity | Marker fields | Per-frame? |
|--------|--------------|------------|
| `ArmyAnt` | `m_carryMarker` | Yes — `GetCarryMarker()` positions carried spirit |
| `Armour` | `m_markerEntrance`, `m_markerFlag` | Yes — `GetEntrance()` positions passengers |
| `Squadie` | `m_laser`, `m_brass`, `m_eye1`, `m_eye2` | Yes — `Attack()` / `FireSecondaryWeapon()` fire position |
| `Officer` | `m_flagMarker` | Init-time only |
| `Spider` | `m_eggLay` + 6 `EntityLeg` root markers | Yes — `EntityLeg::GetLegRootPos()` every frame |
| `Tripod` | 3 `EntityLeg` root markers | Yes — `EntityLeg::GetLegRootPos()` every frame |
| `SporeGenerator` | `m_eggMarker`, `m_tail[4]` | Init-time only |
| `SoulDestroyer` | `s_tailMarker` (static) | Init-time only |
| `Triffid` | `m_launchPoint`, `m_stem` | Init-time only (via `Building::SetShape`) |
| `LaserFence` | `m_marker2` | Yes — `GetTopPosition()` fence endpoint |

#### Entities with no shape loading

`Darwinian` (hardcodes `m_centerPos`/`m_radius` in `Begin()`),
`SpaceInvader` (via `AirstrikeUnit`, no shape in constructor).

#### Entities with multiple shapes

`ArmyAnt` (3 in local array, animation swap), `Centipede` (2 statics:
head/body), `SoulDestroyer` (2 statics: head/tail).

#### `EntityLeg` — most architecturally challenging

`EntityLeg` loads 2 shapes per leg (upper/lower) in its constructor, and
calls `m_parent->m_shape->GetMarkerWorldMatrix(m_rootMarker, rootMat)`
**every frame** in `GetLegRootPos()`.  Spider has 6 legs, Tripod has 3.
This is the deepest per-frame `Shape*` dependency in simulation.

### ~~6.1 — Move `ShapeStatic` to NeuronCore~~ (Cancelled)

> **STATUS: ❌ Cancelled.**  `ShapeStatic` is an asset/mesh type — it
> belongs in `NeuronClient`, not `NeuronCore`.  `NeuronCore` is strictly
> shared server/client infrastructure (math, networking, timers, logging).
> Moving `ShapeStatic` would also cascade to `text_stream_readers`,
> `math_utils`, `plane`, and `vector2` — 6+ files pulled into NeuronCore
> that don't belong there.
>
> The current compile-time setup works correctly:
> - `GameLogic/pch.h` forward-declares `ShapeStatic`, `ShapeFragmentData`,
>   `ShapeMarkerData`
> - Individual `.cpp` files `#include "ShapeStatic.h"` via the NeuronClient
>   include path
> - `GameLogic.lib` compiles fine in the full solution context
>
> The actual problem is **runtime**: entity constructors call
> `Resource::GetShapeStatic()` which requires the full client
> resource system.  This is solved by `ShapeStore` (Phase 6.1) +
> null-safety (Phase 6.2) — no header moves needed.

### 6.1 — Shape loading via `ShapeStore`

> **STATUS: ✅ Done.**  Abstract `ShapeStore` interface in
> `GameLogic/ShapeStore.h`; `ClientShapeStore` concrete implementation in
> `GameRender/ClientShapeStore.h/.cpp`.  93 `Resource::GetShapeStatic()`
> calls across 46 GameLogic `.cpp` files migrated to `GetShape()`.
> 50 stale `resource.h` includes removed (only `entity.cpp` retains it
> for `g_app->m_resource->GetTextReader("stats.txt")`).  Build verified.

**Goal:** Entity/building constructors no longer call
`Resource::GetShapeStatic()` directly.  Instead, shapes are
loaded through an abstract `ShapeStore` interface that the client
populates and headless processes leave as `nullptr`.

**Implementation (deviation from original design):**

The original plan specified a `Register`/`Get` pattern where renderers
would pre-populate a hash map.  The actual implementation is simpler:
`ClientShapeStore::Get()` delegates directly to
`Resource::GetShapeStatic()`, which already has a lazy-loading cache
(`inline static HashTable<ShapeStatic*> m_shapes`).  No pre-registration
is needed — the existing `Resource` cache serves that purpose.

```cpp
// GameLogic/ShapeStore.h
#pragma once

class ShapeStatic;

class ShapeStore
{
public:
    virtual ~ShapeStore() = default;
    [[nodiscard]] virtual ShapeStatic* Get(const char* _name) = 0;
};

extern ShapeStore* g_shapeStore;  // nullptr on headless

[[nodiscard]] inline ShapeStatic* GetShape(const char* _name)
{
    return g_shapeStore ? g_shapeStore->Get(_name) : nullptr;
}
```

```cpp
// GameRender/ClientShapeStore.h — delegates to Resource::GetShapeStatic()
class ClientShapeStore : public ShapeStore
{
public:
    [[nodiscard]] ShapeStatic* Get(const char* _name) override;
};
```

**Wiring:** `GameRender/GameRender.cpp` — `InitGameRenderers()` sets
`g_shapeStore = &s_clientShapeStore;` (static instance) as its first
operation.

**Migration pattern:**
```diff
- m_shape = Resource::GetShapeStatic("armyant.shp");
+ m_shape = GetShape("armyant.shp");
```

On headless (server), `g_shapeStore` is `nullptr` — `GetShape()` returns
`nullptr` and constructors get null shapes.  Phase 6.2 adds null guards
to the ~12 simulation-side dereferences.

**Stale include cleanup:** After migration, 50 of 51 `resource.h`
includes in GameLogic were no longer needed (no `Resource::` or
`g_app->m_resource` usage remaining).  Only `entity.cpp` retains
`resource.h` for `g_app->m_resource->GetTextReader("stats.txt")`.

### 6.2 — Null-safety for headless `Shape*` usage

> **STATUS: ❌ Not started.**  Prerequisite: 6.1 (constructors may
> yield `nullptr`).

**Goal:** All simulation-side `m_shape` dereferences handle `nullptr`
gracefully so a headless process doesn't crash.

**Call sites requiring null guards (~12):**

| Call site | Current code | Null-safe replacement |
|-----------|-------------|----------------------|
| `Entity::Begin()` | `m_shape->CalculateCenter/Radius()` | Already has `if (m_shape)` in some paths; add to others |
| `Building::Initialise()` | `m_shape->CalculateCenter/Radius()` | Guard + hardcoded defaults |
| `Building::DoesRayHit()` | `m_shape->RayHit()` | Already has sphere fallback — use it when `m_shape == nullptr` |
| `Building::DoesSphereHit()` | `m_shape->SphereHit()` | Already has sphere fallback |
| `Building::DoesShapeHit()` | `m_shape->ShapeHit()` | Already has sphere fallback |
| `Building::Destroy()` | `m_shape` → `MakeExplosion` | Guard (skip explosion on headless) |
| `EntityLeg::GetLegRootPos()` | `m_parent->m_shape->GetMarkerWorldMatrix()` | Return identity position |
| `ArmyAnt::GetCarryMarker()` | `m_shape->GetMarkerWorldMatrix()` | Return `m_pos` (entity position) |
| `Squadie::Attack()` | `m_shape->GetMarkerWorldMatrix(m_laser)` | Return forward-offset from `m_pos` |
| `Armour::GetEntrance()` | `m_shape->GetMarkerWorldMatrix(m_markerEntrance)` | Return `m_pos` |
| `LaserFence::GetTopPosition()` | `m_shape->GetMarkerWorldMatrix(m_marker2)` | Return `m_pos + up * height` |
| `Entity::RayHit()` | `m_shape->RayHit()` | Sphere fallback (already partially guarded) |

### 6.3 — Decouple `SimEvent::Explosion` from `Shape*`

> **STATUS: ❌ Not started.**  Prerequisite: 6.1 (renderers populate
> `ShapeStore`).

**Problem:** `GameSimEvent::Explosion` currently stores
`const ShapeStatic* shape`.  This is the last pointer from simulation
into presentation data.  On headless, `shape` would be `nullptr`, and
the drain function would crash calling `g_explosionManager.AddExplosion()`.

**Fix:** Replace the pointer with a type identifier:

```cpp
struct Explosion
{
    int entityOrBuildingType;   // Entity::Type or Building::Type
    bool isBuilding;            // disambiguate the two enums
    GameMatrix transform;
    float fraction;
};
```

The drain function in `Starstrike/main.cpp` looks up the shape:

```cpp
[](const GameSimEvent::Explosion& e) {
    ShapeStatic* shape = e.isBuilding
        ? g_buildingShapes.Get(e.entityOrBuildingType)
        : g_entityShapes.Get(e.entityOrBuildingType);
    if (shape)
        g_explosionManager.AddExplosion(shape, e.transform, e.fraction);
},
```

This removes the last `ShapeStatic*` crossing from simulation → client.

### 6.4 — Move `m_shape` ownership to renderer companions (optional)

> **STATUS: ❌ Not started.**  Lower priority — headless works after
> 6.1–6.2.

**Goal:** Remove `ShapeStatic* m_shape` from `Entity` and `Building`
base classes entirely.  Renderers own and manage shape pointers.

**Approach:**

- Each `EntityRenderer` / `BuildingRenderer` subclass holds its own
  `ShapeStatic*` member(s), loaded during `InitGameRenderers()` or
  lazily on first `Render()`.
- `ShapeMarkerData*` fields (`m_carryMarker`, `m_markerEntrance`, etc.)
  either move to the renderer (if rendering-only) or are replaced with
  data-driven `LegacyVector3` offsets (if simulation-visible).
- `EntityLeg` constructor and `GetLegRootPos()` would need the shape
  for IK — either query `ShapeStore` or receive pre-computed offsets.

This is a large refactor touching ~27 files.  It is not required for
headless operation (6.2 null-safety is sufficient) but achieves the
cleanest possible simulation/presentation separation.

### 6.5 — Interpolation buffer (optional, future)

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

> **STATUS: ❌ Not started.**  Requires Phase 5.3 (`NeuronServer` links
> `GameLogic`).

With `GameLogic` fully decoupled:

1. Create `BotClient/BotClient.vcxproj` (executable).
2. Links: `NeuronCore` + `GameLogic` + networking.
3. No `NeuronClient`, no `GameRender`, no window, no GPU.
4. Runs `Location::Advance()` in a loop, sends commands via network.
5. `g_shapeStore` is `nullptr` — entity constructors get `nullptr` shapes;
   null-safety guards (Phase 6.2) ensure no crashes.

---

## Handling `Shape*` in Constructors — The Asset Problem

> **See Phase 6.1 (`ShapeStore`) and Phase 6.2 (null-safety) for the
> implementation plan.**

Currently `ArmyAnt::ArmyAnt()` does:
```cpp
m_shapes[0] = Resource::GetShapeStatic("armyant.shp");
```

The simulation does not need the shape for its core logic, but does use
it for collision bounds (`CalculateCenter`/`CalculateRadius`) and marker
lookups (`GetCarryMarker()`).  The fix:

1. **Phase 6.1** introduces `ShapeStore` (type-indexed cache in
   `GameLogic`, populated by renderers at init).  Constructors change
   from `Resource::GetShapeStatic("armyant.shp")` to
   `g_shapeStore ? g_shapeStore->Get("armyant.shp") : nullptr`.
   `ShapeStatic` stays in `NeuronClient` — the compile-time setup
   (forward declarations in `pch.h`, full include via NeuronClient
   include path) is correct as-is.
2. **Phase 6.2** adds null guards to the ~12 simulation-side `m_shape`
   dereferences so headless processes don't crash.
3. **Phase 6.3** decouples `SimEvent::Explosion` from `ShapeStatic*`
   by replacing the pointer with a type ID.
4. **Phase 6.4** (optional) moves `m_shape` ownership entirely to
   render companions.

---

## Handling `g_app` — The God Global

`g_app` reaches everything: renderer, camera, resource loader, location,
sound, particles.  **Phase 5.1 partially addressed this** by extracting
`GameAppSim` as a base class containing only simulation-relevant data
members.  `g_app` is now typed as `GameAppSim*` (not `GameApp*`), which
prevents simulation code from accidentally accessing `GameMain` (DX12/WinRT)
members at compile time.

The boundary is:

- **Simulation code** may access `g_app->m_location`, `g_app->m_globalWorld`,
  `g_app->m_clientToServer`, and similar simulation-state members (all on
  `GameAppSim`).
- **Simulation code must NOT access** `g_app->m_renderer`, `g_app->m_camera`,
  `g_app->m_resource` (shape/texture loading), `g_app->m_particleSystem`,
  `g_app->m_soundSystem`.  These calls move to render companions or the
  `SimEventQueue`.
- **Client code** that needs `GameMain` methods (e.g. `IsActive()`,
  `IsSuspended()`) uses `static_cast<GameApp*>(g_app)`.

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
| 2 | `GameRender/<Type>Renderer.h/.cpp` | **New** — one per entity/building type (~80 files) | ✅ 14 entity renderers + 43 building renderers |
| 2 | `Starstrike/team.cpp` | Switch from `entity->Render()` to registry dispatch | ✅ |
| 2 | `Starstrike/unit.cpp` | Switch from `entity->Render()` to registry dispatch | ✅ |
| 2 | `Starstrike/location.cpp` | Switch from `building->Render()` to registry dispatch | ✅ |
| 2 | `GameLogic/*.cpp` (migrated entities) | Delete old `Render()` bodies | ✅ All 14 entity types stubbed |
| 2 | `GameLogic/tree_render_interface.h` | **Deleted** — `ITreeRenderBackend` consolidated into `IRenderBackend` | ✅ |
| 2 | `NeuronClient/tree_renderer.h/.cpp` | Changed `ITreeRenderBackend` → `IRenderBackend`; `ReleaseTree` → `ReleaseBuilding` | ✅ |
| 2 | `NeuronClient/opengl_directx.cpp` | Changed `g_treeRenderBackend` → `g_renderBackend` init/shutdown | ✅ |
| 3 | `NeuronCore/SimEventQueue.h` | **New** — generic `SimEventQueue<TEvent, Capacity>` template (reusable across projects) | ✅ |
| 3 | `NeuronCore/Overloaded.h` | **New** — aggregate template for `std::visit` lambda dispatch | ✅ |
| 3 | `GameLogic/GameSimEvent.h` | **New** — `std::variant`-based event struct with 6 alternatives + `Visit()` + factories + `SimParticle`/`SimSoundSource` constants; `sizeof <= 88` enforced by `static_assert` | ✅ |
| 3 | `GameLogic/GameSimEventQueue.h/.cpp` | **New** — typedef `SimEventQueue<GameSimEvent, 1024>` + `g_simEventQueue` global | ✅ |
| 3 | All entity/building `.cpp` with `g_explosionManager`, `m_particleSystem`, `m_soundSystem` | Replace with `g_simEventQueue.Push(SimEvent::Make*(...))` calls; remove stale includes | ✅ |
| 3 | `Starstrike/main.cpp` | Add `DrainSimEvents()` with `std::visit` + `Overloaded` after `Location::Advance()` | ✅ |
| 4 | `GameLogic/worldobject.h` | Remove `virtual Render()` | ✅ |
| 4 | `GameLogic/entity.h` | Remove `virtual Render()`, `Shape* m_shape`, shadow statics | ✅ `Render` override + shadow statics removed; `Shape* m_shape` deferred to Phase 6 |
| 4 | `GameLogic/entity.cpp` | Remove `BeginRenderShadow` / `RenderShadow` / `EndRenderShadow` | ✅ |
| 4 | `GameLogic/building.h` | Remove `virtual Render()` / `RenderAlphas()` / `RenderLights()` / `RenderPorts()`, `Shape* m_shape` | ✅ Virtuals removed; `Shape* m_shape` deferred to Phase 6 |
| 4 | 25 `GameLogic/*.cpp` files | Remove stale rendering includes (`renderer.h` ×16, `text_renderer.h` ×8, `ogl_extensions.h` ×1) | ✅ |
| 4 | 7 building `.cpp` files | Remove dead render helper bodies (`RenderSignal`, `RenderSpirit`, etc.) | ✅ |
| 2 W6 | `GameRender/FlagRenderer.h/.cpp` | **New** — companion for `Flag::Render()` + `RenderText()` | ✅ |
| 2 W6 | `GameRender/SnowRenderer.h/.cpp` | **New** — companion for `Snow::Render()` | ✅ |
| 2 W6 | `GameRender/SpaceInvaderRenderer.h/.cpp` | **New** — companion for `SpaceInvader::Render()` | ✅ |
| 2 W6 | `GameRender/BoxKiteRenderer.h/.cpp` | **New** — companion for `BoxKite::Render()` | ✅ |
| 2 W6 | `GameRender/WeaponRenderer.h/.cpp` | **New** — companion for 7 weapon `Render()` functions | ✅ |
| 2 W6 | `GameRender/WorldObjectRenderer.h` | **New** — base renderer interface for WorldObject subclasses | ✅ |
| 2 W6 | `GameRender/WorldObjectRenderRegistry.h/.cpp` | **New** — type-indexed dispatch for WorldObject effect renderers | ✅ |
| 2 W6 | `GameLogic/flag.cpp`, `snow.cpp`, `airstrike.cpp`, `darwinian.cpp`, `weapons.cpp` | `Render()` bodies + rendering includes removed; no `#if GAMELOGIC_HAS_RENDERER` guards remain | ✅ |
| 2 W6 | `GameLogic/worldobject.h` | `virtual void Render(float)` removed | ✅ |
| 2 W6 | `GameLogic/virii.h` | Dead `Render()` declaration removed | ✅ |
| 2 W6 | `GameLogic/virii.cpp` | Dead 130-line commented-out `RenderLowDetail`/`RenderGlow` block removed | ✅ |
| 2 W6 | `GameLogic/sporegenerator.h` | Dead `RenderTail()` declaration removed | ✅ |
| 5.1 | `GameLogic/GameAppSim.h` | **New** — simulation-side base class for `GameApp`; all data members with DMIs, `GameMode` enum, virtual lifecycle methods, static utilities, `extern GameAppSim* g_app;` | ✅ |
| 5.1 | `Starstrike/GameApp.h` | Rewritten — `class GameApp : public GameAppSim, public GameMain`; only framework overrides, no data members | ✅ |
| 5.1 | `Starstrike/GameApp.cpp` | `GameAppSim* g_app = nullptr;`; member init list removed; 3 static methods `GameAppSim::` qualifier | ✅ |
| 5.1 | `Starstrike/main.cpp` | 4 call sites changed to `static_cast<GameApp*>(g_app)->IsActive()`/`IsSuspended()` | ✅ |
| 5.1 | 59 `GameLogic/*.cpp` files | `#include "GameApp.h"` → `#include "GameAppSim.h"` | ✅ |
| 5.1 | `NeuronCore/rgb_colour.h/.cpp` | **Moved** from `NeuronClient` — pure data type, zero rendering deps | ✅ |
| 5.1 | `NeuronCore/NeuronCore.vcxproj` | Added `rgb_colour.h/.cpp` entries | ✅ |
| 5.1 | `NeuronClient/NeuronClient.vcxproj` | Removed `rgb_colour.h/.cpp` entries | ✅ |
| 5.1 | `GameLogic/GameLogic.vcxproj` | `GAMELOGIC_HAS_RENDERER` removed (Debug + Release); `GameAppSim.h` added to `ClInclude` | ✅ |
| 5.1 | `GameLogic/GameLogicPlatform.h` | **Deleted** — replaced by direct `NeuronCore.h` include in `pch.h` | ✅ |
| 5.1 | `GameLogic/pch.h` | Simplified to `#include "NeuronCore.h"` + 3 forward declarations (`ShapeStatic`, `ShapeFragmentData`, `ShapeMarkerData`) | ✅ |
| 5 post | All `.vcxproj` files | ~~Remove `GAMELOGIC_HAS_RENDERER` define~~ | ✅ Done in 5.1 |
| 5 post | `GameLogic/GameLogicPlatform.h` | ~~**Deleted**~~ | ✅ Done in 5.1 |
| 5 | ~~`GameSim/GameSim.vcxproj`~~ | ~~**New** — pure simulation static lib~~ | N/A — `GameLogic` cleaned in-place |
| 5 | `GameRender/GameRender.vcxproj` | **New** — render companion static lib | ✅ (early) |
| 5 | `Starstrike/Starstrike.vcxproj` | Update link deps | ✅ |
| 5.3 | `NeuronServer/NeuronServer.vcxproj` | Add `GameLogic` `ProjectReference` + `AdditionalIncludeDirectories` (Debug + Release) | ✅ |
| 5 | ~~`GameLogic/GameLogic.vcxproj`~~ | ~~**Deleted**~~ | N/A — `GameLogic` stays |
| 7 | `BotClient/BotClient.vcxproj` | **New** — headless executable | ❌ |
| ~~6.1~~ | ~~`NeuronClient/ShapeStatic.h`~~ | ~~**Moved** to `NeuronCore/ShapeStatic.h`~~ | ❌ Cancelled — `ShapeStatic` stays in `NeuronClient` |
| ~~6.1~~ | ~~`NeuronClient/ShapeStatic.cpp`~~ | ~~**Split** — data→NeuronCore, render→NeuronClient~~ | ❌ Cancelled |
| ~~6.1~~ | ~~`NeuronClient/text_stream_readers.h/.cpp`~~ | ~~**Moved** to `NeuronCore/`~~ | ❌ Cancelled |
| ~~6.1~~ | ~~`NeuronClient/math_utils.h/.cpp`~~ | ~~**Moved** to `NeuronCore/`~~ | ❌ Cancelled |
| ~~6.1~~ | ~~`NeuronCore/NeuronCore.vcxproj`~~ | ~~Add `ShapeStatic.h/.cpp`, `text_stream_readers.h/.cpp`, `math_utils.h/.cpp`~~ | ❌ Cancelled |
| ~~6.1~~ | ~~`NeuronClient/NeuronClient.vcxproj`~~ | ~~Remove moved files; add `ShapeStaticRender.cpp`~~ | ❌ Cancelled |
| ~~6.1~~ | ~~`GameLogic/pch.h`~~ | ~~Replace forward declarations with `#include "ShapeStatic.h"`~~ | ❌ Cancelled — forward declarations stay |
| 6.1 | `GameLogic/ShapeStore.h/.cpp` | **New** — abstract `ShapeStore` interface + `g_shapeStore` global + inline `GetShape()` free function | ✅ |
| 6.1 | `GameRender/ClientShapeStore.h/.cpp` | **New** — concrete `ShapeStore` impl delegating to `Resource::GetShapeStatic()` | ✅ |
| 6.1 | `GameRender/GameRender.cpp` | Added `g_shapeStore = &s_clientShapeStore;` in `InitGameRenderers()` | ✅ |
| 6.1 | 46 `GameLogic/*.cpp` files | `Resource::GetShapeStatic()` → `GetShape()`; added `#include "ShapeStore.h"` | ✅ |
| 6.1 | 50 `GameLogic/*.cpp` files | Removed stale `#include "resource.h"` (only `entity.cpp` retains it) | ✅ |
| 6.2 | ~12 simulation `.cpp` files | Add null guards to `m_shape` dereferences | ❌ |
| 6.3 | `GameLogic/GameSimEvent.h` | `Explosion::shape` → type ID (remove `ShapeStatic*` from variant) | ❌ |
| 6.3 | `Starstrike/main.cpp` | `DrainSimEvents()` looks up shape from renderer cache | ❌ |
| 6.4 | `GameLogic/entity.h` | Remove `ShapeStatic* m_shape` | ❌ |
| 6.4 | `GameLogic/building.h` | Remove `ShapeStatic* m_shape` | ❌ |
| 6.4 | ~27 entity/building `.cpp` files | Move `Shape*` fields and marker fields to renderer companions | ❌ |

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
| `#if GAMELOGIC_HAS_RENDERER` guards in 5 WorldObject subclass files become permanent technical debt if not resolved | ✅ Resolved — Wave 6 extracted all `Render()` bodies into `GameRender/` companions; no guards remain in GameLogic source files.  Macro removed in Phase 5.1 after `GameAppSim` extraction broke the `GameApp.h` → `GameMain` dependency. |

---

## What Stays Coupled (Intentionally)

- **`NeuronCore`** — math, memory, file I/O, timers, logging, strings.
  Both simulation and rendering need these.
- **`LegacyVector3`, `Matrix34`** — math types used everywhere.  These are
  in `NeuronCore`, not rendering.
- **`WorldObjectId`** — pure data, no rendering dependency.
- **`TreeMeshData`, `TreeVertex`** — pure CPU-side data buffers used by
  `Tree` (simulation) and consumed by `TreeRenderer` (rendering).  These
  are GPU-free and belong in `GameLogic`.

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
| 2 — Buildings (Wave 4) | 1–2 weeks | Medium | Wave 1 | ✅ Done (57/57 types registered) |
| 2 — UI/debug (Wave 5) | 2–3 days | Low | Wave 1 | ✅ Done |
| 3 — SimEventQueue (all entity/building types) | 1 week | Medium | Phase 2 in progress | ✅ Done |
| 4 — Remove base-class Render | 1 day | Low | Phase 2 + 3 complete | ✅ Done |
| 2 W6 — WorldObject subclass renderers (Flag, Snow, BoxKite, SpaceInvader, weapons) | 3–5 days | Low | Phase 4 | ✅ Done |
| 5 — GameLogic headless cleanup + GameRender project | 1–2 days | Low | Phase 4 + Wave 6 | ✅ Done (5.1 + 5.2 + 5.3) |
| 6.0 — Audit `Shape*` usage | 1 day | None | Phase 5 | ✅ Done |
| ~~6.1 — Move `ShapeStatic` to NeuronCore~~ | — | — | — | ❌ Cancelled — `ShapeStatic` stays in NeuronClient |
| 6.1 — Shape loading via `ShapeStore` | 2–3 days | Medium | Phase 6.0 | ✅ Done |
| 6.2 — Null-safety for headless `Shape*` | 1 day | Low | Phase 6.1 | ❌ Not started |
| 6.3 — Decouple `SimEvent::Explosion` | 1 day | Low | Phase 6.1 | ❌ Not started |
| 6.4 — Move `m_shape` to renderers (optional) | 3–5 days | Medium | Phase 6.2 | ❌ Not started |
| 6.5 — Interpolation buffer (optional) | 2–3 days | Low | Phase 6.4 | ❌ Not started |
| 7 — Bot client | 1–2 days | Low | Phase 6.2 | ❌ Not started |

Phase 1 alone is enough to start server-side simulation experiments
immediately.  Phases 2–4 can run in parallel with other feature work.

### Recommended Execution Order (Updated)

1. ~~**Phase 0**: Audit + clean `RenderBranch` (0.4) + expand marker audit (0.2)~~ ✅
2. ~~**Phase 1**: PCH split~~ ✅
3. ~~**Phase 2 pre**: `ShadowRenderer`, `BuildingRenderRegistry` (Tree as
   first entry), `EntityRenderRegistry`, `IRenderBackend`~~ ✅
4. ~~**Phase 2 Wave 1**: Trivial entity companions (in `GameRender/`)~~ ✅
5. ~~**Create `LanderRenderer` and `LaserTrooperRenderer`** — complete Waves 2/3~~ ✅
6. ~~**Wave 4 — Building renderers**~~ ✅ — all 57 building types registered
7. ~~**Delete old entity `Render()` bodies**~~ ✅
8. ~~**Migrate `ITreeRenderBackend` → `IRenderBackend`**~~ ✅ —
   `tree_render_interface.h` deleted; single `g_renderBackend` global
9. ~~**Phase 3 pilot**: `Tree::Advance()` side-effects → `SimEventQueue`~~ ✅ —
   `GameSimEvent.h`, `GameSimEventQueue.h/.cpp`,
   `NeuronCore/SimEventQueue.h` created; Tree fully migrated;
   `DrainSimEvents()` in `main.cpp`
10. ~~**Phase 3 — remaining entity/building types + variant conversion**~~ ✅ — all
    simulation-side calls migrated; `std::variant`-based `GameSimEvent`
    with 6 alternatives; `DrainSimEvents()` uses `std::visit` +
    `Overloaded` (`NeuronCore/Overloaded.h`); `sizeof(GameSimEvent) <= 88`;
    vector/matrix fields use `GameVector3`/`GameMatrix`.
    See `docs/SimEvent.md` for full 3-phase history
11. ~~**Phase 2 Wave 5** (UI/debug windows) — move to `GameRender`~~ ✅ —
    all 17 UI/debug window file pairs moved; `OTHER_*` macros extracted
    to `GameLogic/prefs_keys.h`
12. ~~**Phase 4 — Remove base-class `Render()` + stale include cleanup**~~ ✅ —
    Entity shadow statics, all building/entity render declarations/bodies,
    and 25 stale rendering includes removed.  `WorldObject::Render` kept
    (5 files with live `Render()` for non-entity/building subclasses).
    `Shape* m_shape` deferred to Phase 6.
13. ~~**Phase 2 Wave 6 — WorldObject subclass renderers**~~ ✅ Done —
    `FlagRenderer`, `SnowRenderer`, `BoxKiteRenderer`,
    `SpaceInvaderRenderer`, `WeaponRenderer` all in `GameRender/`;
    `WorldObjectRenderer` base + `WorldObjectRenderRegistry`;
    `WorldObject::Render(float)` removed from `worldobject.h`;
    dead declarations (`Virii::Render`, `SporeGenerator::RenderTail`) and
    dead commented-out render code cleaned up.  `GAMELOGIC_HAS_RENDERER`
    removal blocked by `GameApp.h` → `GameMain` dependency (Phase 5).
14. ~~**Phase 5.1 — Headless-clean `GameLogic`**~~ ✅ Done —
    `GameAppSim` base class extracted from `GameApp`; 59 `.cpp` files
    switched to `GameAppSim.h`; `GAMELOGIC_HAS_RENDERER` removed;
    `GameLogicPlatform.h` deleted; `pch.h` simplified; `rgb_colour.h/.cpp`
    moved to NeuronCore.  **GameLogic IS the simulation library** — no
    separate `GameSim` project needed.  Headless compilation proven.
15. ~~**Phase 5.3** — `NeuronServer` links `GameLogic`.~~ ✅ Done —
    `NeuronServer.vcxproj` updated: `ProjectReference` to `GameLogic`,
    `AdditionalIncludeDirectories` added to both Debug and Release
    configs (also fixed pre-existing bug: Release config was missing
    include dirs entirely).
16. ~~**Phase 6.1 — Shape loading via `ShapeStore`**~~ ✅ Done —
    abstract `ShapeStore` in `GameLogic/ShapeStore.h`;
    `ClientShapeStore` in `GameRender/` delegates to
    `Resource::GetShapeStatic()`.  93 call sites across 46 files
    migrated to `GetShape()`.  50 stale `resource.h` includes removed.
    (`ShapeStatic` stays in `NeuronClient` — old Phase 6.1 that
    proposed moving it to NeuronCore was cancelled.)
17. **Phase 6.2 — Null-safety** — add null guards to ~12 `m_shape`
    dereferences for headless operation.
18. **Phase 6.3 — Decouple `SimEvent::Explosion`** — replace
    `const ShapeStatic*` with type ID; drain function looks up shape.
19. **Phase 6.4 — Move `m_shape` ownership to renderers** (optional) —
    remove `ShapeStatic* m_shape` from base classes; renderers own shapes.
20. **Phase 7** — Headless bot client.
