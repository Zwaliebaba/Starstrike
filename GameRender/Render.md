# GameRender — Architecture Analysis & Recommendations

## 1. Current State Summary

The **GameRender** project is a static library that houses all rendering
companions (entity, building, world-object) and the legacy Eclipse-based UI
framework (windows, buttons, menus, scrollbars).  It currently contains
approximately **90+ source files** spanning:

| Category | Files (approx.) | Base Class |
|---|---|---|
| Building renderers | ~42 `.cpp/.h` pairs | `BuildingRenderer` → `DefaultBuildingRenderer` |
| Entity renderers | ~16 `.cpp/.h` pairs | `EntityRenderer` |
| WorldObject renderers | 4 `.cpp/.h` pairs | `WorldObjectRenderer` |
| Standalone helpers | `ShadowRenderer`, `SpiritRenderer` | none (static methods) |
| Registries | 3 (`Entity`, `Building`, `WorldObject`) — 6 files total | — |
| Init | `GameRender.cpp/.h` | — |
| UI framework | ~15 files (`darwinia_window`, `drop_down_menu`, `scrollbar`, `input_field`, etc.) | `EclWindow` / `DarwiniaButton` |
| UI windows | ~10 files (`debugmenu`, `cheat_window`, `mainmenus`, `prefs_*`, `profilewindow`, `network_window`, `reallyquit_window`, `userprofile_window`, `message_dialog`) | `DarwiniaWindow` |

### Render-Pass Structure

Understanding the render loop is essential context for evaluating the
recommendations below.  The main frame renders game objects in this order
(all in `Starstrike/location.cpp` and `Starstrike/team.cpp`):

| Pass | Caller | What it renders | GL state assumed at entry |
|---|---|---|---|
| 1. **Opaque buildings** | `Location::RenderBuildings()` | Iterates `m_buildings`, calls `BuildingRenderer::Render()` | Lighting on, depth write on, blend off |
| 2. **Alpha buildings** | `Location::RenderBuildingAlphas()` | Iterates `m_buildings` (non-depth-sorted first, then depth-sorted back-to-front), calls `BuildingRenderer::RenderAlphas()` | Blend on, depth write off |
| 3. **Virii** | `Team::RenderVirii()` | Looks up `g_entityRenderRegistry.Get(Entity::TypeVirii)` **once**, iterates `m_others` | Texture on, blend additive, depth mask off, cull off |
| 4. **Darwinians** | `Team::RenderDarwinians()` | Looks up `g_entityRenderRegistry.Get(Entity::TypeDarwinian)` **once**, iterates `m_others` | Texture on, blend on |
| 5. **Other entities** | `Team::RenderOthers()` | Iterates `m_others` (skipping Virii/Darwinian), calls `g_entityRenderRegistry.Get(entity->m_type)` per entity | Varies per entity type |
| 6. **Units** | `Unit::Render()` | Iterates `m_entities`, calls `g_entityRenderRegistry.Get(entity->m_type)` per entity | Varies |
| 7. **World-object effects** | *(via m_effects iteration)* | Calls `g_worldObjectRenderRegistry.Get(object->m_type)` | Varies |

Key observations:
- **Passes 3–4 hoist the registry lookup and GL state setup outside the loop** — one setup, thousands of draw calls.  Any per-entity overhead (function call, state query) is multiplied by entity count.
- Each pass resets GL state to a known baseline before starting.  Renderers are **not** required to save/restore state — they set what they need and the pass boundary resets it.
- Call sites use `DEBUG_ASSERT(renderer)` after `Get()` — a null return is treated as a **programming error**, not a runtime condition.

### Strengths

- **Clean companion pattern.**  Rendering is separated from game logic via `EntityRenderer` / `BuildingRenderer` / `WorldObjectRenderer` base classes.  Game types are never modified to add rendering code.
- **Type-indexed dispatch.**  Registries use fixed-size arrays indexed by type enum → O(1) lookup, cache-friendly, no dynamic allocation.
- **Const-correct API.**  Renderers receive `const Entity&` / `const Building&`, enforcing read-only access to game state during draw.
- **Inheritance hierarchy is shallow.**  Most renderers are one level deep.  `DefaultBuildingRenderer` provides a reusable opaque + alpha + lights + ports pass that others can extend via `override`.
- **Single init function.**  `InitGameRenderers()` is the sole entry point — clear ownership and easy to audit.

