# Implementation Plan: Migrate LegacyVector3, Matrix33, Matrix34 → GameMath.h / DirectXMath

## Overview

Replace the legacy scalar math types (`LegacyVector3`, `Matrix33`, `Matrix34`) residing in `NeuronClient` with SIMD-backed DirectXMath equivalents centralized in `NeuronCore/GameMath.h`. The migration must be incremental—each phase leaves every project buildable and the game runnable—because the legacy types are included by virtually every translation unit in GameLogic, NeuronClient, and Starstrike.

## Current State

| Type | Header | Impl | Project | Notes |
|---|---|---|---|---|
| `LegacyVector3` | `NeuronClient/LegacyVector3.h` | `NeuronClient/vector3.cpp` | NeuronClient | 3-float struct; `x`, `y`, `z` public members. Operator `*` = dot, `^` = cross. `GetData()` returns `float*`. Used in ~100+ files across GameLogic, NeuronClient, Starstrike. |
| `Matrix33` | `NeuronClient/matrix33.h` | `NeuronClient/matrix33.cpp` | NeuronClient | 3× `LegacyVector3` rows (`r`, `u`, `f`). Orient/Rotate helpers. `ConvertToOpenGLFormat()` static buffer. |
| `Matrix34` | `NeuronClient/matrix34.h` | `NeuronClient/matrix34.cpp` | NeuronClient | 4× `LegacyVector3` rows (`r`, `u`, `f`, `pos`). Used as world transforms everywhere. `ConvertToOpenGLFormat()` static buffer. |
| `Vector2` | `NeuronClient/vector2.h` | `NeuronClient/vector2.cpp` | NeuronClient | Depends on `LegacyVector3` (conversion ctor). Out of scope for this plan but must remain compilable. |
| `math_utils.h` | `NeuronClient/math_utils.h` | `NeuronClient/math_utils.cpp` | NeuronClient | Geometry utilities taking `LegacyVector3` / `Matrix34` params. |
| `GameMath.h` | `NeuronCore/GameMath.h` | — (inline) | NeuronCore | Already provides `Neuron::Math` wrappers around DirectXMath `XMVECTOR`/`XMMATRIX`. Already included in every project via `NeuronCore.h → pch.h`. |

### Include chain (all projects)

```
pch.h
 └─ NeuronClient.h (or NeuronServer.h)
     └─ NeuronCore.h
         ├─ GameMath.h          ← DirectXMath wrappers (already available everywhere)
         └─ ...
     └─ opengl_directx.h       ← GL shim macros
     └─ ...
```

Legacy headers are included **per-file** (`#include "LegacyVector3.h"`, etc.), not via `pch.h`.

### Dependency graph affected

```
NeuronCore ──► NeuronClient ──► GameLogic ──► Starstrike
                                            ──► NeuronServer (no legacy math use)
```

## Requirements

- Replace `LegacyVector3` with a thin `XMFLOAT3`-backed pure storage struct (`GameVector3`) in all game code. All math operations go through `Neuron::Math` free functions or native DirectXMath on `XMVECTOR`.
- Replace `Matrix33` and `Matrix34` with a single `XMFLOAT4X4`-backed pure storage struct (`GameMatrix`). No separate 3×3 type — DirectXMath operates on 4×4 `XMMATRIX` uniformly.
- Maintain `float x, y, z` field access for serialization, OpenGL shim (`glVertex3fv`), and data-driven code.
- Maintain interop with `Vector2` (which converts to/from `LegacyVector3`).
- **Minimize duplication**: No math methods or arithmetic operators on storage types. All operations consolidated into `Neuron::Math` free functions in `GameMath.h`, delegating to native DirectXMath. Convenience overloads that accept `GameVector3`/`GameMatrix` are trivial load→delegate→store wrappers.
- Match existing `Neuron::Math` conventions in `GameMath.h` (`[[nodiscard]]`, `XM_CALLCONV`, `noexcept`).
- Every phase compiles+links+runs before starting the next.
- Performance: the pure storage approach naturally pushes math into `XMVECTOR`/`XMMATRIX` registers, eliminating unnecessary load/store round-trips by design.

## Architecture Changes

### New files

| File | Project | Purpose |
|---|---|---|
| `NeuronCore/GameVector3.h` | NeuronCore | Pure storage type `GameVector3` (wraps `XMFLOAT3`), with `Load()`/`Store()` conversion helpers to/from `XMVECTOR`. No arithmetic operators or math methods — all math goes through `Neuron::Math` free functions. Replaces `LegacyVector3` as the persistent member type. |
| `NeuronCore/GameMatrix.h` | NeuronCore | Pure storage type `GameMatrix` (wraps `XMFLOAT4X4`), with `Load()`/`Store()` helpers, row accessors, and `Identity()` factory. Replaces both `Matrix33` and `Matrix34` — no separate `GameMatrix33` type because DirectXMath operates on 4×4 `XMMATRIX` uniformly. |

### Modified files (summary)

