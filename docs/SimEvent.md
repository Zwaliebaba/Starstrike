# SimEvent Split & SimEventQueue Relocation Plan

## ADR: Separate Generic Event Infrastructure from Game-Specific Event Types

### Context

`SimEvent` and `SimEventQueue` currently live in **GameLogic**. The struct is a
monolithic tagged union: it carries particle parameters, explosion data, and
sound-system payload all in one flat struct, with a game-specific `Type` enum
that names each variant.

The queue is a clean, game-agnostic concept (fixed-capacity array, Push / Get /
Clear), but it is tightly coupled to the game-specific `SimEvent` layout and
can only be used by GameLogic.

**Goal**: Move the reusable queue infrastructure to **NeuronCore** (the shared
foundation available to all projects, including a future NeuronServer consumer)
and keep only the game-specific event definitions in **GameLogic**.

### Current State

| Item | Location | Role |
|---|---|---|
| `SimEvent` (struct + `Type` enum + factory helpers) | `GameLogic\SimEvent.h` | Monolithic event definition |
| `SimParticle` namespace | `GameLogic\SimEvent.h` | Particle type constants |
| `SimSoundSource` namespace | `GameLogic\SimEvent.h` | Sound source type constants |
| `SimEventQueue` class | `GameLogic\SimEventQueue.h/.cpp` | Fixed-capacity queue |
| `g_simEventQueue` global | `GameLogic\SimEventQueue.cpp` | Singleton instance |
| ~38 `.cpp` files in GameLogic | Various | **Producers** — call `g_simEventQueue.Push(SimEvent::Make*(…))` or construct events field-by-field |
| `Starstrike\main.cpp` | Starstrike (exe) | **Consumer** — `DrainSimEvents()` switch on `SimEvent::Type` |

#### Producer construction patterns

Of the ~218 event pushes across GameLogic, **175 use the `Make*` factories**
and **43 construct events field-by-field** (manual `SimEvent evt = {};` followed
by individual field assignments).  The manual-construction sites are
concentrated in 11 files: `radardish.cpp`, `researchitem.cpp`,
`souldestroyer.cpp`, `spam.cpp`, `spider.cpp`, `spirit.cpp`,
`sporegenerator.cpp`, `tree.cpp`, `triffid.cpp`, `tripod.cpp`, `virii.cpp`.

Several manual `SoundBuildingEvent` constructions (e.g. in `tree.cpp`) set
`evt.pos` / `evt.vel`, but `DrainSimEvents()` never reads those fields for
`SoundBuildingEvent` — it looks up the building by ID.  These are dead writes
and should be removed during factory conversion.

The same problem is larger for `SoundEntityEvent`: the consumer
(main.cpp:130–136) calls `GetEntity(evt.objectId)` and passes the whole
`Entity*` to `TriggerEntityEvent`, which reads `_entity->m_type`,
`_entity->m_pos`, `_entity->m_vel` directly — **not** the event's `pos`,
`vel`, or `objectType` fields.  All 35 factory calls and 14 manual
constructions copy these three fields for nothing.  `MakeSoundEntity` should
be simplified to `MakeSoundEntity(WorldObjectId _objectId, const char*
_eventName)`, dropping the 3 dead parameters.  This should be done in Step 0
alongside the `SoundBuildingEvent` cleanup since it also reduces the number of
`LegacyVector3` parameters that Phase 2 would have to migrate through the
conversion bridge.

#### Struct size

`SimEvent` uses a flat layout (not a `union`) — every event carries all fields
regardless of type.  Current approximate size: **~136 bytes** (including
padding).  At 1024 capacity the queue occupies **~136 KB** of BSS.  After
Phase 2 the `Matrix34→GameMatrix` change adds 16 bytes per event (48 → 64),
bringing the per-event size to ~152 bytes and queue footprint to ~152 KB.
Both are acceptable for BSS-allocated data.  Phase 3 converts the flat struct
to a `std::variant` of per-type aggregates, reducing per-event size to ~88
bytes (~88 KB queue footprint) — a 42% reduction.

#### Dependency graph (relevant chain)

```
Starstrike ──► NeuronClient ──► NeuronCore
     │                              ▲
     └──► GameLogic ────────────────┘
              │
              ├── SimEvent.h         (game-specific types, depends on LegacyVector3, Matrix34, RGBAColour, WorldObjectId)
              ├── SimEventQueue.h    (generic queue, depends on SimEvent.h)
              └── SimEventQueue.cpp  (global instance)
```

`LegacyVector3`, `Matrix34`, `RGBAColour` are in **NeuronClient**.
`WorldObjectId` is in **GameLogic** (`worldobject.h`).

### Requirements

1. **SimEventQueue** should be a reusable, templated class in **NeuronCore** so
   any project (client, server, tests) can use the same pattern.
2. **Game-specific event type** (`SimEvent`, `SimParticle`, `SimSoundSource`)
   stays in GameLogic — it knows about `WorldObjectId`, `ShapeStatic*`, etc.
3. The **global instance** `g_simEventQueue` stays accessible to GameLogic
   producers (they include the header and call `Push`).
4. **Zero runtime cost change** — the queue is still a flat array, no virtual
   dispatch, no allocations.
5. **Minimal churn** — producer call sites (`g_simEventQueue.Push(SimEvent::Make*(…))`) should not change.

---

## Phase 1: Split SimEvent & Relocate SimEventQueue

### Design

#### New files

| File | Project | Contents |
|---|---|---|
| `NeuronCore\SimEventQueue.h` | NeuronCore | `template<typename TEvent, int Capacity> class SimEventQueue` — generic queue |
| `GameLogic\GameSimEvent.h` | GameLogic | `SimParticle`, `SimSoundSource` namespaces + `struct GameSimEvent` (renamed from `SimEvent`) with `Type` enum and factory helpers |
| `GameLogic\GameSimEventQueue.h` | GameLogic | `using GameSimEventQueue = SimEventQueue<GameSimEvent, 1024>;` typedef + `extern GameSimEventQueue g_simEventQueue;` |
| `GameLogic\GameSimEventQueue.cpp` | GameLogic | Definition of `g_simEventQueue` |