### Pain Points

1. ~~**`GameRender.cpp` is a 235-line include/registration monolith.**~~ *Resolved by §2.2.*  The monolith has been split into `InitBuildingRenderers.cpp`, `InitEntityRenderers.cpp`, and `InitWorldObjectRenderers.cpp`.  `GameRender.cpp` is now a 15-line orchestrator.
2. **Three near-identical registries.**  `EntityRenderRegistry`, `BuildingRenderRegistry`, and `WorldObjectRenderRegistry` are structurally identical (fixed array + `Register` + `Get`) but are separate classes with separate `.cpp/.h` pairs (6 files total).
3. **Duplicated billboard / quad rendering.**  Camera-aligned billboard quads are hand-rolled with raw `glBegin(GL_QUADS)` in at least 12 renderers (`DarwinianRenderer`, `SpiritRenderer`, `SpawnBuildingRenderer`, `TeleportBuildingRenderer`, `WeaponRenderer`, `MiscEffectRenderer`, `DefaultBuildingRenderer::RenderLights`, `DefaultBuildingRenderer::RenderPorts`, `EscapeRocketBuildingRenderer`, `ShadowRenderer`, etc.).  Some use `glTexCoord2i(0,1)` top-left origin; others use `glTexCoord2f(0,0)` bottom-left — these UV conventions are **not uniform** and must be handled carefully during consolidation.
4. **Duplicated spirit-dot rendering.**  `SpawnBuildingRenderer::RenderSpirit`, `TeleportBuildingRenderer::RenderSpirit`, and `SpiritRenderer::RenderSpirit` all draw the same inner + outer billboard quad pattern with minor colour/alpha variations.
5. ~~**`const_cast` hacks** in `DarwinianRenderer`, `FuelBuildingRenderer`, `SpawnBuildingRenderer`.~~ *Resolved by §2.1.*  The `m_shadowBuildingId` mutation moved to `Darwinian::Advance()`, caching accessors made `const`.  **Note:** 27 `const_cast` calls remain in other renderers (see §2.1 implementation notes); these are separate issues not covered by the original scope.
6. **UI framework mixed with render companions.**  Eclipse windows, buttons, menus, scrollbars, and preference/debug windows live alongside entity/building renderers.  These are two distinct responsibilities with different change frequencies.
7. **`friend` coupling between GameLogic and GameRender.**  `Darwinian` declares `friend class DarwinianRenderer` and `SpawnBuilding` declares `friend class SpawnBuildingRenderer`.  These tie GameLogic to specific GameRender classes at the header level.  Avoid adding more `friend` declarations; prefer exposing data through `const` accessors.

---

## 2. Recommendations

### 2.1 Eliminate `const_cast` Hacks — ✅ COMPLETED

> **Implemented.** All changes listed below have been applied and build successfully (Debug x64).  Runtime verification pending.
>
> **Implementation notes:**
> - Shadow invalidation added to `Darwinian::Advance()` after the state switch, outside the `m_onGround` guard so it runs even for airborne/dead Darwinians.
> - `DarwinianRenderer.cpp` no longer includes `rocket.h` (the `EscapeRocket` cast was the only consumer).
> - `GetLinkedBuilding()` returns `FuelBuilding*` (non-const pointer to a *different* object), not `const FuelBuilding*` — callers in `FuelBuilding::Advance()` and `::Destroy()` need the mutable pointer.
> - 27 other `const_cast` calls remain in GameRender (e.g., `SolarPanelBuildingRenderer`, `SpawnPointBuildingRenderer`, `ResearchItemBuildingRenderer`, `ReceiverBuildingRenderer`, `SoulDestroyerRenderer`).  These are separate patterns (non-const port accessors, `float*` casts, rotation mutations) and were out of scope.

**Problem:** Several renderers violate their own const-correct API:

| File (GameRender/) | Line | `const_cast` target | Severity |
|---|---|---|---|
| `DarwinianRenderer.cpp` | 174 | `const_cast<Darwinian&>(d).m_shadowBuildingId = -1` | **High** — mutates game state during draw |
| `FuelBuildingRenderer.cpp` | 17, 20 | `const_cast<FuelBuilding&>(fuel).GetLinkedBuilding()` / `GetFuelPosition()` | Medium — caching accessor |
| `SpawnBuildingRenderer.cpp` | 45 | `const_cast<SpawnBuilding&>(spawn).GetSpiritLink()` | Medium — caching accessor |