| File | Change |
|---|---|
| `NeuronCore/GameMath.h` | `#include "GameVector3.h"` and `#include "GameMatrix.h"`. Add free-function equivalents of legacy member functions. |
| `NeuronClient/LegacyVector3.h` | Phase 2: rewrite internals to store `XMFLOAT3`, keep `x`/`y`/`z` via union or properties. Phase 4+: thin typedef/alias to `GameVector3`. |
| `NeuronClient/matrix33.h` | Phase 2–3: add conversion operators to `GameMatrix`. Phase 4+: thin alias to `GameMatrix` (with translation zeroed). |
| `NeuronClient/matrix34.h` | Phase 2–3: rewrite internals. Phase 4+: thin alias to `GameMatrix`. |
| `NeuronClient/vector3.cpp` | Reimpl using `GameVector3` helpers; eventually deleted. |
| `NeuronClient/math_utils.h/.cpp` | Migrate function signatures to accept `GameVector3`/`GameMatrix` (or overloads). |
| `GameLogic/*.cpp`, `Starstrike/*.cpp` | Gradual file-by-file migration from `LegacyVector3` → `GameVector3`, `Matrix34` → `GameMatrix`. |
| `GameLogic/worldobject.h` | `m_pos`, `m_vel` change from `LegacyVector3` to `GameVector3`. |
| `GameLogic/building.h` | `m_front`, `m_up`, `m_centrePos` change from `LegacyVector3` to `GameVector3`. |

## Implementation Steps

---

### Phase 1: Introduce Storage Types and Free Functions in NeuronCore (2 new + 1 modified, low risk)

**Goal**: Define `GameVector3` and `GameMatrix` as storage-oriented types in `NeuronCore/GameMath.h` ecosystem. No legacy code is modified yet. Every project already pulls in `GameMath.h` via `pch.h`, so these types become available everywhere with zero include changes.

#### Step 1.1 — Define `GameVector3` pure storage type

**File**: `NeuronCore/GameVector3.h` (new)

- Action: Create a `struct GameVector3` in `namespace Neuron::Math` that is a **pure storage type** — no arithmetic operators, no math methods:
  - Stores `XMFLOAT3` internally (or public `float x, y, z` members in a union with `XMFLOAT3` for layout compatibility).
  - Provides constructors: default (zero-init), `(float, float, float)`, from `XMFLOAT3`, from `XMVECTOR` (stores).
  - Provides `Load()` → `XMVECTOR` and constructor from `XMVECTOR` (store).
  - Provides `GetData()` → `float*` and `GetDataConst()` → `float const*` (for OpenGL shim interop / `glVertex3fv`).
  - Provides `Zero()` and `Set(float,float,float)` mutators for legacy compatibility.
  - Provides `operator==` and `operator!=` (uses `NearlyEquals` per-component, matching `LegacyVector3` semantics).
  - **No** arithmetic operators (`+`, `-`, `*`, `/`, `+=`, `-=`, etc.).
  - **No** math methods (`Mag`, `Normalise`, `Dot`, `Cross`, etc.).
  - **No** `operator*` (dot) or `operator^` (cross) — these remain **only** on `LegacyVector3` during migration.
  - **No** duplicate constants — migrated code uses the existing `Neuron::Math::Vector3::UP`, `ZERO`, etc. (`XMVECTORF32`) constants directly. Where a `GameVector3` is needed for storage initialization, construct on the fly: `GameVector3(0.f, 1.f, 0.f)` is `constexpr` and free.
- Why: Establishes the new data type as a pure load/store bridge to `XMVECTOR`. All math operations go through the existing `Neuron::Math` free functions in `GameMath.h`, eliminating API duplication. This also means migrated code naturally operates in `XMVECTOR` registers, making the Phase 6 SIMD optimization the default rather than a retrofit.
- Dependencies: None.
- Build impact: **Low** — new file, included only by `GameMath.h`.
- Risk: Low. Union layout between `XMFLOAT3` and `{x,y,z}` must be verified for standard-layout compliance.

#### Step 1.2 — Define `GameMatrix` pure storage type

**File**: `NeuronCore/GameMatrix.h` (new)

- Action: Create a single `struct GameMatrix` in `namespace Neuron::Math` that is a **pure storage type** — no math methods:
  - Wraps `XMFLOAT4X4`. Represents an affine transform (rotation + translation). Both `Matrix33` and `Matrix34` map to this single type — a `Matrix33` equivalent is simply a `GameMatrix` with translation zeroed.
  - Provides `Load()` → `XMMATRIX`, constructor from `XMMATRIX` (store).
  - Provides row accessors: `GetRight()`, `GetUp()`, `GetForward()`, `GetTranslation()` → `GameVector3`.
  - Provides row mutators: `SetRight()`, `SetUp()`, `SetForward()`, `SetTranslation()`.
  - Provides constructor `(GameVector3 front, GameVector3 up, GameVector3 pos)` matching the legacy `Matrix34(front, up, pos)` pattern — computes `right = Cross(up, front)`, normalises, then builds the 4×4 rows.
  - Provides `Identity()` static factory.
  - Provides `operator==` (per-element approximate comparison, matching `Matrix34` semantics).
  - **No** `GameMatrix33` type. DirectXMath operates on 4×4 `XMMATRIX` for everything; using `XMFLOAT3X3` would force extra conversion overhead. The old `Matrix33(r, u, f)` pattern becomes `GameMatrix(r, u, f, GameVector3(0,0,0))`. The old `GetOr()` becomes a free function that copies rotation rows and zeros translation.
  - **No** arithmetic operators (`operator*`), no `Transpose()`, `Invert()`, `Normalise()`, `DecomposeToYDR()`, `RotateAround*()` — all of these use native DirectXMath or `Neuron::Math` free functions on the loaded `XMMATRIX`.