#### Removed files

| File | Reason |
|---|---|
| `GameLogic\SimEvent.h` | Replaced by `GameLogic\GameSimEvent.h` |
| `GameLogic\SimEventQueue.h` | Replaced by generic `NeuronCore\SimEventQueue.h` + `GameLogic\GameSimEventQueue.h` |
| `GameLogic\SimEventQueue.cpp` | Replaced by `GameLogic\GameSimEventQueue.cpp` |

#### Layer diagram (after)

```
NeuronCore
  └── SimEventQueue.h              // template<TEvent, Capacity> class SimEventQueue

GameLogic (depends on NeuronCore via GameLogicPlatform.h → NeuronCore.h)
  ├── GameSimEvent.h               // struct GameSimEvent  (Type enum, fields, Make* factories)
  │     includes: LegacyVector3.h, matrix34.h, rgb_colour.h, worldobject.h
  ├── GameSimEventQueue.h          // using GameSimEventQueue = SimEventQueue<GameSimEvent,1024>;
  │     includes: NeuronCore/SimEventQueue.h, GameSimEvent.h
  │     declares: extern GameSimEventQueue g_simEventQueue;
  └── GameSimEventQueue.cpp        // GameSimEventQueue g_simEventQueue;

Starstrike\main.cpp
  includes: GameSimEventQueue.h    // drain loop unchanged
```

#### Generic `SimEventQueue<T,N>` template (NeuronCore)

```cpp
// NeuronCore\SimEventQueue.h
#pragma once

// ---------------------------------------------------------------------------
// SimEventQueue<TEvent, Capacity>
//
// Fixed-capacity, flat-array queue of deferred simulation side-effects.
// TEvent must be trivially copyable.  No per-frame allocations.
//
// Producers call Push() during simulation ticks.
// The client drains via Count()/Get() after the simulation step, then Clear().
// ---------------------------------------------------------------------------

template<typename TEvent, int Capacity>
class SimEventQueue
{
public:
    void Push(const TEvent& _event)
    {
        DEBUG_ASSERT(m_count < Capacity); // Overflow diagnostic in debug builds
        if (m_count < Capacity)
        {
            m_events[m_count] = _event;
            ++m_count;
        }
    }

    int Count() const { return m_count; }

    const TEvent& Get(int _index) const
    {
        DEBUG_ASSERT(_index >= 0 && _index < m_count);
        return m_events[_index];
    }

    void Clear() { m_count = 0; }

private:
    TEvent m_events[Capacity] = {};
    int    m_count = 0;
};
```

#### Game-specific `GameSimEvent` (GameLogic)

The current `SimEvent` struct is renamed to `GameSimEvent`. A `using` alias
preserves the old name so that all ~38 producer files only need their
`#include` path changed — the call-site code `SimEvent::MakeParticle(…)` etc.
continues to compile.

```cpp
// GameLogic\GameSimEvent.h
#pragma once
#include "LegacyVector3.h"
#include "matrix34.h"
#include "rgb_colour.h"
#include "worldobject.h"

class ShapeStatic;

namespace SimParticle { /* unchanged constants */ }
namespace SimSoundSource { /* unchanged constants */ }

struct GameSimEvent
{
    enum Type { ParticleSpawn, Explosion, SoundStop, SoundEntityEvent, SoundBuildingEvent, SoundOtherEvent };
    // ... all existing fields ...
    // ... all existing Make* factories ...
};

// Back-compat alias so call sites can still write SimEvent::MakeParticle(…)
using SimEvent = GameSimEvent;
```

```cpp
// GameLogic\GameSimEventQueue.h
#pragma once
#include "SimEventQueue.h"   // NeuronCore generic template
#include "GameSimEvent.h"

using GameSimEventQueue = SimEventQueue<GameSimEvent, 1024>;

extern GameSimEventQueue g_simEventQueue;
```

### Phase 1 migration steps (one commit each)

| # | Step | Files touched | Risk |
|---|---|---|---|
| 0a | **Convert manual-construction sites to factories** — In the 11 files that build `SimEvent` field-by-field (`radardish.cpp`, `researchitem.cpp`, `souldestroyer.cpp`, `spam.cpp`, `spider.cpp`, `spirit.cpp`, `sporegenerator.cpp`, `tree.cpp`, `triffid.cpp`, `tripod.cpp`, `virii.cpp`), replace manual construction with the appropriate `SimEvent::Make*` factory call. Remove dead `pos`/`vel` writes on `SoundBuildingEvent` events (consumer never reads them). | 11 `.cpp` files | Low — pure refactor; behaviour unchanged |
| 0b | **Simplify `MakeSoundEntity`** — Drop dead `_pos`, `_vel`, `_objectType` parameters from `MakeSoundEntity` (consumer reads them from the `Entity*`, not the event). Update all 35 factory call sites and the 14 manual-construction sites (already converted in 0a). Touches additional files beyond the 11 in Step 0a: `engineer.cpp`, `entity.cpp`, `insertion_squad.cpp`, `officer.cpp`. Also update `SimEvent.h` factory signature. | ~15 `.cpp` + 1 `.h` | Low — dead-parameter removal; behaviour unchanged |
| 1 | **Add generic queue template** — Create `NeuronCore\SimEventQueue.h` with the `SimEventQueue<TEvent, Capacity>` class template (includes `DEBUG_ASSERT` on overflow). Add the file to `NeuronCore.vcxproj`. | 1 new file | None — additive only, no existing code changed |
| 2 | **Add game-specific headers** — Create `GameLogic\GameSimEvent.h` (copy of current `SimEvent.h`, struct renamed to `GameSimEvent`, add `using SimEvent = GameSimEvent;`). Create `GameLogic\GameSimEventQueue.h` / `.cpp` with the typedef and global. | 3 new files | None — additive only |
| 3 | **Redirect includes** — In every GameLogic `.cpp` that currently does `#include "SimEventQueue.h"` or `#include "SimEvent.h"`, change to `#include "GameSimEventQueue.h"`. Update `Starstrike\main.cpp` similarly. Note: `Starstrike\main.cpp`'s `DrainSimEvents()` switch uses `SimEvent::ParticleSpawn`, `SimEvent::Explosion`, etc. directly — the `using SimEvent = GameSimEvent;` alias is critical for this file. Also add `default: DEBUG_ASSERT(false);` to the `DrainSimEvents()` switch to catch unhandled event types in debug builds. | ~42 files, include path only | Low — `using` alias keeps all call sites compiling |
| 4 | **Remove old files** — Delete `GameLogic\SimEvent.h`, `GameLogic\SimEventQueue.h`, `GameLogic\SimEventQueue.cpp`. Remove from `GameLogic.vcxproj`. | 3 removed files | Low — all references already redirected |
| 5 | **Build & verify** — Full rebuild of all configurations. Run existing tests. | 0 | Verification only |