**Root cause analysis:**

- **`DarwinianRenderer.cpp:174`** — The renderer checks `GodDish::m_activated` / `m_timer` and `EscapeRocket::IsSpectacle()`, and if the spectacle is over, sets `m_shadowBuildingId = -1`.  This is a **gameplay state change** executed during rendering, which violates the contract and is frame-rate-dependent.

- **`FuelBuilding::GetFuelPosition()` (`GameLogic/rocket.cpp:41–51`)** — This is a caching accessor.  It lazily initializes `m_fuelMarker` via `m_shape->GetMarkerData("MarkerFuel")` on first call, then computes a world-space position from it.  The mutation is a **cache fill**, not a logical state change.

- **`FuelBuilding::GetLinkedBuilding()` (`GameLogic/rocket.cpp:64–78`)** — This is a **pure read** — it looks up `m_fuelLink` in the building list and returns a pointer.  No mutation at all; it simply needs a `const` qualifier on the method.

- **`SpawnBuilding::GetSpiritLink()` (`GameLogic/spawnpoint.cpp:129–139`)** — Same caching pattern as `GetFuelPosition()`: lazily fills `m_spiritLink` via `GetMarkerData()`.

**Fixes:**

1. **`DarwinianRenderer`** — Move the `m_shadowBuildingId` invalidation into `Darwinian::Advance()`.  Add equivalent checks there:
   - If `m_shadowBuildingId != -1`, look up the building.
   - If the building is a `GodDish` and `!dish->m_activated && dish->m_timer < 1.0f`, set `m_shadowBuildingId = -1`.
   - If the building is an `EscapeRocket` and `!rocket->IsSpectacle()`, set `m_shadowBuildingId = -1`.
   - The renderer then simply reads `m_shadowBuildingId`; if it's `-1`, skip the shadow.  Remove the `const_cast` entirely.

2. **`FuelBuilding::GetFuelPosition()` and `SpawnBuilding::GetSpiritLink()`** — Mark the cached marker pointer (`m_fuelMarker`, `m_spiritLink`) as `mutable` and make the methods `const`.  This is semantically correct: the pointer is a lazily-filled cache of an immutable shape lookup, not logical state.

3. **`FuelBuilding::GetLinkedBuilding()`** — Add `const` to the method signature (no `mutable` needed).  Return `const FuelBuilding*`.

**Files changed:**

| File | Project | Action |
|---|---|---|
| `GameLogic/darwinian.cpp` | GameLogic | Add shadow invalidation to `Darwinian::Advance()` |
| `GameLogic/darwinian.h` | GameLogic | No change (field already accessible to Advance) |
| `GameRender/DarwinianRenderer.cpp` | GameRender | Remove `const_cast`; read-only check of `m_shadowBuildingId` |
| `GameLogic/rocket.h` | GameLogic | Mark `m_fuelMarker` as `mutable`; make `GetFuelPosition() const`, `GetLinkedBuilding() const` |
| `GameLogic/rocket.cpp` | GameLogic | Update method signatures |
| `GameLogic/spawnpoint.h` | GameLogic | Mark `m_spiritLink` as `mutable`; make `GetSpiritLink() const` |
| `GameLogic/spawnpoint.cpp` | GameLogic | Update method signature |
| `GameRender/FuelBuildingRenderer.cpp` | GameRender | Remove all `const_cast` calls |
| `GameRender/SpawnBuildingRenderer.cpp` | GameRender | Remove `const_cast` call |

**Priority:** High — the `DarwinianRenderer` mutation is a correctness bug; the others are code-smell that blocks further const-correctness.

---

### 2.2 Split `GameRender.cpp` into Per-Category Init Files — ✅ COMPLETED

> **Implemented.** Three new files created, `GameRender.cpp` reduced to a 15-line orchestrator, all added to `GameRender.vcxproj`.  Build successful (Debug x64).
>
> **Files created:**
> - `GameRender/InitBuildingRenderers.cpp` — 42 static instances, 57 `Register()` calls
> - `GameRender/InitEntityRenderers.cpp` — 17 static instances, 17 `Register()` calls
> - `GameRender/InitWorldObjectRenderers.cpp` — 4 static instances, 14 `Register()` calls

**Problem:** `GameRender.cpp` includes 60+ renderer headers.

**Proposal:** Split into three files by category:

| New file (GameRender/) | Registers | Approx. includes |
|---|---|---|
| `InitBuildingRenderers.cpp` | All `g_buildingRenderRegistry.Register(...)` calls + their `static` instances | ~35 building renderer headers |
| `InitEntityRenderers.cpp` | All `g_entityRenderRegistry.Register(...)` calls + their `static` instances | ~16 entity renderer headers |
| `InitWorldObjectRenderers.cpp` | All `g_worldObjectRenderRegistry.Register(...)` calls + their `static` instances | ~5 world-object renderer headers |

`GameRender.cpp` becomes:

```cpp
#include "pch.h"
#include "GameRender.h"

// Defined in per-category init files.
void InitBuildingRenderers();
void InitEntityRenderers();
void InitWorldObjectRenderers();

void InitGameRenderers()
{
    InitBuildingRenderers();
    InitEntityRenderers();
    InitWorldObjectRenderers();
}
```

**Impact:** A change to any single renderer header now only recompiles its category init file (~1/3 of the fan-out).  Adding a new renderer only touches one of the three files, reducing merge conflicts.  No macro infrastructure, no static-init-order risks.

**All new `.cpp` files must `#include "pch.h"` first.**  Add them to `GameRender.vcxproj`.

**Priority:** Medium — simple mechanical split, immediate build-time improvement.

---

### 2.3 Extract a Billboard / Quad Helper

**Problem:** Camera-aligned billboard quads are hand-rolled with `glBegin(GL_QUADS)` in 12+ places.

**Important constraint:** Passes 3 and 4 (Virii, Darwinians) set up GL state **once** outside the loop and draw thousands of entities.  The helper **must not** bind textures, set colour, or call `glEnable`/`glDisable` per quad — that would be a frame-time regression.

**Proposal:** Stateless helpers that **only emit vertices**, leaving GL state management to the caller:

```cpp
// GameRender/BillboardHelper.h  (new file, GameRender project)
#pragma once
#include "LegacyVector3.h"

// Stateless helpers for common billboard vertex patterns.
// All methods assume the caller has already set up GL state (blend,
// texture, colour) outside the loop.  These only emit glVertex/glTexCoord.
//
// UV convention: bottom-left origin (0,0) = bottom-left, (1,1) = top-right.
// Callers using top-left origin (e.g., DarwinianRenderer) must pass
// _flipV = true.
namespace BillboardHelper
{
    // Camera-aligned quad.  Caller must have called glBegin(GL_QUADS)
    // or call this between its own glBegin/glEnd pair.
    void EmitQuad(const LegacyVector3& _pos, float _halfW, float _halfH,
                  bool _flipV = false);

    // Overload: uniform size.
    void EmitQuad(const LegacyVector3& _pos, float _halfSize,
                  bool _flipV = false);

    // Beam / ribbon quad between two points, facing the camera.
    // Emits 4 vertices with tex coords.
    void EmitBeam(const LegacyVector3& _from, const LegacyVector3& _to,
                  float _halfWidth);
}
```

The key design decisions:
- **No colour / texture / state calls inside the helper.**  The caller sets `glColor4*`, `glBindTexture`, and `glEnable(GL_BLEND)` once; the helper only emits geometry.
- **UV flip flag** handles the two conventions found in the codebase (bottom-left vs. top-left origin).  Each migration must check the original `glTexCoord` calls and set the flag correctly.  Always verify visually.
- `EmitQuad` reads camera axes from `g_context->m_camera` internally (same as every existing call site).

**Migration strategy:** Convert one renderer at a time.  Visually verify each conversion in-game before moving to the next.  Start with `SpiritRenderer` (simplest) as a proof-of-concept.

**Files changed:**

| File | Project | Action |
|---|---|---|
| `GameRender/BillboardHelper.h` | GameRender | New file |
| `GameRender/BillboardHelper.cpp` | GameRender | New file; `#include "pch.h"` first; add to `GameRender.vcxproj` |

**Priority:** Medium — high duplication reduction, low risk if migrated incrementally.

---

### 2.4 Consolidate Spirit Rendering

**Depends on:** 2.3 (BillboardHelper).  Implement after `BillboardHelper` exists so `RenderDot` can use it internally.

**Problem:** Three independent spirit-dot implementations:

| Location | Colour source | Inner/outer sizes |
|---|---|---|
| `GameRender/SpiritRenderer.cpp` | Team colour via `Spirit::m_teamId` | 1.0 / 3.0 (scaled by birth/death state) |
| `GameRender/SpawnBuildingRenderer.cpp` (private static) | Hardcoded `(150, 50, 25)` | 2 / 6 |
| `GameRender/TeleportBuildingRenderer.cpp` (protected method) | Team colour via `_teamId` param | 2 / 6 |

**Proposal:** Unify into `SpiritRenderer`:

```cpp
// GameRender/SpiritRenderer.h
#pragma once
#include "LegacyVector3.h"
#include "rgb_colour.h"

class Spirit;

class SpiritRenderer
{
  public:
    // Full spirit with birth/death fade (existing).
    static void Render(const Spirit& _spirit, float _predictionTime);

    // Simple inner + outer billboard dot.  Caller manages GL state.
    static void RenderDot(const LegacyVector3& _pos,
                          const RGBAColour& _colour,
                          int _innerAlpha = 255, int _outerAlpha = 100,
                          float _innerSize = 2.0f, float _outerSize = 6.0f);
};
```

`SpawnBuildingRenderer` calls `SpiritRenderer::RenderDot(position, RGBAColour(150, 50, 25))`.
`TeleportBuildingRenderer` calls `SpiritRenderer::RenderDot(pos, teamColour, 100, 50)`.

**Files changed:**

| File | Project | Action |
|---|---|---|
| `GameRender/SpiritRenderer.h` | GameRender | Add `RenderDot` declaration |
| `GameRender/SpiritRenderer.cpp` | GameRender | Implement `RenderDot` using `BillboardHelper::EmitQuad` |
| `GameRender/SpawnBuildingRenderer.cpp` | GameRender | Remove private `RenderSpirit`; call `SpiritRenderer::RenderDot` |
| `GameRender/SpawnBuildingRenderer.h` | GameRender | Remove `RenderSpirit` declaration |
| `GameRender/TeleportBuildingRenderer.cpp` | GameRender | Remove `RenderSpirit`; call `SpiritRenderer::RenderDot` |
| `GameRender/TeleportBuildingRenderer.h` | GameRender | Remove `RenderSpirit` declaration |

**Priority:** Low–Medium — depends on 2.3; small change once the helper exists.

---

### 2.5 Templatize the Three Registries

**Problem:** Three classes are structurally identical (6 files, same logic).

**Proposal:** Replace with a single header-only class template:

```cpp
// GameRender/RenderRegistry.h  (new file)
#pragma once

template <typename TRenderer, int MaxTypes>
class RenderRegistry
{
  public:
    void Register(int _type, TRenderer* _renderer)
    {
        DEBUG_ASSERT(_type >= 0 && _type < MaxTypes);
        m_renderers[_type] = _renderer;
    }

    [[nodiscard]] TRenderer* Get(int _type) const
    {
        if (_type < 0 || _type >= MaxTypes)
            return nullptr;
        return m_renderers[_type];
    }

  private:
    TRenderer* m_renderers[MaxTypes] = {};
};
```

Each existing registry header becomes a typedef + extern:

```cpp
// GameRender/EntityRenderRegistry.h
#pragma once
#include "RenderRegistry.h"
#include "entity.h"       // for Entity::NumEntityTypes
class EntityRenderer;
using EntityRenderRegistry = RenderRegistry<EntityRenderer, Entity::NumEntityTypes>;
extern EntityRenderRegistry g_entityRenderRegistry;
```

**Note:** `RenderRegistry.h` must **not** include `entity.h`, `building.h`, or `worldobject.h`.  The max-types constant is passed as a template argument; the heavy GameLogic headers stay in the per-registry typedef headers only.

**Impact:** Eliminates the 3 `.cpp` files (`EntityRenderRegistry.cpp`, `BuildingRenderRegistry.cpp`, `WorldObjectRenderRegistry.cpp`).  The typedef headers still exist (3 files), but the logic is written once.  The global definitions (`EntityRenderRegistry g_entityRenderRegistry;`) move into the category init `.cpp` files from 2.2.

**Priority:** Low — mechanical refactor, no functional change.

---

### 2.6 Document GL State Contracts Per Render Pass

**Problem:** GL state setup/teardown is copy-pasted across renderers.  The original plan proposed an RAII `GLStateScope` that saves/restores state via `glGet*` calls.