- Why: Keeps a single source of truth for all math operations (the `XMVECTOR`/`XMMATRIX` overloads in `Neuron::Math` and native DirectXMath). Eliminates the `Matrix33`/`Matrix34` code duplication problem — both legacy types had near-identical `Orient*`, `RotateAround*`, `Normalise`, and `DecomposeToYDR` implementations.
- Dependencies: Step 1.1.
- Build impact: **Low** — new file.
- Risk: Medium. Matrix memory layout must match expectations for `ConvertToOpenGLFormat` and the GL shim's `glLoadMatrixf`-style calls. The legacy `Matrix34` stores rows as `{r, u, f, pos}` which maps to a **row-major** 4×3 layout; DirectXMath is row-major 4×4 — verify the mapping carefully.

#### Step 1.3 — Extend `GameMath.h` with free functions and include new headers

**File**: `NeuronCore/GameMath.h` (modify)

- Action:
  - Add `#include "GameVector3.h"` and `#include "GameMatrix.h"` at the top of `GameMath.h` (after `MathCommon.h`).
  - Add **convenience `GameVector3`-accepting overloads** of existing `Neuron::Math` free functions. These are trivial one-liner delegations (load → call XMVECTOR version → return), keeping the `XMVECTOR` overload as the single source of truth:
    ```cpp
    [[nodiscard]] inline float Length(GameVector3 const& _v) noexcept { return Length(XMLoadFloat3(&_v.v)); }
    [[nodiscard]] inline float LengthSquare(GameVector3 const& _v) noexcept { return LengthSquare(XMLoadFloat3(&_v.v)); }
    [[nodiscard]] inline float Dotf(GameVector3 const& _a, GameVector3 const& _b) noexcept { ... }
    [[nodiscard]] inline GameVector3 Cross(GameVector3 const& _a, GameVector3 const& _b) noexcept { ... }
    [[nodiscard]] inline GameVector3 Normalize(GameVector3 const& _v) noexcept { ... }
    [[nodiscard]] inline GameVector3 SetLength(GameVector3 const& _v, float _len) noexcept { ... }
    ```
  - Add **domain-specific free functions** migrated from the legacy member functions. These operate on `XMMATRIX` / `XMVECTOR` (not on storage types) to avoid duplication:
    - `OrientFU(FXMVECTOR _front, FXMVECTOR _up)` → `XMMATRIX` — builds an orthonormal rotation matrix from front+up. Same for `OrientRU`, `OrientUF`, `OrientUR`, `OrientFR`, `OrientRF`.
    - `DecomposeToYDR(FXMMATRIX _m)` → `XMFLOAT3` {yaw, dive, roll} — euler decomposition.
    - `NormaliseMatrix(FXMMATRIX _m)` → `XMMATRIX` — re-orthonormalise the rotation rows.
    - `HorizontalAndNormalise(FXMVECTOR _v)` → `XMVECTOR` — zeroes Y, normalises XZ plane.
    - `TransformPoint(GameMatrix const& _m, GameVector3 const& _v)` → `GameVector3` — affine point transform (wraps `XMVector3TransformCoord`).
    - `TransformVector(GameMatrix const& _m, GameVector3 const& _v)` → `GameVector3` — direction-only transform (wraps `XMVector3TransformNormal`).
    - `InverseTransformPoint(GameMatrix const& _m, GameVector3 const& _v)` → `GameVector3` — replaces `Matrix34::InverseMultiplyVector`.
    - `ToColumnMajor(GameMatrix const& _m, float (&_out)[16])` — replaces `ConvertToOpenGLFormat()`. Writes to caller-supplied array (no static buffer).
    - `GetOrientation(GameMatrix const& _m)` → `GameMatrix` — returns copy with translation zeroed (replaces `Matrix34::GetOr()` → `Matrix33`).
  - These functions already have natural homes: the `Orient*` functions extend the existing rotation helpers, and `TransformPoint`/`TransformVector` complement the existing `RotateAroundX/Y/Z`.
- Why: Consolidates all math operations into `Neuron::Math` free functions. The storage types stay pure. No parallel API.
- Dependencies: Steps 1.1, 1.2.
- Build impact: **High** — `GameMath.h` is included via `NeuronCore.h` which is in every pch. However, since we are only *adding* new functions (no modifications to existing code), the rebuild is safe.
- Risk: Low — purely additive.

> **Design principle**: Every math operation has exactly one implementation — the `XMVECTOR`/`XMMATRIX` version. The `GameVector3`/`GameMatrix` convenience overloads are trivial load→delegate→store wrappers. If you need to add a new math operation, add it once as an `XMVECTOR` overload, then optionally add a `GameVector3` convenience wrapper.

#### Phase 1 Verification

- Full rebuild of all projects (Debug x64, Release x64).
- No runtime changes expected; game runs identically.
- Write a small unit test (VS Native Unit Test) that exercises:
  - `GameVector3` construction, `Load()`/constructor round-trip, `GetData()` returns correct pointer, `x`/`y`/`z` field access.
  - `GameMatrix` construction (from front/up/pos), `Load()`/constructor round-trip, `Identity()`, row accessor correctness.
  - `GameVector3` convenience overloads: `Length`, `Dotf`, `Cross`, `Normalize`, `SetLength` match known values.
  - `TransformPoint` and `TransformVector` match `Matrix34 * LegacyVector3` for known inputs.
  - `ToColumnMajor` matches legacy `ConvertToOpenGLFormat()` output for a known matrix.
  - `OrientFU` matches legacy `Matrix34::OrientFU` for known inputs.

---

### Phase 2: Add Compatibility Layer — Implicit Conversion Between Legacy and New Types (3 files, medium risk)