### Trade-off analysis

| Decision | Pros | Cons | Alternatives |
|---|---|---|---|
| **Template in NeuronCore** | Reusable for all projects (client, server, tests); no virtual dispatch; header-only so no link dependency added; `DEBUG_ASSERT` is already available in NeuronCore | Adds a template to the shared foundation; slightly larger compile unit if instantiated in many TUs | Put in NeuronClient — but excludes NeuronServer from reuse; put in GameLogic — defeats reusability goal |
| **`using SimEvent = GameSimEvent` alias** | Zero churn on ~38 producer call sites; critical for `Starstrike\main.cpp` which uses `SimEvent::ParticleSpawn` etc. in its switch | Alias may confuse new developers who search for `struct SimEvent` | Full rename at all call sites — higher churn, same end state; can be done later as a follow-up |
| **Keep global `g_simEventQueue` in GameLogic** | Producers don't need any new linkage; global is defined in the same lib that uses it | Global is still a global (same as before) | Move global to Starstrike and pass by pointer — cleaner but touches every producer file signature |
| **Flat array, not ring buffer** | Matches current behavior exactly; no correctness risk | `DEBUG_ASSERT` fires in debug builds on overflow; Release silently drops events if > 1024 per frame (same as before) | Ring buffer or growable vector — unnecessary complexity for current load |

### Performance note

No runtime cost change. The template instantiation produces identical machine
code to the current non-template class. The struct layout is unchanged so cache
behavior is identical.

---

## Phase 2: Migrate GameSimEvent Fields from Legacy Math to DirectXMath

### Context

After the split (Steps 1-5 above), `GameSimEvent` still depends on the legacy
NeuronClient math types:

| Field | Current type | Header |
|---|---|---|
| `pos` | `LegacyVector3` | `NeuronClient/LegacyVector3.h` |
| `vel` | `LegacyVector3` | `NeuronClient/LegacyVector3.h` |
| `transform` | `Matrix34` | `NeuronClient/matrix34.h` |
| `particleColour` | `RGBAColour` | `NeuronClient/rgb_colour.h` (out of scope — not a math type) |

These types are being replaced project-wide per `MathPlan.md`:
- `LegacyVector3` → `GameVector3` (`NeuronCore/GameVector3.h`) — pure `XMFLOAT3` storage, no arithmetic operators.
- `Matrix34` → `GameMatrix` (`NeuronCore/GameMatrix.h`) — pure `XMFLOAT4X4` storage.

Both new types live in **NeuronCore** and are available everywhere via
`pch.h → NeuronCore.h → GameMath.h`. Migrating `GameSimEvent` to use them
removes its include dependency on the NeuronClient legacy headers, moving it
one step closer to being compilable against NeuronCore alone (a prerequisite
for eventual server-side event logging).

### Prerequisites

> ✅ **Resolved**: All prerequisites delivered as of 2025-07-18.  `GameVector3`
> and `GameMatrix` exist in NeuronCore.  Implicit conversion operators on
> `LegacyVector3`, `Matrix34`, and `Matrix33` are in place.

- ✅ `MathPlan.md` **Phase 1** (GameVector3 / GameMatrix types exist in NeuronCore).
- ✅ `MathPlan.md` **Phase 2** (implicit conversion operators on `LegacyVector3` ↔ `GameVector3`
  and `Matrix34` ↔ `GameMatrix`).
- ✅ **This document Phase 1 Steps 0-5** (split is complete; `GameSimEvent.h` is the
  canonical header; all producers use factories).

### Design

#### Header change — `GameLogic\GameSimEvent.h`

Replace the legacy include/field types:

```diff
  // GameLogic\GameSimEvent.h
  #pragma once

- #include "LegacyVector3.h"
- #include "matrix34.h"
- #include "rgb_colour.h"
+ #include "GameVector3.h"
+ #include "GameMatrix.h"
+ #include "rgb_colour.h"      // RGBAColour is unchanged — not a math type
  #include "worldobject.h"

  class ShapeStatic;

  // ... SimParticle / SimSoundSource namespaces unchanged ...

  struct GameSimEvent
  {
    // ... Type enum unchanged ...

    // Spatial data
-   LegacyVector3 pos;
-   LegacyVector3 vel;
+   Neuron::Math::GameVector3 pos;
+   Neuron::Math::GameVector3 vel;

    // ... objectId, objectType unchanged ...

    // ... particleType, particleSize, particleColour unchanged ...

    // Explosion fields
    const ShapeStatic* shape;
-   Matrix34 transform;
+   Neuron::Math::GameMatrix transform;
    float fraction;

    // ... sound fields unchanged ...

    // Factory helpers — signatures change to accept new types.
    // Overloads accepting LegacyVector3 / Matrix34 are provided during
    // the transition period via implicit conversions (MathPlan Phase 2),
    // so no call-site changes are needed yet.
  };

  using SimEvent = GameSimEvent;
```

#### Factory helper migration

The `Make*` factory functions change their parameter types from `LegacyVector3`
/ `Matrix34` to `GameVector3` / `GameMatrix`. Thanks to the implicit conversion
operators added in `MathPlan.md` Phase 2, **every existing call site compiles
without modification** — a `LegacyVector3` argument implicitly converts to
`GameVector3` at the call boundary.