**Why RAII save/restore is wrong here:** `glGet*` / `glIsEnabled` calls are expensive on many GL drivers — they force a CPU–GPU sync.  In the Virii and Darwinian passes (thousands of entities), adding `glGet*` per entity would be a measurable frame-time regression.  Furthermore, the codebase **does not use save/restore** — each pass starts from a known state and renderers set what they need.

**Proposal instead:** Add a **GL state contract comment block** at the top of each render-pass function in `Starstrike/location.cpp` and `Starstrike/team.cpp`, documenting the expected state at entry and exit.  Example:

```cpp
// GL State Contract:
//   Entry: GL_BLEND on, blendFunc(SRC_ALPHA, ONE), GL_TEXTURE_2D on,
//          GL_DEPTH_TEST on, depthMask off, GL_CULL_FACE off.
//   Exit:  same as entry (pass boundary resets state).
//   Renderers may change blend func / texture binding freely.
void Team::RenderVirii(float _predictionTime)
{
    ...
}
```

And in each renderer, add a brief comment noting what state it modifies:

```cpp
void SomeRenderer::Render(const Entity& _entity, const EntityRenderContext& _ctx)
{
    // Modifies: glColor4f, glBlendFunc (restores before return).
    ...
}
```

**Future option:** If GL state bugs become frequent, introduce a **shadow state tracker** — a lightweight struct that mirrors GL state in CPU memory (updated alongside `gl*` calls, never querying the driver).  This is a larger undertaking and only justified if state bugs are actually occurring.

**Priority:** Low — documentation-only change.  Deferred until state bugs motivate stronger tooling.

---

### 2.7 Add VS Project Filters for UI vs. Renderers

**Problem:** Eclipse UI files (`darwinia_window`, `drop_down_menu`, `scrollbar`, `input_field`, and all `*_window` files) live alongside entity/building renderers in the same flat project.

**Important:** Moving files to subdirectories would break `#include "darwinia_window.h"` across the entire solution — high risk for no functional benefit.

**Proposal:** Add filter groups in `GameRender.vcxproj.filters` only:

```xml
<Filter Include="Renderers\Building" />
<Filter Include="Renderers\Entity" />
<Filter Include="Renderers\WorldObject" />
<Filter Include="Renderers\Helpers" />
<Filter Include="UI\Framework" />
<Filter Include="UI\Windows" />
```

No files are moved on disk.  Include paths don't change.  Solution Explorer becomes navigable.

**Priority:** Low — organizational improvement only.

---

## 3. Recommended Execution Order

| Phase | Task | Risk | Effort | Impact | Status |
|---|---|---|---|---|---|
| ~~1~~ | ~~**2.1** — Eliminate `const_cast` hacks~~ | ~~Low~~ | ~~Small~~ | ~~**High** (correctness)~~ | ✅ Done |
| ~~2~~ | ~~**2.2** — Split `GameRender.cpp` into 3 init files~~ | ~~Very Low~~ | ~~Small~~ | ~~Medium (build time)~~ | ✅ Done |
| 3 | **2.3** — Extract `BillboardHelper` | Low | Medium | Medium (code quality) | Pending |
| 4 | **2.4** — Consolidate spirit rendering (uses 2.3) | Low | Small | Low–Medium | Pending |
| 5 | **2.5** — Templatize registries | Very Low | Small | Low (DRY) | Pending |
| 6 | **2.6** — Document GL state contracts | Very Low | Small | Low (documentation) | Pending |
| 7 | **2.7** — VS project filters | Very Low | Trivial | Low (organization) | Pending |

---

## 4. Build & Verification Strategy

**Configs to build after each phase:** Debug x64, Release x64.

| Phase | Build type | Runtime verification |
|---|---|---|
| 1 (const_cast) | Full rebuild of GameLogic + GameRender + Starstrike (headers changed) | Play a level with Darwinians near a GodDish and EscapeRocket — verify shadows appear and disappear correctly.  Verify FuelBuilding pipe rendering.  Verify SpawnBuilding spirit-link rendering. |
| 2 (split init) | Full rebuild of GameRender only | Launch game, enter any level — all buildings, entities, and effects must render identically to before. |
| 3 (BillboardHelper) | Incremental (new files + modified renderers) | Migrate one renderer at a time.  After each, visually compare in-game against a reference screenshot.  Pay special attention to UV orientation (flipped textures are the most likely regression). |
| 4 (spirit consolidation) | Incremental (modified renderers) | Verify spirit dots at SpawnBuildings, Teleports, and free-floating spirits all render with correct colour and size. |
| 5 (template registry) | Full rebuild of GameRender + Starstrike | Same as phase 2 — all renderers must dispatch correctly. |
| 6–7 (docs/filters) | No code change / project-file-only change | Solution Explorer shows correct grouping. |