**Goal**: Allow legacy code and new code to coexist by providing implicit or explicit conversions between `LegacyVector3` ↔ `GameVector3` and `Matrix34` ↔ `GameMatrix`. This enables **file-by-file** migration in later phases.

#### Step 2.1 — Add conversion operators/constructors to `LegacyVector3`

**File**: `NeuronClient/LegacyVector3.h` (modify)

- Action:
  - Add `#include "GameVector3.h"` (already available via pch, but explicit include for clarity).
  - Add converting constructor: `LegacyVector3(GameVector3 const& v) : x(v.x), y(v.y), z(v.z) {}`.
  - Add conversion operator: `operator GameVector3() const { return GameVector3(x, y, z); }`.
  - This allows any function taking `GameVector3` to accept `LegacyVector3` and vice versa.
- Why: Enables gradual migration — callers can pass either type.
- Dependencies: Phase 1.
- Build impact: **High** — `LegacyVector3.h` is included by ~100+ files. But the change is purely additive.
- Risk: Medium — implicit conversions can cause ambiguous overloads if both old and new overloads exist for the same function. Mitigate by not adding duplicate overloads until Phase 4.

#### Step 2.2 — Add conversion operators/constructors to `Matrix34`

**File**: `NeuronClient/matrix34.h` (modify)

- Action:
  - Add `#include "GameMatrix.h"`.
  - Add converting constructor: `Matrix34(GameMatrix const& m)` — extracts rows.
  - Add conversion operator: `operator GameMatrix() const` — constructs `GameMatrix` from `{r, u, f, pos}`.
- Why: Same rationale as Step 2.1.
- Dependencies: Phase 1 + Step 2.1.
- Build impact: **High** — `matrix34.h` is widely included.
- Risk: Medium — same ambiguity risk as Step 2.1.

#### Step 2.3 — Add conversion operators/constructors to `Matrix33`

**File**: `NeuronClient/matrix33.h` (modify)

- Action: Same pattern as Step 2.2 but for `Matrix33` ↔ `GameMatrix`.
  - The converting constructor builds a `Matrix33` from the rotation rows of a `GameMatrix`, ignoring translation.
  - The conversion operator builds a `GameMatrix` from `{r, u, f}` with translation zeroed.
  - This is valid because `GameMatrix33` does not exist — both legacy matrix types map to the single `GameMatrix` (4×4) storage type.
- Dependencies: Phase 1 + Step 2.1.
- Build impact: **High**.
- Risk: Medium.

#### Step 2.4 — Bridge globals

**File**: `NeuronCore/GameMath.h` (already modified in Step 1.3)

- Action: Migrated code uses the existing `Neuron::Math::Vector3::UP` / `Neuron::Math::Vector3::ZERO` (`XMVECTORF32`) constants directly. These implicitly convert to `XMVECTOR` for all math operations. Where a `GameVector3` storage value is needed, construct inline: `GameVector3(0.f, 1.f, 0.f)` is `constexpr` and free.
  - **Do not** add duplicate `GameVector3` constants (`g_upVec3`, `g_zeroVec3`). This would create a parallel constant namespace.
  - The legacy `g_upVector` / `g_zeroVector` globals remain usable throughout the migration via Phase 2 implicit conversions and are removed in Phase 5.
- Why: Single source of truth for constants. Existing `XMVECTORF32` constants are already available in every translation unit via `GameMath.h`.
- Dependencies: Step 1.1.
- Build impact: **None** — no changes needed.
- Risk: Low.

#### Phase 2 Verification

- Full rebuild of all projects.
- Runtime: game must run identically — no behavioral changes, only new code paths available.
- Unit test: verify round-trip conversion `LegacyVector3 → GameVector3 → LegacyVector3` preserves values. Same for matrices.

---

### Phase 3: Migrate Core Data Structures (headers, high impact)

**Goal**: Change the canonical member types in `WorldObject`, `Building`, and `Entity` from `LegacyVector3` / `Matrix34` to `GameVector3` / `GameMatrix`. This is the highest-risk phase because these headers are included everywhere.

> **Strategy**: Thanks to Phase 2's implicit conversions, changing a member from `LegacyVector3` to `GameVector3` is source-compatible for **field access**: any code that writes `obj.m_pos.x` still works (GameVector3 has public `x, y, z`), and any code that passes `obj.m_pos` to a function taking `LegacyVector3` still compiles via the implicit conversion operator.
>
> **However**, because `GameVector3` is a pure storage type with no arithmetic operators, any code that performs math directly on members (`m_pos + m_vel * dt`, `m_pos * otherVec` for dot) will fail to compile. These sites must be fixed in the same commit. The blast radius is manageable because:
> 1. Most `operator*`-as-dot sites use local variables, not members directly.
> 2. `WorldObject::m_pos` and `m_vel` are primarily read/written via assignment; arithmetic is done on locals.
> 3. Compiler errors at each site force the correct migration pattern (load → compute → store).

#### Step 3.0 — Pre-migration audit

- Action: Before touching any headers, do a codebase-wide search for:
  - `m_pos +`, `m_pos -`, `m_pos *`, `m_vel +`, `m_vel -`, `m_vel *` (direct arithmetic on members that will break).
  - `LegacyVector3 * LegacyVector3` patterns (dot product via `operator*`).
  - `LegacyVector3 ^ LegacyVector3` patterns (cross product via `operator^`).