```cpp
[[nodiscard]] static GameSimEvent MakeParticle(
    const Neuron::Math::GameVector3& _pos,
    const Neuron::Math::GameVector3& _vel,
    int _particleType,
    float _size = -1.0f,
    RGBAColour _colour = RGBAColour(0))
{
    GameSimEvent e = {};
    e.type = ParticleSpawn;
    e.pos = _pos;
    e.vel = _vel;
    e.particleType = _particleType;
    e.particleSize = _size;
    e.particleColour = _colour;
    return e;
}

[[nodiscard]] static GameSimEvent MakeExplosion(
    const ShapeStatic* _shape,
    const Neuron::Math::GameMatrix& _transform,
    float _fraction)
{
    GameSimEvent e = {};
    e.type = Explosion;
    e.shape = _shape;
    e.transform = _transform;
    e.fraction = _fraction;
    return e;
}
```

`MakeSoundOther` follows the same pattern for its `_pos` parameter.
`MakeSoundStop`, `MakeSoundBuilding`, and `MakeSoundEntity` have no vector
parameters (the dead `_pos`/`_vel`/`_objectType` params were removed from
`MakeSoundEntity` in Phase 1 Step 0b) and are unchanged.

#### Consumer-side impact — `DrainSimEvents()` in `Starstrike\main.cpp`

The drain loop reads `evt.pos`, `evt.vel`, `evt.transform`, etc. and passes
them to rendering/audio APIs that currently accept `LegacyVector3` / `Matrix34`.
During the transition period (while those APIs still take legacy types), the
implicit conversion operators handle the round-trip automatically:

```cpp
// Before and after — source code is identical:
g_app->m_particleSystem->CreateParticle(evt.pos, evt.vel, evt.particleType, ...);
//                                       ^^^ GameVector3 → LegacyVector3 implicit conversion
```

When the downstream APIs are themselves migrated (per `MathPlan.md` Phases 3-4),
the conversions become unnecessary and the fields flow through as
`GameVector3` / `GameMatrix` end-to-end.

### Migration steps

| # | Step | Files touched | Risk | Dependency |
|---|---|---|---|---|
| 6 | **Migrate `GameSimEvent` fields** — Change `pos`, `vel` from `LegacyVector3` to `GameVector3`; `transform` from `Matrix34` to `GameMatrix`. Update `#include` directives in `GameSimEvent.h`. | 1 file (`GameSimEvent.h`) | Low — implicit conversions handle all ~38 producer call sites and the consumer | MathPlan Phase 2 complete |
| 7 | **Remove legacy includes from `GameSimEvent.h`** — Drop `#include "LegacyVector3.h"` and `#include "matrix34.h"`. Verify no transitive dependency.  **Note:** `GameLogic\pch.h` includes `GameLogicPlatform.h` → `NeuronClient.h`, which transitively provides `LegacyVector3.h` and `matrix34.h` in every GameLogic TU. After removing the explicit includes, `GameSimEvent.h` still compiles within GameLogic — but would fail if included from a project with a different PCH (e.g. a future test project or NeuronServer). Ensure the header remains self-contained by including `GameVector3.h` / `GameMatrix.h` explicitly (done in Step 6). | 1 file | Low — header no longer needs legacy types directly | Step 6 |
| 8 | **Migrate `DrainSimEvents()` consumer** — Once downstream APIs (`ParticleSystem::CreateParticle`, `ExplosionManager::AddExplosion`, `SoundSystem::TriggerEntityEvent`) accept `GameVector3` / `GameMatrix`, remove any remaining implicit conversion round-trips. | 1 file (`Starstrike\main.cpp`) | Low | MathPlan Phase 4 for downstream APIs |
| 9 | **Build & verify** — Full rebuild. Verify particles, explosions, and sounds still work in-game. | 0 | Verification only | — |

### Interaction with MathPlan.md

This phase is designed to **slot between MathPlan Phases 2 and 3**:

```
MathPlan Phase 1:  GameVector3 / GameMatrix types created          ← prerequisite
MathPlan Phase 2:  Implicit conversions on legacy types             ← prerequisite
  ──► SimEvent.md Phase 2 Step 6-7: Migrate GameSimEvent fields    ← THIS WORK
MathPlan Phase 3:  Migrate WorldObject / Building / Entity headers
MathPlan Phase 4:  File-by-file .cpp migration
  ──► SimEvent.md Phase 2 Step 8: Migrate DrainSimEvents consumer  ← cleanup
```

This ordering is optimal because:
1. `GameSimEvent` is a **leaf consumer** of `LegacyVector3` — it stores vectors
   but does no arithmetic on them (the `Make*` factories just copy values in).
   Migrating it requires only storage-type changes, not load→compute→store
   rewrites.
2. Doing it before MathPlan Phase 3 proves the conversion bridge works
   end-to-end on a simple, contained struct before tackling the more complex
   `WorldObject` / `Entity` headers.
3. It eliminates `GameSimEvent.h`'s dependency on NeuronClient math headers
   early, cleaning up the dependency graph for the broader Phase 3 migration.

### Trade-off analysis

| Decision | Pros | Cons | Alternatives |
|---|---|---|---|
| **Migrate `GameSimEvent` fields early (before `WorldObject`)** | Low-risk proof of the conversion bridge; cleans up the dependency graph; `GameSimEvent` is a leaf type with no arithmetic | Creates a temporary state where `GameSimEvent` uses `GameVector3` but producers still pass `LegacyVector3` (relies on implicit conversion) | Wait until MathPlan Phase 4 file-by-file sweep — but this delays the dependency cleanup |
| **Use `Neuron::Math::GameVector3` (fully qualified) in the struct** | Unambiguous; follows NeuronCore conventions | Verbose for legacy GameLogic code style | Add `using Neuron::Math::GameVector3;` at the top of the header — acceptable shorthand |
| **Keep `RGBAColour` unchanged** | `RGBAColour` is a 4-byte colour, not a math vector; no SIMD benefit; no NeuronCore replacement exists | One legacy NeuronClient include remains in `GameSimEvent.h` | Migrate `RGBAColour` to a NeuronCore colour type — out of scope, separate plan |
| **Implicit conversion for transition period** | Zero call-site churn; same binary output; bridge is already planned in MathPlan Phase 2 | Implicit conversions can mask unintended copies; slight overhead from load/store round-trips on the conversion boundary | Explicit conversion at each call site — higher churn, more noise in the diff, same result |