**Performance check (phases 3–4):**  Darwinians and Virii are the highest-count entities (thousands per level).  After migrating their renderers to use `BillboardHelper`, run a stress level and confirm frame time has not regressed by more than 0.1 ms.  Use the in-game `ProfileWindow` or an external GPU profiler.

---

## 5. Risks & Mitigations

| Risk | Affected Phase | Mitigation |
|---|---|---|
| **`Darwinian::Advance()` doesn't currently look up buildings for shadow invalidation** — the check may not have access to the building's timer/state. | 1 | Verify that `Location::GetBuilding()` is callable from `Advance()`.  It already is (used in other `Advance*` methods).  The additional building lookup per Darwinian per tick is negligible — `m_shadowBuildingId` is only set for Darwinians watching a spectacle (small subset). |
| **UV convention mismatch in BillboardHelper** — some call sites use top-left origin, others use bottom-left. | 3 | The `_flipV` parameter handles this explicitly.  Each migration must check the original `glTexCoord` calls and set the flag correctly.  Always verify visually. |
| **`GetFuelPosition` / `GetSpiritLink` lazy init is not thread-safe** after marking `mutable`. | 1 | Not a concern — `Advance()` and `Render()` run on the same thread in this engine.  No concurrent access to these fields. |
| **`WeaponRenderer::RenderLaser` is a static method called directly from the laser render loop**, not via the registry. | — | Leave `WeaponRenderer` and `MiscEffectRenderer` as-is.  Their internal switch dispatch is intentional — weapons share common GL state setup.  Splitting them would duplicate setup code and add 8 new files + includes to the init file, contradicting goal 2.2. |

---

## 6. Success Criteria

- [x] `const_cast` calls eliminated from `DarwinianRenderer`, `FuelBuildingRenderer`, `SpawnBuildingRenderer` (§2.1 scope)
- [ ] Zero `const_cast` calls remain in *all* GameRender `.cpp` files (27 remain in other renderers — future work)
- [x] `m_shadowBuildingId` invalidation occurs in `Darwinian::Advance()`, not in `DarwinianRenderer::Render()`
- [x] `GetFuelPosition()`, `GetLinkedBuilding()`, and `GetSpiritLink()` are `const`-qualified
- [x] `GameRender.cpp` contains no `#include` of individual renderer headers
- [ ] `BillboardHelper` is used by at least `SpiritRenderer`, `ShadowRenderer`, and `DefaultBuildingRenderer::RenderLights`
- [ ] Only one implementation of spirit-dot rendering exists (`SpiritRenderer::RenderDot`)
- [ ] All three registries use a single `RenderRegistry<T, N>` template
- [x] Debug x64 builds cleanly with zero new warnings (phases 1–2)
- [ ] Release x64 build verified
- [ ] Visual rendering is identical before and after (screenshot comparison on 3 representative levels)
- [ ] Frame time on a stress level does not regress by more than 0.1 ms

---

## 7. Out of Scope (Noted for Future)

- **Batched quad rendering.**  `BillboardHelper` (2.3) is a stepping stone toward collecting all billboard quads into a single vertex buffer and issuing one draw call per texture.  This is a significant perf win for scenes with thousands of spirits/Darwinians but requires a broader rendering pipeline change.
- **GL-to-DX12 migration.**  All renderers use raw `gl*` calls.  The billboard helper and documented state contracts create natural abstraction seams for a future API migration.
- **Data-driven renderer registration.**  Instead of code-based `Register()` calls, renderers could be declared in a data file.  This is premature until the renderer count stabilizes.
- **Removing `friend` declarations.**  `Darwinian` and `SpawnBuilding` declare `friend class` for their renderers.  Long-term, exposing needed data through `const` public accessors would decouple GameLogic from GameRender at the class level.  This is a gradual migration — do not add new `friend` declarations.
- **Shadow state tracker.**  If GL state bugs become frequent, a CPU-side struct mirroring GL state (updated alongside `gl*` calls, never calling `glGet*`) could catch mismatches in debug builds.  Only justified if state bugs are actually occurring.