- Produce a list of files and line counts. If the total break count per header change is < 50 sites, proceed with compiler-error-driven migration. If > 100, consider batching the header change with the `.cpp` fixups in a single commit per subsystem.

#### Step 3.1 — Migrate `WorldObject`

**File**: `GameLogic/worldobject.h` (modify)

- Action:
  - Replace `#include "LegacyVector3.h"` with `#include "GameVector3.h"` (or rely on pch).
  - Change `LegacyVector3 m_pos;` → `GameVector3 m_pos;`.
  - Change `LegacyVector3 m_vel;` → `GameVector3 m_vel;`.
  - Fix all `.cpp` files that perform arithmetic directly on `m_pos` / `m_vel` in the same commit. The pattern is:
    ```cpp
    // Before (LegacyVector3 operators):
    m_pos += m_vel * dt;
    float dot = m_pos * otherVec;
    LegacyVector3 cross = m_pos ^ otherVec;

    // After (XMVECTOR via Neuron::Math free functions):
    XMVECTOR pos = m_pos.Load();
    XMVECTOR vel = m_vel.Load();
    pos = XMVectorAdd(pos, XMVectorScale(vel, dt));
    m_pos = GameVector3(pos);
    float dot = Dotf(m_pos, otherVec);   // GameVector3 convenience overload
    GameVector3 cross = Cross(m_pos, otherVec); // GameVector3 convenience overload
    ```
- Why: `WorldObject` is the root base class; changing it propagates to all entities and buildings.
- Dependencies: Phase 2.
- Build impact: **High** — `worldobject.h` is transitively included by ~60+ files. Full rebuild required.
- Risk: **High** — the main hazard is that `GameVector3` has no operators, so any direct arithmetic on members breaks. This is **intentional** — each compile error forces the correct XMVECTOR migration pattern. The pre-migration audit (Step 3.0) bounds the blast radius.

#### Step 3.2 — Migrate `Building`

**File**: `GameLogic/building.h` (modify)

- Action:
  - Change `LegacyVector3 m_front;` → `GameVector3 m_front;`.
  - Change `LegacyVector3 m_up;` → `GameVector3 m_up;`.
  - Change `LegacyVector3 m_centrePos;` → `GameVector3 m_centrePos;`.
  - Replace `#include "LegacyVector3.h"` / `#include "matrix34.h"` with `#include "GameVector3.h"` / `#include "GameMatrix.h"`.
- Why: `Building` is the second most widely inherited type.
- Dependencies: Step 3.1.
- Build impact: **High** — triggers rebuild of all GameLogic and Starstrike building files.
- Risk: Medium (lower than 3.1 because patterns are the same).

#### Step 3.3 — Migrate `Camera`

**File**: `Starstrike/camera.h` (modify)

- Action: Change `LegacyVector3` members/params to `GameVector3`. Ensure `GetPos()`, `GetRight()`, `GetUp()`, `GetFront()` return `GameVector3`. Fix all arithmetic on camera members in `camera.cpp` in the same commit — camera math is concentrated in one file, making it a contained change.
  - `GetRight()` currently computes `m_up ^ m_front` — replace with `Cross(m_up, m_front)` returning `GameVector3`.
- Dependencies: Phase 2.
- Build impact: **High** — camera is used in many render files.
- Risk: Medium. Camera has ~15 `LegacyVector3` members (the highest density of any single header), but all arithmetic is in `camera.cpp`.

#### Phase 3 Verification

- Full rebuild of all projects (Debug x64, Release x64).
- Run game and verify all game systems still function: camera movement, building placement, entity movement, control tower reprogramming, rendering.
- Spot-check that `glVertex3fv(pos.GetData())` calls compile and produce correct visuals.

---

### Phase 4: File-by-File Migration of `.cpp` Files (many files, low per-file risk)

**Goal**: Convert individual `.cpp` files from using `LegacyVector3` / `Matrix34` local variables and function parameters to `GameVector3` / `GameMatrix`. This is the long tail and can be done incrementally by any team member.

#### Strategy

- Work through files alphabetically or by subsystem.
- For each file, apply the **load → compute → store** pattern:
  1. Replace `#include "LegacyVector3.h"` with `#include "GameVector3.h"` (if not already via pch).
  2. Replace `#include "matrix34.h"` / `#include "matrix33.h"` with `#include "GameMatrix.h"`.
  3. Replace local variable types: `LegacyVector3 foo(...)` → `GameVector3 foo(...)` for storage, or `XMVECTOR foo = ...` for temporaries used only in math.
  4. Replace `Matrix34 mat(...)` → `GameMatrix mat(...)` for storage, or `XMMATRIX mat = ...` for temporaries.
  5. Replace `vec ^ vec` → `Cross(a, b)` (free function in `Neuron::Math`).
  6. Replace `vec * vec` (dot) → `Dotf(a, b)` (free function in `Neuron::Math`).
  7. Replace `vec.Mag()` → `Length(vec)`, `vec.MagSquared()` → `LengthSquare(vec)`, `vec.Normalise()` → `Normalize(vec)`.
  8. Replace `mat.ConvertToOpenGLFormat()` → `float gl[16]; ToColumnMajor(mat, gl);`.
  9. Replace `mat.OrientFU(f, u)` → `mat = GameMatrix(OrientFU(f.Load(), u.Load()))`.
  10. Replace `mat * vec` / `vec * mat` → `TransformPoint(mat, vec)`.
  11. Replace `g_upVector` → `Neuron::Math::Vector3::UP` (for `XMVECTOR` contexts) or `GameVector3(0.f, 1.f, 0.f)` (for storage contexts).
  12. Replace `g_zeroVector` → `Neuron::Math::Vector3::ZERO` or `GameVector3()`.
  13. Replace `g_identityMatrix34` → `GameMatrix::Identity()`.
  14. For math-heavy functions, prefer loading to `XMVECTOR`/`XMMATRIX` at function entry, doing all computation in registers, and storing back once at the end. This is the natural pattern with pure storage types and gives SIMD benefits immediately.
  15. Compile file, fix errors, verify.