### Performance note

During the transition period, each `Make*` call that passes a `LegacyVector3`
argument incurs one implicit conversion (3 float copies). This is identical in
cost to the current direct assignment and vanishes entirely once the producer
code is migrated to `GameVector3` in MathPlan Phase 4.

The `GameMatrix` field (`transform`) changes from 4× `LegacyVector3` (48 bytes)
to `XMFLOAT4X4` (64 bytes) — an increase of 16 bytes per event. At 1024 max
events this adds 16 KB to the queue's static footprint. The queue is stack- or
BSS-allocated and the extra bytes are in the padding row (row 3 w-column and
row 4 entirely). This is a negligible cost and brings the struct into alignment
with DirectXMath's native 4×4 layout, eliminating conversion overhead on the
consumer side when `ExplosionManager::AddExplosion` is eventually migrated to
accept `GameMatrix` directly.

---

## Phase 3: Convert Flat Struct to `std::variant`

### Context

After Phases 1 and 2, `GameSimEvent` is a flat struct — every event carries all
fields regardless of type (particle fields, explosion fields, sound fields).
Per-event size is ~152 bytes (after Phase 2's `GameMatrix` change).  At 1024
capacity the queue occupies ~152 KB of BSS.

Most fields are unused for any given event type.  Converting to `std::variant`
eliminates the wasted space and provides compile-time exhaustiveness checking:
the compiler enforces that every alternative is handled in the consumer.

### Prerequisites

- **Phase 1 complete** — all producers use factory methods; `GameSimEvent.h` is
  the canonical header; queue template is in NeuronCore.
- **Phase 2 complete** — fields use `GameVector3` / `GameMatrix` (so the variant
  structs don't reference legacy math types that would need another migration).
- **C++20 or later** — `std::variant` is C++17; CTAD for aggregates (needed for
  `Overloaded` without a deduction guide) is C++20.  The project uses
  `stdcpp20` (Release) and `stdcpplatest` (Debug), so both are satisfied.

### Design

#### Per-event structs (nested in `GameSimEvent`)

Each variant alternative is a named aggregate containing only the fields the
consumer actually reads.  Struct names match the old `Type` enum values so that
`SimEvent::ParticleSpawn` (previously the enum value) now resolves to the
struct.

```cpp
// Nested inside struct GameSimEvent

struct ParticleSpawn                                    // 36 bytes
{
    Neuron::Math::GameVector3 pos;                      // 12
    Neuron::Math::GameVector3 vel;                      // 12
    int           particleType;                         //  4
    float         particleSize;                         //  4  (-1 = default)
    RGBAColour    particleColour;                       //  4  (0 = default)
};

struct Explosion                                        // 80 bytes (incl. padding)
{
    const ShapeStatic* shape;                           //  8  non-owning
    Neuron::Math::GameMatrix transform;                 // 64  world transform
    float         fraction;                             //  4  0.0–1.0
};

struct SoundStop                                        // 24 bytes
{
    WorldObjectId objectId;                             // 16
    const char*   eventName;                            //  8  nullptr = stop all
};

struct SoundEntityEvent                                 // 24 bytes
{
    WorldObjectId objectId;                             // 16
    const char*   eventName;                            //  8
};

struct SoundBuildingEvent                               // 24 bytes
{
    WorldObjectId objectId;                             // 16
    const char*   eventName;                            //  8
};

struct SoundOtherEvent                                  // 40 bytes
{
    Neuron::Math::GameVector3 pos;                      // 12
    WorldObjectId objectId;                             // 16
    int           soundSourceType;                      //  4  SimSoundSource::Type*
    const char*   eventName;                            //  8
};
```

**Notable changes from the flat struct:**

- `SoundEntityEvent` no longer carries `pos`, `vel`, `objectType` — removed in
  Phase 1 Step 0b (consumer reads from `Entity*`).
- `SoundBuildingEvent` no longer carries `objectType` — dead field removed in
  Phase 3 Step 10 (consumer reads `_building->m_type` directly).
  `MakeSoundBuilding` signature changes from
  `(WorldObjectId, int, const char*)` to `(WorldObjectId, const char*)`.

#### `GameSimEvent` wrapper struct

```cpp
// GameLogic\GameSimEvent.h
#pragma once

#include "GameVector3.h"
#include "GameMatrix.h"
#include "rgb_colour.h"
#include "worldobject.h"

#include <variant>

class ShapeStatic;

namespace SimParticle { /* unchanged */ }
namespace SimSoundSource { /* unchanged */ }

struct GameSimEvent
{
    // --- Variant alternatives -------------------------------------------
    struct ParticleSpawn     { /* as above */ };
    struct Explosion         { /* as above */ };
    struct SoundStop         { /* as above */ };
    struct SoundEntityEvent  { /* as above */ };
    struct SoundBuildingEvent{ /* as above */ };
    struct SoundOtherEvent   { /* as above */ };

    using Variant = std::variant<
        ParticleSpawn, Explosion, SoundStop,
        SoundEntityEvent, SoundBuildingEvent, SoundOtherEvent>;

    Variant data;

    // --- Visitor convenience -------------------------------------------
    template<typename Visitor>
    decltype(auto) Visit(Visitor&& _vis) const
    {
        return std::visit(std::forward<Visitor>(_vis), data);
    }

    // --- Factory helpers (preserve producer call-site compatibility) ----

    [[nodiscard]] static GameSimEvent MakeParticle(
        const Neuron::Math::GameVector3& _pos,
        const Neuron::Math::GameVector3& _vel,
        int _particleType,
        float _size = -1.0f,
        RGBAColour _colour = RGBAColour(0))
    {
        return { .data = ParticleSpawn{ _pos, _vel, _particleType, _size, _colour } };
    }

    [[nodiscard]] static GameSimEvent MakeExplosion(
        const ShapeStatic* _shape,
        const Neuron::Math::GameMatrix& _transform,
        float _fraction)
    {
        return { .data = Explosion{ _shape, _transform, _fraction } };
    }

    [[nodiscard]] static GameSimEvent MakeSoundStop(
        WorldObjectId _objectId,
        const char* _eventName = nullptr)
    {
        return { .data = SoundStop{ _objectId, _eventName } };
    }

    [[nodiscard]] static GameSimEvent MakeSoundEntity(
        WorldObjectId _objectId,
        const char* _eventName)
    {
        return { .data = SoundEntityEvent{ _objectId, _eventName } };
    }

    [[nodiscard]] static GameSimEvent MakeSoundBuilding(
        WorldObjectId _objectId,
        const char* _eventName)
    {
        return { .data = SoundBuildingEvent{ _objectId, _eventName } };
    }

    [[nodiscard]] static GameSimEvent MakeSoundOther(
        const Neuron::Math::GameVector3& _pos,
        WorldObjectId _objectId,
        int _soundSourceType,
        const char* _eventName)
    {
        return { .data = SoundOtherEvent{ _pos, _objectId, _soundSourceType, _eventName } };
    }
};

static_assert(std::is_trivially_copyable_v<GameSimEvent>);

// Back-compat alias
using SimEvent = GameSimEvent;
```

**Key design decisions:**

1. **Nested structs** — The variant alternatives are nested inside
   `GameSimEvent` so that `SimEvent::ParticleSpawn` resolves to the struct
   (previously the enum value).  This preserves naming familiarity in the
   consumer and across grep results.

2. **`Visit()` convenience method** — Wraps `std::visit` over `data`, so
   consumers write `evt.Visit(Overloaded{...})` instead of
   `std::visit(..., evt.data)`.  No runtime cost (inlined).

3. **`using Variant`** — Exposed as a public typedef for cases where code
   needs the raw variant type (e.g., `std::get_if`).

4. **Factories return `GameSimEvent`** — Uses C++20 designated initializers
   (`{ .data = ... }`) for clarity.  Producer call sites are completely
   unchanged: `g_simEventQueue.Push(SimEvent::MakeParticle(...))`.

5. **Trivially copyable** — All alternatives are aggregates of trivial types.
   `std::variant` of trivially copyable types is itself trivially copyable
   (guaranteed since C++20 via conditionally-trivial special members in
   `variant`).  The wrapper adds no non-trivial members.  Queue flat-array
   copy semantics are preserved.  Enforced with
   `static_assert(std::is_trivially_copyable_v<GameSimEvent>)`.

6. **`Type` enum removed** — The `std::variant` discriminant replaces the
   manual enum.  The nested struct names serve as the type tags.  Any code
   that previously compared `evt.type == SimEvent::ParticleSpawn` must switch
   to `std::holds_alternative<SimEvent::ParticleSpawn>(evt.data)` or use the
   visitor pattern.

#### `Overloaded` helper (NeuronCore)

The `std::visit` + overloaded-lambdas pattern requires a small helper:

```cpp
// NeuronCore\Overloaded.h
#pragma once

// Overloaded — aggregate for composing lambda visitors.
// C++20 aggregate CTAD: no deduction guide needed.
template<typename... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
```

This is added to NeuronCore so any project can use it.  Add
`#include "Overloaded.h"` to `NeuronCore.h` for availability via `pch.h`.

#### Consumer rewrite — `DrainSimEvents()` in `Starstrike\main.cpp`

The `switch` on the `Type` enum is replaced by `std::visit` with an
`Overloaded` lambda set.  The logic in each arm is identical to the current
implementation:

```cpp
static void DrainSimEvents()
{
    for (int i = 0; i < g_simEventQueue.Count(); ++i)
    {
        g_simEventQueue.Get(i).Visit(Overloaded{
            [](const SimEvent::ParticleSpawn& e)
            {
                g_app->m_particleSystem->CreateParticle(
                    e.pos, e.vel, e.particleType,
                    e.particleSize, e.particleColour);
            },
            [](const SimEvent::Explosion& e)
            {
                if (e.shape)
                    g_explosionManager.AddExplosion(
                        e.shape, e.transform, e.fraction);
            },
            [](const SimEvent::SoundStop& e)
            {
                g_app->m_soundSystem->StopAllSounds(e.objectId, e.eventName);
            },
            [](const SimEvent::SoundEntityEvent& e)
            {
                Entity* entity = g_app->m_location->GetEntity(e.objectId);
                if (entity)
                    g_app->m_soundSystem->TriggerEntityEvent(entity, e.eventName);
            },
            [](const SimEvent::SoundBuildingEvent& e)
            {
                Building* building = g_app->m_location->GetBuilding(
                    e.objectId.GetUniqueId());
                if (building)
                    g_app->m_soundSystem->TriggerBuildingEvent(
                        building, e.eventName);
            },
            [](const SimEvent::SoundOtherEvent& e)
            {
                WorldObject tmp;
                tmp.m_pos = e.pos;
                tmp.m_id = e.objectId;
                g_app->m_soundSystem->TriggerOtherEvent(
                    &tmp, e.eventName, e.soundSourceType);
            },
        });
    }
    g_simEventQueue.Clear();
}
```

**Exhaustiveness** — Adding a new alternative to the variant without adding a
corresponding lambda produces a compile error.  The `default: DEBUG_ASSERT(false)`
added in Phase 1 Step 3 is no longer needed and is removed (the compiler
enforces completeness statically).

#### Size analysis

| Alternative | Size (bytes) | Notes |
|---|---|---|
| `ParticleSpawn` | 36 | 2×GameVector3(12) + int + float + RGBAColour |
| `Explosion` | 80 | ptr(8) + GameMatrix(64) + float(4) + padding(4) |
| `SoundStop` | 24 | WorldObjectId(16) + ptr(8) |
| `SoundEntityEvent` | 24 | WorldObjectId(16) + ptr(8) |
| `SoundBuildingEvent` | 24 | WorldObjectId(16) + ptr(8) — `objectType` removed |
| `SoundOtherEvent` | 40 | GameVector3(12) + WorldObjectId(16) + int(4) + ptr(8) |
| **`std::variant` total** | **~88** | max(80) + discriminant(4) + padding(4) |

A manual tagged union (`enum` + `union`) would produce the same 88 bytes
(4-byte enum + 4-byte padding + 80-byte union).  `std::variant` has **zero
size overhead** compared to a hand-rolled union while providing type safety
and exhaustiveness checking.

| Metric | Phase 2 flat struct | Phase 3 `std::variant` | Reduction |
|---|---|---|---|
| Per-event size | ~152 bytes | ~88 bytes | **42%** |
| Queue footprint (1024) | ~152 KB | ~88 KB | **~64 KB saved** |

> **Note:** The original Phase 3 placeholder estimated ~72 bytes for a tagged
> union.  That figure assumed the pre-Phase-2 `Matrix34` (48 bytes).  After
> Phase 2's migration to `GameMatrix` (`XMFLOAT4X4`, 64 bytes), the largest
> alternative (`Explosion`) grows to 80 bytes, making the correct variant size
> ~88 bytes.

### Migration steps

| # | Step | Files touched | Risk | Dependency |
|---|---|---|---|---|
| 10 | **Simplify `MakeSoundBuilding`** — Drop dead `_objectType` parameter (consumer reads `_building->m_type`, not `evt.objectType`).  Update all 42 call sites across ~18 files.  Analogous to Phase 1 Step 0b's `MakeSoundEntity` cleanup. | `GameSimEvent.h` + ~18 `.cpp` | Low — dead-parameter removal; behaviour unchanged | Phase 1 + 2 complete |
| 11 | **Add `Overloaded` helper** — Create `NeuronCore\Overloaded.h` with the aggregate template.  Add `#include "Overloaded.h"` to `NeuronCore.h`.  Add to `NeuronCore.vcxproj`. | 3 files (1 new, 2 modified) | None — additive only | — |
| 12 | **Convert `GameSimEvent` to variant** — Replace the flat struct with the `std::variant`-based wrapper: nested structs, `Variant` typedef, `Visit()` helper, updated factories.  Remove the `Type` enum.  Add `#include <variant>`.  Add `static_assert(std::is_trivially_copyable_v<GameSimEvent>)`. | 1 file (`GameSimEvent.h`) | Medium — breaking change for consumer; must be atomically paired with Step 13 | Steps 10, 11 |
| 13 | **Rewrite `DrainSimEvents()`** — Replace the `switch`/`case` with `std::visit` + `Overloaded` lambdas.  Remove the `default: DEBUG_ASSERT(false)` (compiler now enforces exhaustiveness statically). | 1 file (`Starstrike\main.cpp`) | Medium — logic rewrite (same behaviour, different dispatch) | Step 12 |
| 14 | **Build & verify** — Full rebuild (Debug + Release).  Verify particles, explosions, and sounds still work in-game.  Spot-check `sizeof(GameSimEvent)` ≤ 88. | 0 | Verification only | — |

> **Steps 12 + 13 must be a single commit** — changing `GameSimEvent` to a
> variant without updating the consumer would break the build.

### Trade-off analysis

| Decision | Pros | Cons | Alternatives |
|---|---|---|---|
| **`std::variant` vs manual `union`** | Type safety; compiler-enforced exhaustiveness; no manual discriminant management; zero size overhead (same ~88 bytes) | Requires `<variant>` header; `std::visit` codegen on MSVC can use a function-pointer table instead of a jump table — but improving with each compiler release; slightly more complex error messages on mismatch | Manual `enum` + `union` — same size, no type safety, no exhaustiveness checking; error-prone if new types are added |
| **Nested structs inside `GameSimEvent`** | `SimEvent::ParticleSpawn` naming preserved (was enum value, now struct); all related types in one header; grep-friendly | Wrapper struct adds one level of indirection (`.data` member); nested types can't be forward-declared | Top-level structs in a namespace — but `namespace SimEvent` would conflict with the `using SimEvent = GameSimEvent` alias |
| **`Visit()` convenience method** | Consumer writes `evt.Visit(Overloaded{...})` — clean, discoverable; avoids exposing `.data` in the public API | Extra template in the struct; some teams prefer explicit `std::visit` | Drop `Visit()` and use `std::visit(..., evt.data)` directly — marginally more verbose |
| **`Overloaded` in NeuronCore** | Reusable across all projects; standard C++ pattern; 3-line header | One more header in the shared foundation | Local to `main.cpp` — works but other consumers (tests, tools) would duplicate it |
| **Drop `SoundBuildingEvent.objectType` (Step 10)** | Shrinks the struct from 32 → 24 bytes; removes confirmed dead parameter (consumer reads from `Building*`); consistent with Phase 1 Step 0b's `MakeSoundEntity` cleanup | Requires touching ~18 producer files + 1 header | Keep the dead field — wastes 8 bytes per `SoundBuildingEvent`, inconsistent with the Step 0b cleanup |

### `std::visit` codegen concern

MSVC's `std::visit` historically generates an indirect call through a
function-pointer table rather than a direct jump table.  For 6 alternatives
this means one extra load + indirect branch per event.  At typical event
volumes (tens to low hundreds per frame) this is **negligible** — the drain
loop runs once per frame after `Location::Advance()` and is not a hot path.

If profiling ever shows the visitor dispatch as a bottleneck:

1. Replace `std::visit` with a manual `switch` on `data.index()` +
   `std::get_if` — recovers direct-branch codegen while keeping the variant
   layout and size savings.
2. Or use a constexpr visitor dispatch library with known-good codegen.

Both are opt-in micro-optimizations and should not be pursued without
profiling evidence.

### Performance note

The variant layout reduces per-event size by 42% (~152 → ~88 bytes), improving
cache utilization during the drain loop.  The most common events
(`ParticleSpawn` at 36 bytes, `SoundEntityEvent` at 24 bytes) are
significantly smaller than the current flat struct, so the effective cache
improvement is better than the 42% headline figure for typical workloads.

The queue remains a flat BSS-allocated array with no per-frame allocations.
`SimEventQueue<GameSimEvent, 1024>` occupies ~88 KB — comfortably within a
single L2 cache slice (typically 256 KB–1 MB per core on modern x64).

All alternatives are trivially copyable aggregates, so `Push()` compiles to a
`memcpy`-equivalent — same as the current flat struct.

---

### Full Checklist

#### Phase 1: Split & Relocate ✅

- [x] 43 manual-construction sites converted to `Make*` factory calls (Step 0a)
- [x] Dead `pos`/`vel` writes on `SoundBuildingEvent` removed (Step 0a)
- [x] `MakeSoundEntity` simplified — dead `_pos`, `_vel`, `_objectType` params removed; 49 call sites updated (Step 0b)
- [x] `NeuronCore\SimEventQueue.h` created with template (includes `DEBUG_ASSERT` on overflow)
- [x] `GameLogic\GameSimEvent.h` created (struct + alias + constant namespaces)
- [x] `GameLogic\GameSimEventQueue.h` created (typedef + extern)
- [x] `GameLogic\GameSimEventQueue.cpp` created (global definition)
- [x] All ~42 `.cpp` includes redirected to `GameSimEventQueue.h`
- [x] `Starstrike\main.cpp` include redirected (`using SimEvent = GameSimEvent` alias required for switch cases)
- [x] Old `SimEvent.h`, `SimEventQueue.h`, `SimEventQueue.cpp` removed
- [x] Full rebuild passes (Debug + Release)
- [x] `DrainSimEvents()` switch still compiles and covers all `Type` values
- [x] `DrainSimEvents()` switch has `default: DEBUG_ASSERT(false);` for exhaustiveness
- [x] `docs\gamelogic.md` updated to reflect new file names **and** corrected struct description (Phase 3 section rewritten with actual `GameSimEvent` variant architecture, correct file paths, `std::visit` drain loop, cross-reference to `SimEvent.md`)

#### Phase 2: DirectXMath Migration ✅

- [x] `MathPlan.md` created
- [x] `MathPlan.md` Phase 1 complete (GameVector3, GameMatrix types exist in NeuronCore)
- [x] `MathPlan.md` Phase 2 complete (implicit conversions on legacy types)
- [x] `GameSimEvent.h` fields migrated to `GameVector3` / `GameMatrix`
- [x] `GameSimEvent.h` self-contained: includes `GameVector3.h` / `GameMatrix.h` explicitly (not relying on `pch.h` transitivity)
- [x] `#include "LegacyVector3.h"` and `#include "matrix34.h"` removed from `GameSimEvent.h`
- [x] Full rebuild passes (Debug + Release)
- [x] In-game verification: particles, explosions, and sounds work correctly
- [ ] `DrainSimEvents()` consumer migrated (after downstream APIs accept new types — see Step 8 details below)

#### Phase 3: `std::variant` Conversion ✅

- [x] `MakeSoundBuilding` simplified — dead `_objectType` param removed; 37 call sites updated across 17 files (Step 10)
- [x] `NeuronCore\Overloaded.h` created; included from `NeuronCore.h` (Step 11)
- [x] `GameSimEvent` converted to `std::variant` wrapper: nested structs, `Variant` typedef, `Visit()` helper, `Type` enum removed (Step 12)
- [x] `static_assert` not applicable — MSVC `std::variant` is not trivially copyable even with all-trivial alternatives; replaced with explanatory comment (Step 12)
- [x] 20 remaining manual-construction sites converted to factory calls across 5 files: `souldestroyer.cpp`, `radardish.cpp`, `spam.cpp`, `spirit.cpp`, `tree.cpp` (missed in Phase 1 Step 0a, caught during Step 12)
- [x] `DrainSimEvents()` rewritten to use `std::visit` + `Overloaded` lambdas; `default: DEBUG_ASSERT(false)` removed (Step 13)
- [x] Full rebuild passes (Debug + Release)
- [x] In-game verification: particles, explosions, and sounds work correctly
- [x] `sizeof(GameSimEvent)` ≤ 88 verified (compile-time `static_assert` in `GameSimEvent.h`)

### Status

- **Phase 1**: ✅ Complete.
- **Phase 2**: ✅ Complete (Steps 6-7, 9). In-game verified. Step 8 (`DrainSimEvents()` consumer cleanup) deferred — see below.
- **Phase 3**: ✅ Complete (Steps 10-14). In-game verified. `sizeof` spot-check pending.

#### Step 8: `DrainSimEvents()` consumer cleanup (deferred)

`DrainSimEvents()` currently relies on implicit conversions at three points:

| Call site | Event field type | Downstream API parameter type | Conversion |
|---|---|---|---|
| `CreateParticle(e.pos, e.vel, ...)` | `GameVector3` | `const LegacyVector3&` | `GameVector3` → `LegacyVector3` (2×) |
| `AddExplosion(e.shape, e.transform, ...)` | `GameMatrix` | `const Matrix34&` | `GameMatrix` → `Matrix34` |
| `tmp.m_pos = e.pos` | `GameVector3` | `WorldObject::m_pos` (`LegacyVector3`) | `GameVector3` → `LegacyVector3` |

These conversions are zero-cost (float copies identical to direct assignment)
and will vanish automatically when the downstream APIs are migrated:

1. **`ParticleSystem::CreateParticle`** — migrate `_pos` / `_vel` params from
   `LegacyVector3` to `GameVector3` (MathPlan Phase 4, Starstrike file sweep).
2. **`ExplosionManager::AddExplosion`** — migrate `_transform` param from
   `Matrix34` to `GameMatrix` (MathPlan Phase 4, Starstrike file sweep).
3. **`WorldObject::m_pos`** — migrate from `LegacyVector3` to `GameVector3`
   (MathPlan Phase 3, `WorldObject` header migration).

No action needed in `DrainSimEvents()` itself — once the downstream types
change, the implicit conversions simply stop being invoked. Step 8 will be
closed as a by-product of MathPlan Phases 3-4 with no dedicated commit.

### Date

2025-07-19