#### Step 4.1 — Migrate GameLogic `.cpp` files

Files (representative, not exhaustive):

| File | Key changes | Risk |
|---|---|---|
| `GameLogic/controltower.cpp` | Replace `LegacyVector3` locals, `Matrix34` locals, `g_upVector` usage. ~15 instances. | Low |
| `GameLogic/building.cpp` | Base class impl. | Low |
| `GameLogic/worldobject.cpp` | Base class impl. | Low |
| `GameLogic/entity.cpp` | Entity base. | Low |
| `GameLogic/darwinian.cpp` | Heavily uses vector math. | Medium |
| `GameLogic/engineer.cpp` | Uses `Matrix34` for placement. | Low |
| `GameLogic/armour.cpp` | Physics-heavy. | Medium |
| `GameLogic/centipede.cpp` | Complex entity math. | Medium |
| `GameLogic/bridge.cpp` | Geometry. | Low |
| ... | ~40+ more files in GameLogic | Low-Medium |

- Build impact per file: **Low** — only the modified `.cpp` recompiles.
- Risk per file: **Low** — each file is independent.

#### Step 4.2 — Migrate NeuronClient `.cpp` files

| File | Key changes | Risk |
|---|---|---|
| `NeuronClient/3d_sprite.cpp` | Render code with `LegacyVector3`. | Low |
| `NeuronClient/bitmap.cpp` | Minimal math. | Low |
| `NeuronClient/d3d12_backend.cpp` | May already use DirectXMath; verify. | Low |
| ... | ~10-15 files | Low |

#### Step 4.3 — Migrate Starstrike `.cpp` files

| File | Key changes | Risk |
|---|---|---|
| `Starstrike/camera.cpp` | Core camera math; many `LegacyVector3` ops. | Medium |
| `Starstrike/GameApp.cpp` | App setup. | Low |
| `Starstrike/clouds.cpp` | Render code. | Low |
| `Starstrike/explosion.cpp` | Particle positions. | Low |
| ... | ~15-20 files | Low-Medium |

#### Step 4.4 — Migrate `math_utils.h/.cpp`

**File**: `NeuronClient/math_utils.h` + `NeuronClient/math_utils.cpp` (modify)

- Action: Change all function signatures from `LegacyVector3`/`Matrix34` params to `GameVector3`/`GameMatrix`. Since implicit conversions exist (Phase 2), callers that haven't migrated yet still compile. Internally, the geometry functions should load to `XMVECTOR` at entry and work in registers — these are the highest-value SIMD targets since they contain tight loops (ray-tri, ray-sphere, etc.).
- Build impact: **High** — `math_utils.h` is included by many files. But since types are convertible, this is source-compatible.
- Risk: Medium — verify that the `NearlyEquals` macro (used in `math_utils.h`) still works with `GameVector3` fields. It operates on floats, so it should be fine.
- Dependencies: Phase 2 conversions must be in place.

#### Phase 4 Verification

- Incremental builds after each file.
- Full build after completing each subsystem batch.
- Run game and spot-check affected systems.

---

### Phase 5: Remove Legacy Types (4 files, medium risk)

**Goal**: Delete the old headers and implementation files once all consumers have migrated.

#### Step 5.1 — Verify no remaining legacy operator usage

**Files**: All `.cpp` files across GameLogic, NeuronClient, Starstrike.

- Action: Do a final codebase search for `operator*` between two vector types (dot product) and `operator^` (cross product). Since these operators exist only on `LegacyVector3` and were never added to `GameVector3`, any remaining usage will already be a compile error once the legacy headers are deleted. This step is a verification pass, not a code change.
- Dependencies: All Phase 4 files migrated.
- Build impact: **None** — verification only.

#### Step 5.2 — Delete legacy headers and implementations

**Files to delete**:
- `NeuronClient/LegacyVector3.h`
- `NeuronClient/vector3.cpp`
- `NeuronClient/matrix33.h`
- `NeuronClient/matrix33.cpp`
- `NeuronClient/matrix34.h`
- `NeuronClient/matrix34.cpp`

**Files to update**: Remove these from the NeuronClient project file (`.vcxproj` / `CMakeLists.txt`).

- Action: Delete files, remove from project, full rebuild, fix any remaining `#include` references.
- Dependencies: All Phase 4 migration complete.
- Build impact: **High** — full rebuild.
- Risk: Medium — any missed migration will now fail to compile. This is by design; the compiler errors guide remaining fixups.

#### Step 5.3 — Remove `g_upVector` / `g_zeroVector` globals

**File**: `NeuronClient/vector3.cpp` (deleted in 5.2) + any remaining references.

- Action: Search for `g_upVector` and `g_zeroVector` usage across the codebase. Replace with `Neuron::Math::Vector3::UP` / `Neuron::Math::Vector3::ZERO` (`XMVECTORF32`) for `XMVECTOR` contexts, or `GameVector3(0.f, 1.f, 0.f)` / `GameVector3()` for storage contexts.
- Dependencies: Step 5.2.
- Build impact: Covered by 5.2 rebuild.
- Risk: Low.

#### Step 5.4 — Remove `Vector2` ↔ `LegacyVector3` coupling

**File**: `NeuronClient/vector2.h` (modify)

- Action: Replace `LegacyVector3` forward declarations and conversion constructors with `GameVector3` equivalents.
- Dependencies: Step 5.2.
- Build impact: **Medium** — `vector2.h` is moderately included.
- Risk: Low.

#### Step 5.5 — Clean up `g_identityMatrix34`

**File**: `NeuronClient/matrix34.cpp` (deleted in 5.2) + references.

- Action: Replace `g_identityMatrix34` with `GameMatrix::Identity()` or a `constexpr` global in `GameMatrix.h`.
- Dependencies: Step 5.2.
- Build impact: Covered by 5.2 rebuild.
- Risk: Low.

#### Phase 5 Verification

- Full rebuild of all projects in all configurations.
- Run full game playthrough of at least one level to verify no regressions.
- Verify no remaining references to deleted files.

---

### Phase 6: Optimization Pass (ongoing, reduced scope)

**Goal**: With the pure-storage-type approach, Phase 4 migration naturally pushes math into `XMVECTOR`/`XMMATRIX` registers. Phase 6 is now focused on **batch-level** optimizations in identified hot loops, not per-site SIMD conversion (which already happened in Phase 4).

#### Step 6.1 — Profile hot loops

- Action: Profile the game to identify math-heavy hot spots (entity `Advance()` loops, particle systems, collision detection in `math_utils`).
- Use the Profiler Agent for measurement-driven work.

#### Step 6.2 — Batch SIMD in hot loops

- Action: In identified hot loops, hoist `Load()` calls out of inner loops and keep data in `XMVECTOR`/`XMMATRIX` across multiple operations. Since Phase 4 already uses `XMVECTOR` for computation, the main gain here is reducing load/store traffic by widening the register-resident scope:
  ```cpp
  // Phase 4 pattern (per-entity, already SIMD):
  for (auto& entity : entities) {
      XMVECTOR pos = entity.m_pos.Load();
      XMVECTOR vel = entity.m_vel.Load();
      pos = XMVectorMultiplyAdd(vel, XMVectorReplicate(dt), pos);
      entity.m_pos = GameVector3(pos);
  }

  // Phase 6 refinement (only if profiling shows benefit):
  // Batch multiple operations per entity before storing.
  ```
- Risk: Low per-change; requires profiling to verify improvement.

#### Step 6.3 — Consider SoA layout for entity arrays

- Action: For very hot paths (e.g., Darwinian AI with thousands of entities), consider Structure-of-Arrays layout where `x[]`, `y[]`, `z[]` are separate arrays. This is a larger architectural change and should only be done if profiling shows the AoS layout is a bottleneck.
- Risk: High — significant refactor; only if justified by profiling data.

---

## Build & Verification Strategy

| After Phase | Build Type | Runtime Test |
|---|---|---|
| Phase 1 | Full rebuild (all configs) | Unit tests for `GameVector3`, `GameMatrix` |
| Phase 2 | Full rebuild | Unit tests for round-trip conversions; game runs unchanged |
| Phase 3 | Full rebuild | Full gameplay test: camera, buildings, entities, control towers, rendering |
| Phase 4 (per batch) | Incremental | Spot-check affected systems |
| Phase 5 | Full rebuild | Full gameplay test; verify no dead includes |
| Phase 6 | Incremental | Profile before/after; verify no behavioral changes |

**Configurations to test**: Debug x64, Release x64.

## Data & Content Dependencies

- None. This is a code-only migration; no data files, assets, or scripts are affected.
- Serialization formats (e.g., `Building::Read` / `Building::Write` in text files) use `float` values and `atoi`/`printf`, which operate on scalar fields — these remain unchanged since `GameVector3` exposes `x`, `y`, `z`.

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| **`operator*` (dot) / `operator^` (cross) compile errors in Phase 3** | Blocks build until fixups applied | High | Pre-migration audit (Step 3.0) bounds the blast radius. These operators live only on `LegacyVector3`; they are never added to `GameVector3`. Compiler errors force migration to `Dotf()` / `Cross()` free functions. Fix sites in the same commit as the header change. |
| **No arithmetic operators on `GameVector3`** | More verbose code at migration sites | Medium | The `Neuron::Math` convenience overloads (`Length(GameVector3)`, `Dotf(GameVector3, GameVector3)`, etc.) reduce verbosity. For hot math, the load→compute→store pattern with `XMVECTOR` is the intended idiom and pays for itself in performance. |
| **`XMFLOAT3` / `float x,y,z` union UB** | Undefined behavior on non-MSVC compilers | Low (MSVC-only project) | Use `XMFLOAT3` as sole member and provide `x`/`y`/`z` as accessor references. Or use MSVC-supported anonymous struct union, matching `XMFLOAT3`'s own pattern. |
| **`ConvertToOpenGLFormat()` static buffer** | Thread-safety / re-entrancy issues (pre-existing) | Low | Replaced with `ToColumnMajor(mat, out[16])` writing to a caller-supplied array. No static buffer in the new API. |
| **Massive rebuild from header changes** | Lost developer time | High | Phase 2 and 3 each trigger full rebuilds. Schedule these for end-of-day or CI. Group header changes tightly. |
| **`Matrix34` row-major layout vs DirectXMath** | Incorrect transforms | Medium | DirectXMath uses row-major `XMFLOAT4X4`; `Matrix34` stores `{r, u, f, pos}` as row vectors. These are compatible. Verify with unit test that `TransformPoint(GameMatrix(front, up, pos), point)` matches `Matrix34(front, up, pos) * point` for known inputs. |
| **Both `mat * vec` and `vec * mat` compute the same result in legacy code** | Incorrect transforms if only one direction is migrated | Medium | Both legacy operators are identical (affine point transform). Replace all call sites with `TransformPoint(mat, vec)`. The asymmetry is eliminated. |
| **`Vector2` coupling** | Compile errors in `vector2.h` | Low | Fix in Phase 5 step 5.4; low risk because `Vector2` usage is limited. |
| **No `GameMatrix33` type** | Code that used `Matrix33` directly needs adaptation | Low | `Matrix33` usage is almost exclusively as the orientation part of `Matrix34`. Replace with `GameMatrix` (4×4 with translation zeroed) or with `GetOrientation(mat)`. The `Matrix33(r, u, f)` constructor maps to `GameMatrix(r, u, f, GameVector3())`. |

## Success Criteria

- [x] `GameVector3` (pure storage) and `GameMatrix` (pure storage) types defined in NeuronCore with DirectXMath backing.
- [x] All math operations consolidated as `Neuron::Math` free functions in `GameMath.h` — no math methods on storage types.
- [x] No `GameMatrix33` type — both `Matrix33` and `Matrix34` replaced by single `GameMatrix` (4×4).
- [ ] All `LegacyVector3`, `Matrix33`, `Matrix34` member variables in core types (`WorldObject`, `Building`, `Camera`) migrated to new types.
- [ ] All `.cpp` files in GameLogic, NeuronClient, and Starstrike use new types exclusively with load→compute→store pattern.
- [ ] Legacy headers (`LegacyVector3.h`, `matrix33.h`, `matrix34.h`) and implementations deleted.
- [ ] `g_upVector` / `g_zeroVector` / `g_identityMatrix34` globals replaced with existing `XMVECTORF32` constants and `GameMatrix::Identity()`.
- [ ] All build configurations compile and link cleanly with zero new warnings.
- [ ] Game runs identically — no visual, gameplay, or behavioral regressions.
- [ ] Unit tests pass for storage type construction, Load/Store round-trips, convenience overloads, `TransformPoint` correctness, `ToColumnMajor` correctness, and `Orient*` correctness.
- [ ] `math_utils.h` functions migrated to new types with XMVECTOR internals.

## Estimated Scope

| Phase | Files Changed | Effort | Risk |
|---|---|---|---|
| Phase 1 | 2 new + 1 modified | Small (1–2 days) | Low |
| Phase 2 | 3 modified | Small (1 day) | Medium |
| Phase 3 | 3 headers + associated `.cpp` fixups | Medium (2–3 days) | High |
| Phase 4 | ~60–80 `.cpp` files | Large (1–2 weeks) | Low per file |
| Phase 5 | ~8 deleted/modified | Small (1 day) | Medium |
| Phase 6 | Varies (profile-driven) | Ongoing | Low |

**Total estimated effort**: 2–3 weeks for a single developer, assuming no other priorities. Phase 4 is the long tail and can be parallelized or done opportunistically when touching files for other reasons. Phase 6 is reduced in scope because the pure-storage approach means Phase 4 migration naturally operates in XMVECTOR/XMMATRIX registers.

---

## Status

| Phase | Status | Notes |
|---|---|---|
| Phase 1 | ✅ Complete | `GameVector3.h`, `GameMatrix.h` created in NeuronCore. `GameMath.h` extended with convenience overloads (`Length`, `LengthSquare`, `Dotf`, `Cross`, `Normalize`, `SetLength`, `TransformPoint`, `TransformVector`, `GetOrientation`). Build passes. |
| Phase 2 | ✅ Complete | Implicit conversion operators added to `LegacyVector3` (↔ `GameVector3`), `Matrix34` (↔ `GameMatrix`), `Matrix33` (↔ `GameMatrix`). Build passes. Conversion bridge validated end-to-end via `SimEvent.md` Phase 2 (`GameSimEvent` fields migrated to new types; all ~38 producer call sites compile via implicit conversions). |
| Phase 3 | Not started | Migrate `WorldObject`, `Building`, `Camera` header fields. |
| Phase 4 | Not started | File-by-file `.cpp` migration. |
| Phase 5 | Not started | Remove legacy types. |
| Phase 6 | Not started | Optimization pass. |

### Remaining Phase 1 verification items

- [ ] Unit tests for `GameVector3` (construction, Load/Store round-trip, GetData, field access)
- [ ] Unit tests for `GameMatrix` (construction, Identity, row accessors, Load/Store round-trip)
- [ ] Unit tests for convenience overloads (Length, Dotf, Cross, Normalize, SetLength)
- [ ] Unit tests for `TransformPoint` / `TransformVector` matching legacy `Matrix34 * LegacyVector3`

### Remaining Phase 2 verification items

- [ ] Unit tests for round-trip conversion `LegacyVector3 → GameVector3 → LegacyVector3`
- [ ] Unit tests for round-trip conversion `Matrix34 → GameMatrix → Matrix34`
- [ ] Unit tests for round-trip conversion `Matrix33 → GameMatrix → Matrix33`
- [ ] In-game verification: game runs identically with no visual or behavioral regressions

### Date

2025-07-18
