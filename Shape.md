# Shape System Refactor: ShapeStatic / ShapeInstance Split

## ADR-002: Separate Shared Shape Geometry from Per-Instance Animation State

**Status**: Proposed  
**Status**: Implemented  
**Date**: 2025-07-24  
**Implemented**: 2025-07-26

---

## 1. Context & Problem Statement

The current `Shape` class conflates two concerns:

1. **Immutable geometry** — mesh data (positions, normals, colours, vertices, triangles), fragment tree hierarchy, marker definitions, bounding volumes. This is loaded once from disk and never modified.
2. **Per-instance mutable state** — fragment transforms (`m_transform`), angular/linear velocity (`m_angVel`, `m_vel`), and the world-space scratch buffer (`m_positionsInWS`) used during hit-testing.

Today, `Resource` exposes two APIs:

- **`GetShape`** — returns a shared cached `Shape*`. All callers share the same object. Safe only when no caller mutates fragment transforms.
- **`GetShapeCopy`** — loads a **full duplicate** from disk (geometry, names, tree, scratch buffers — everything), just so the caller can have independent `m_transform` / `m_angVel` state.

### Why This Is a Problem

- **Memory waste**: Each `GetShapeCopy` call duplicates all vertex, normal, colour, and triangle arrays. For a shape with 2,000 vertices across 10 fragments, that's ~100 KB of geometry duplicated per copy — only to get ~90 bytes of independent transform state per fragment.
- **Disk I/O on spawn**: Every `GetShapeCopy` call re-reads and re-parses the `.shp` file from disk, adding latency to entity construction.
- **Ownership ambiguity**: `GetShape` callers must never delete the `Shape*`; `GetShapeCopy` callers must. This is documented only in a comment and enforced by convention. `Building::m_shape` stores both kinds in the same field with no type distinction.
- **Marker coupling**: `ShapeMarker::GetWorldTransform` walks `m_parents[]`, an array of raw `ShapeFragment*` pointers. These point into the per-instance fragment tree. If a shape is shared, markers from one entity would read another entity's transforms — a subtle correctness bug prevented only by the copy.

---

## 2. Current State Analysis

### Classes Involved

| Class | File | Role |
|---|---|---|
| `Shape` | `NeuronClient/shape.h/.cpp` | Owns root `ShapeFragment*`, loads `.shp` files, delegates render/hit to root fragment |
| `ShapeFragment` | `NeuronClient/shape.h/.cpp` | Tree node. Owns geometry arrays, transform, velocity, scratch buffer. Recursive render/hit/lookup. |
| `ShapeMarker` | `NeuronClient/shape.h/.cpp` | Attached to a fragment. Stores `m_parents[]` (array of `ShapeFragment*`) for world-matrix computation. |
| `Resource` | `NeuronClient/resource.h/.cpp` | Cache of shared shapes (`m_shapes`). Provides `GetShape` and `GetShapeCopy`. |
| `Building` | `GameLogic/building.h/.cpp` | Base class storing `Shape* m_shape`. Most subclasses use `GetShape` (shared). |
| `Entity` | `GameLogic/entity.h/.cpp` | Base entity class storing `Shape* m_shape` (separate from `Building::m_shape`). All entities use `GetShape`. |
| `ThrowableWeapon` | `GameLogic/weapons.h` | Stores `Shape* m_shape`. Subclasses: `Grenade`, `AirStrikeMarker`, `ControllerGrenade`. Uses `GetShape`. |
| `Rocket` | `GameLogic/weapons.h` | Stores `Shape* m_shape`. Uses `GetShape`. |
| `Shockwave` | `GameLogic/weapons.h` | Stores `Shape* m_shape`. Uses `GetShape`. |
| `Missile` | `GameLogic/weapons.h` | Stores `Shape* m_shape` and `ShapeMarker* m_booster`. Uses `GetShape`. |
| `EntityLeg` | `GameLogic/entity_leg.h` | Stores `Shape* m_shapeUpper`, `Shape* m_shapeLower`. Uses `GetShape`. |
| `GlobalBuilding` | `Starstrike/global_world.h` | Stores `Shape* m_shape`. Uses `GetShape`. |
| `SphereWorld` | `Starstrike/global_world.h` | Stores `Shape* m_shapeOuter/Middle/Inner`. Uses `GetShape`. |
| `Teleport` | `GameLogic/teleport.h/.cpp` | Inherits `Building`. Overrides `SetShape` to call `LookupMarker("MarkerTeleportEntrance")`. Stores `ShapeMarker* m_entrance`. |
| `Renderer` | `Starstrike/renderer.cpp` | `MarkUsedCells(const ShapeFragment*, ...)` and `MarkUsedCells(const Shape*, ...)` read fragment tree directly. |

### ShapeFragment Data Breakdown

**Immutable after load (bulk of memory)**:
```
m_positions[]       — LegacyVector3 * numPositions
m_normals[]         — LegacyVector3 * numNormals
m_colours[]         — RGBAColour * numColours
m_vertices[]        — VertexPosCol * numVertices
m_triangles[]       — ShapeTriangle * numTriangles
m_name              — char*
m_parentName        — char*
m_center            — LegacyVector3
m_radius            — float
m_mostPositiveY     — float
m_mostNegativeY     — float
m_childFragments    — LList<ShapeFragment*>  (tree structure)
m_childMarkers      — LList<ShapeMarker*>    (tree structure)
```

**Mutable per-instance (~90 bytes per fragment)**:
```
m_transform         — Transform3D (64 bytes)  ← written by RadarDish (only external mutator)
m_angVel            — LegacyVector3 (12 bytes) ← set per-entity for render prediction
m_vel               — LegacyVector3 (12 bytes) ← set per-entity for render prediction
```

**Mutable scratch (variable size)**:
```
m_positionsInWS[]   — LegacyVector3 * numVertices  ← overwritten every hit-test call
```

### Call Sites Using `GetShapeCopy`

| Entity | File | Reason for Copy |
|---|---|---|
| `RadarDish` | `GameLogic/radardish.cpp:35` | Rotates `Dish` and `UpperMount` fragments independently per dish instance. Writes `m_angVel` and calls `m_transform.RotateAround()` every tick. Deletes `m_shape` in destructor. |

`GetShapeCopy` is also called internally by `GetShape` (line 163) to perform the initial load — but that path always passes `_animating = false` and caches the result.

### Call Sites Using `GetShape` (shared, representative sample)

| Entity | File |
|---|---|
| `Building` (base) | `building.cpp:74` — `s_controlPad` (static, shared across all buildings) |
| `Armour` | `armour.cpp:33` |
| `ResearchItem` | `researchitem.cpp:32` |
| `StaticShape` | `staticshape.cpp:55` |
| `DynamicBase` | `generichub.cpp:76` |
| `SoulDestroyer` | `souldestroyer.cpp` — `s_shapeHead`, `s_shapeTail` (static, shared) |
| `ConstructionYard` | `constructionyard.cpp:27-30` — `s_rung` (static, shared) |

All of these treat `m_shape` as read-only during render and hit-test. They pass an external `Matrix34` transform and never touch fragment `m_transform` or `m_angVel`.

---

## 3. Proposed Design

### 3.1 New Class Hierarchy

```
ShapeStatic                (shared, loaded once per .shp file)
├── ShapeFragmentData[]    (geometry, names, bounds, tree structure)
└── ShapeMarkerData[]      (name, parentName, depth, base transform)

ShapeInstance              (per-entity, lightweight)
├── ShapeStatic*           (back-pointer to shared data)
└── FragmentState[]        (flat array indexed by fragment ordinal)
     ├── Transform3D  transform
     ├── LegacyVector3 angVel
     └── LegacyVector3 vel
```

### 3.2 ShapeFragmentData (new — replaces immutable half of ShapeFragment)

```cpp
// Immutable geometry and metadata for one fragment. Owned by ShapeStatic.
// Shared by all ShapeInstances that reference the same ShapeStatic.
class ShapeFragmentData
{
public:
    // Geometry (loaded from disk, never modified)
    unsigned int        m_numPositions;
    LegacyVector3*      m_positions;
    unsigned int        m_numNormals;
    LegacyVector3*      m_normals;
    unsigned int        m_numColours;
    RGBAColour*         m_colours;
    unsigned int        m_numVertices;
    VertexPosCol*       m_vertices;
    unsigned int        m_numTriangles;
    ShapeTriangle*      m_triangles;

    // Identity / hierarchy
    char*               m_name;
    char*               m_parentName;
    Transform3D         m_baseTransform;    // As loaded from file (rest pose)

    // Bounding volume (computed once at load)
    LegacyVector3       m_center;
    float               m_radius;
    float               m_mostPositiveY;
    float               m_mostNegativeY;

    // Tree structure
    int                 m_fragmentIndex;    // Ordinal in ShapeStatic's flat array
    LList<ShapeFragmentData*> m_childFragments;
    LList<ShapeMarkerData*>   m_childMarkers;

    // Render / hit-test (operate on shared data + external per-instance state)
    void Render(const FragmentState* _states, float _predictionTime) const;
    void RenderSlow() const;    // Unchanged — only reads m_positions, m_normals, etc.

    bool RayHit(const FragmentState* _states, LegacyVector3* _wsScratch,
                RayPackage* _package, const Matrix34& _transform, bool _accurate) const;
    bool SphereHit(const FragmentState* _states, LegacyVector3* _wsScratch,
                   SpherePackage* _package, const Matrix34& _transform, bool _accurate) const;

    ShapeFragmentData* LookupFragment(const char* _name);
    ShapeMarkerData*   LookupMarker(const char* _name);

    void CalculateCenter(const FragmentState* _states, const Matrix34& _transform,
                         LegacyVector3& _center, int& _numFragments) const;
    void CalculateRadius(const FragmentState* _states, const Matrix34& _transform,
                         const LegacyVector3& _center, float& _radius) const;
};
```

### 3.3 FragmentState (new — per-instance mutable data)

```cpp
// Per-instance state for one fragment. Small, contiguous, cache-friendly.
struct FragmentState
{
    Transform3D     transform;  // 64 bytes — current animated transform
    LegacyVector3   angVel;     // 12 bytes — angular velocity for prediction
    LegacyVector3   vel;        // 12 bytes — linear velocity for prediction
    // Total: 88 bytes per fragment
};
```

### 3.4 ShapeMarkerData (renamed from ShapeMarker)

```cpp
class ShapeMarkerData
{
public:
    Matrix34        m_transform;
    char*           m_name;
    char*           m_parentName;
    int             m_depth;
    int*            m_parentIndices;    // Fragment ordinals, NOT raw pointers

    // Computes world matrix using fragment states instead of chasing pointers
    Matrix34    GetWorldMatrix(const FragmentState* _states,
                               const Matrix34& _rootTransform) const;
    Transform3D GetWorldTransform(const FragmentState* _states,
                                  const Transform3D& _rootTransform) const;
};
```

**Key change**: `m_parents` (array of `ShapeFragment*`) becomes `m_parentIndices` (array of `int`). World-matrix computation indexes into the `FragmentState[]` array passed by the caller, eliminating the pointer coupling that currently forces full shape copies.

### 3.5 ShapeStatic (replaces the shared-shape role of Shape)

```cpp
class ShapeStatic
{
public:
    ShapeFragmentData*  m_rootFragment;
    char*               m_name;
    int                 m_numFragments;     // Total fragment count (flat)

    ShapeStatic(const char* _filename);
    ShapeStatic(TextReader* _in);
    ~ShapeStatic();

    void Load(TextReader* _in);

    // Render/hit-test using default (rest-pose) transforms — for non-animating users
    void Render(float _predictionTime, const Matrix34& _transform) const;
    void XM_CALLCONV Render(float _predictionTime, XMMATRIX _transform) const;

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform,
                  const Matrix34& _ourTransform, bool _accurate = false) const;

    LegacyVector3 CalculateCenter(const Matrix34& _transform) const;
    float CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const;

    // Fragment/marker lookup by name (returns indices or data pointers)
    int                 GetFragmentIndex(const char* _name) const;
    ShapeFragmentData*  GetFragmentData(const char* _name) const;
    ShapeMarkerData*    GetMarkerData(const char* _name) const;

    // Marker world-matrix using rest-pose transforms (for non-animating callers)
    // ~45 call sites in the codebase use this — avoids forcing them to provide a FragmentState*.
    Matrix34    GetMarkerWorldMatrix(const ShapeMarkerData* _marker,
                                     const Matrix34& _rootTransform) const;
    Transform3D GetMarkerWorldTransform(const ShapeMarkerData* _marker,
                                        const Transform3D& _rootTransform) const;
};
```

When called without a `ShapeInstance`, `ShapeStatic` uses the `m_baseTransform` from each `ShapeFragmentData` (i.e., the rest pose loaded from the file). This covers all current `GetShape` users — they never modify transforms, so they don't need a `ShapeInstance` at all.

### 3.6 ShapeInstance (replaces GetShapeCopy for animating entities)

```cpp
class ShapeInstance
{
public:
    ShapeStatic*    m_master;           // Non-owning; Resource owns all masters
    FragmentState*  m_fragmentStates;   // Flat array [m_master->m_numFragments]

    explicit ShapeInstance(ShapeStatic* _master);
    ~ShapeInstance();

    // Non-copyable, movable (owns heap-allocated FragmentState[])
    ShapeInstance(const ShapeInstance&) = delete;
    ShapeInstance& operator=(const ShapeInstance&) = delete;
    ShapeInstance(ShapeInstance&& _other) noexcept;
    ShapeInstance& operator=(ShapeInstance&& _other) noexcept;

    // Access per-instance state by fragment index
    FragmentState&       GetState(int _fragmentIndex);
    const FragmentState& GetState(int _fragmentIndex) const;

    // Convenience: lookup + access combined
    FragmentState&       GetState(const char* _fragmentName);

    // Render/hit using this instance's transforms
    void Render(float _predictionTime, const Matrix34& _transform) const;
    void XM_CALLCONV Render(float _predictionTime, XMMATRIX _transform) const;

    bool RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate = false) const;
    bool ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform,
                  const Matrix34& _ourTransform, bool _accurate = false) const;

    // Marker world-matrix using this instance's fragment transforms
    Matrix34    GetMarkerWorldMatrix(const ShapeMarkerData* _marker,
                                     const Matrix34& _rootTransform) const;
    Transform3D GetMarkerWorldTransform(const ShapeMarkerData* _marker,
                                        const Transform3D& _rootTransform) const;

    LegacyVector3 CalculateCenter(const Matrix34& _transform) const;
    float CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const;

    // Access the master for geometry queries
    ShapeStatic* GetMaster() const { return m_master; }
};
```

### 3.7 Resource API Changes

```cpp
class Resource
{
    // ...existing members...
    HashTable<ShapeStatic*> m_shapes;   // Was HashTable<Shape*>

public:
    // Shared geometry — replaces GetShape.
    // Returns cached master; loads from disk on first call.
    ShapeStatic* GetShapeStatic(const char* _name);

    // Deprecated compatibility wrappers (Phase 2 removal)
    // Shape* GetShape(const char* _name);       // REMOVED
    // Shape* GetShapeCopy(const char* _name);   // REMOVED
};
```

### 3.8 World-Space Scratch Buffer Strategy

Today each `ShapeFragment` owns a permanent `m_positionsInWS[]` array sized to `m_numVertices`. This is only used inside `RayHit` and `SphereHit` and is overwritten every call.

Options:
1. **Thread-local arena** — allocate from a per-thread linear allocator, reset each frame. Zero overhead when no hit-tests occur.
2. **Pass-in buffer** — `ShapeInstance` owns a single `LegacyVector3*` buffer sized to `max(numVertices)` across all fragments. Reused per hit-test call.
3. **Allocate on master** — keep the scratch buffer in `ShapeFragmentData` (as today). Acceptable only because hit-tests are not concurrent.

**Recommendation**: Option 3 for the initial refactor (zero migration risk for hit-tests), with a `// TODO: move to per-thread arena` annotation. Option 1 in a follow-up pass.

---

## 4. Migration Plan

### Phase 1: Introduce New Types Alongside Old (Non-Breaking)

**Files to create**:
- `NeuronClient/ShapeStatic.h` / `NeuronClient/ShapeStatic.cpp`
- `NeuronClient/ShapeInstance.h` / `NeuronClient/ShapeInstance.cpp`

**Changes to existing files**:
- `NeuronClient/shape.h` — Add `ShapeFragmentData`, `ShapeMarkerData`, `FragmentState`. Keep old `Shape`, `ShapeFragment`, `ShapeMarker` intact.
- `NeuronClient/resource.h/.cpp` — Add `GetShapeStatic`. Keep `GetShape` and `GetShapeCopy` working (delegating internally to `ShapeStatic`).

**Goal**: Both old and new APIs compile and work. No gameplay code changes yet.

### Phase 2: Mechanical `GetShape` → `GetShapeStatic` Type Migration

**Rationale for doing this before RadarDish**: RadarDish inherits `Teleport` → `Building`. Its constructor calls `SetShape(...)`, which writes to `Building::m_shape`. If `Building::m_shape` is still `Shape*` when we try to give RadarDish a `ShapeStatic*`, we need awkward adapters. By migrating the base types first, Phase 3 can cleanly use `ShapeStatic*` everywhere.

This phase is a large (~30-35 files) but mechanical find-and-replace. `ShapeStatic` mirrors the read-only API surface of `Shape` exactly, so most call sites compile with just a type substitution.

#### 2a. Building Hierarchy

| Change | Scope |
|---|---|
| `Building::m_shape` type | `Shape*` → `ShapeStatic*` |
| `Building::SetShape` | Takes `ShapeStatic*` instead of `Shape*` |
| `Building::s_controlPad` | `Shape*` → `ShapeStatic*` |
| `Building::s_controlPadStatus` | `ShapeMarker*` → `ShapeMarkerData*` |
| `Building::m_lights` | `LList<ShapeMarker*>` → `LList<ShapeMarkerData*>` |
| `BuildingPort::m_marker` | `ShapeMarker*` → `ShapeMarkerData*` |
| `Building::SetShapeLights` | `ShapeFragment*` → `const ShapeFragmentData*` |
| `Building::SetShapePorts` | `ShapeFragment*` → `const ShapeFragmentData*` |
| `Building::DoesShapeHit` | `Shape*` → `ShapeStatic*` (virtual, ~10 overrides) |
| `Building::Render` | Calls `m_shape->Render(...)` — same API, `ShapeStatic::Render` uses rest-pose |
| `Teleport::SetShape` | Override signature: `Shape*` → `ShapeStatic*` |
| `Teleport::m_entrance` | `ShapeMarker*` → `ShapeMarkerData*` |
| All `g_app->m_resource->GetShape(...)` calls in buildings | → `g_app->m_resource->GetShapeStatic(...)` |
| Static shapes (`s_shapeHead`, `s_rung`, etc.) | `Shape*` → `ShapeStatic*` — type change only |

#### 2b. Entity Hierarchy

| Change | Scope |
|---|---|
| `Entity::m_shape` | `Shape*` → `ShapeStatic*` |
| `EntityLeg::m_shapeUpper`, `m_shapeLower` | `Shape*` → `ShapeStatic*` |
| `EntityLeg::m_rootMarker` | `ShapeMarker*` → `ShapeMarkerData*` |
| All entity `GetShape` calls | → `GetShapeStatic` |

#### 2c. Weapons

| Change | Scope |
|---|---|
| `ThrowableWeapon::m_shape` | `Shape*` → `ShapeStatic*` |
| `Rocket::m_shape` | `Shape*` → `ShapeStatic*` |
| `Shockwave::m_shape` | `Shape*` → `ShapeStatic*` |
| `Missile::m_shape` | `Shape*` → `ShapeStatic*` |
| `Missile::m_booster` | `ShapeMarker*` → `ShapeMarkerData*` |

#### 2d. Global World

| Change | Scope |
|---|---|
| `GlobalBuilding::m_shape` | `Shape*` → `ShapeStatic*` |
| `SphereWorld::m_shapeOuter/Middle/Inner` | `Shape*` → `ShapeStatic*` |

#### 2e. Renderer

| Change | Scope |
|---|---|
| `Renderer::MarkUsedCells(const ShapeFragment*, ...)` | → `MarkUsedCells(const ShapeFragmentData*, ...)` |
| `Renderer::MarkUsedCells(const Shape*, ...)` | → `MarkUsedCells(const ShapeStatic*, ...)` |

#### 2f. Explosion System

| Change | Scope |
|---|---|
| `Explosion` constructor | `ShapeFragment*` → `const ShapeFragmentData*` + optional `const FragmentState*` (nullable — use `m_baseTransform` when null) |
| `ExplosionManager::AddExplosion(ShapeFragment*, ...)` | → `AddExplosion(const ShapeFragmentData*, const FragmentState*, ...)` |
| `ExplosionManager::AddExplosion(const Shape*, ...)` | → `AddExplosion(const ShapeStatic*, ...)` |

Concrete new signatures:
```cpp
// explosion.h
Explosion(const ShapeFragmentData* _frag, const FragmentState* _state,
          const Matrix34& _transform, float _fraction);
// _state: nullable. When null, uses _frag->m_baseTransform. When non-null, uses _state->transform.

void ExplosionManager::AddExplosion(const ShapeFragmentData* _frag, const FragmentState* _state,
                                     const Matrix34& _transform, bool _recurse, float _fraction);
void ExplosionManager::AddExplosion(const ShapeStatic* _shape, const Matrix34& _transform,
                                     float _fraction = 1.0f);
```

#### 2g. ShapeMarker → ShapeMarkerData Call Sites (~45 GetWorldMatrix calls)

All `ShapeMarker*` member fields become `ShapeMarkerData*`. All `marker->GetWorldMatrix(rootMat)` calls become `m_shape->GetMarkerWorldMatrix(marker, rootMat)` (using the rest-pose convenience method on `ShapeStatic`). This is the most numerous change but each is a one-line substitution.

**Representative examples**:
```cpp
// Before (building.cpp — SetShapePorts)
port->m_mat = marker->GetWorldMatrix(buildingMat);

// After
port->m_mat = m_shape->GetMarkerWorldMatrix(marker, buildingMat);

// Before (radardish.cpp — SetDetail, still on Building::m_shape at this point)
Matrix34 worldMat = m_entrance->GetWorldMatrix(rootMat);

// After
Matrix34 worldMat = m_shape->GetMarkerWorldMatrix(m_entrance, rootMat);
```

### Phase 3: Migrate `GetShapeCopy` Call Sites to `ShapeInstance`

**Prerequisite**: Phase 2 is complete — `Building::m_shape` is `ShapeStatic*`, `Teleport::SetShape` takes `ShapeStatic*`, all `ShapeMarker*` members are `ShapeMarkerData*`.

Migrate entities that currently call `GetShapeCopy` to use `ShapeInstance`:

| Entity | Current | After |
|---|---|---|
| `RadarDish` | `m_shape = GetShapeCopy("radardish.shp", true)` + `delete m_shape` in dtor | `m_shapeInstance = ShapeInstance(GetShapeStatic("radardish.shp"))`. Stores fragment indices for `Dish` and `UpperMount`. Writes `m_shapeInstance.GetState(idx)` instead of `m_dish->m_angVel`. No manual delete needed — instance is a member, destroyed with the entity. |

**Detailed migration for RadarDish**:

Before (after Phase 2 type migration):
```cpp
// radardish.h
ShapeFragment* m_dish;          // ← still raw fragment pointer
ShapeFragment* m_upperMount;
ShapeMarkerData* m_focusMarker; // ← already changed in Phase 2

// radardish.cpp constructor
ShapeStatic* radarShape = g_app->m_resource->GetShapeCopy("radardish.shp", true); // still uses copy!
SetShape(radarShape);
m_dish       = m_shape->m_rootFragment->LookupFragment("Dish");
m_upperMount = m_shape->m_rootFragment->LookupFragment("UpperMount");
m_focusMarker = m_shape->GetMarkerData("MarkerFocus");

// radardish.cpp Advance
m_upperMount->m_angVel.y = signf(rotationAxis.y) * sqrtf(fabsf(rotationAxis.y));
m_dish->m_angVel.Zero();
m_upperMount->m_transform.RotateAround(...);
m_dish->m_transform.RotateAround(...);

// radardish.cpp destructor
delete m_shape;
m_shape = nullptr;
```

After:
```cpp
// radardish.h
ShapeInstance    m_shapeInstance; // Per-instance animation state (move-only, see §3.6)
int              m_dishIndex;
int              m_upperMountIndex;
ShapeMarkerData* m_focusMarker;

// radardish.cpp constructor
ShapeStatic* master = g_app->m_resource->GetShapeStatic("radardish.shp");
SetShape(master);                                       // Sets Building::m_shape (shared)
m_shapeInstance = ShapeInstance(master);                 // Move-assigns; allocates FragmentState[]
m_dishIndex       = master->GetFragmentIndex("Dish");
m_upperMountIndex = master->GetFragmentIndex("UpperMount");
m_focusMarker     = master->GetMarkerData("MarkerFocus");

// radardish.cpp Advance
m_shapeInstance.GetState(m_upperMountIndex).angVel.y = signf(rotationAxis.y) * ...;
m_shapeInstance.GetState(m_dishIndex).angVel.Zero();
m_shapeInstance.GetState(m_upperMountIndex).transform.RotateAround(...);
m_shapeInstance.GetState(m_dishIndex).transform.RotateAround(...);

// radardish.cpp destructor
// Nothing — ShapeInstance is a value member, destroyed automatically.
// Building::m_shape is non-owning; Resource owns all masters.
```

For rendering and hit-testing, `RadarDish` overrides `Render` to call `m_shapeInstance.Render(...)` instead of `m_shape->Render(...)`. Similarly for `DoesShapeHit` — uses `m_shapeInstance.ShapeHit(...)` to include animated transforms.

For marker world-matrix calls in RadarDish (e.g., `SetDetail`), use `m_shapeInstance.GetMarkerWorldMatrix(m_entrance, rootMat)` instead of `m_shape->GetMarkerWorldMatrix(m_entrance, rootMat)` to pick up animated transforms.

### Phase 4: Remove Old Types

- Delete `Shape`, `ShapeFragment`, `ShapeMarker` classes from `shape.h`/`shape.cpp`.
- Remove `GetShape`, `GetShapeCopy` from `Resource`.
- Drop `Shape::m_animating` — this field is dead code (set in constructors but never read anywhere).
- Audit `ShapeExporter` (declared `friend class` of `ShapeFragment` at `shape.h:97`). If it's part of the asset pipeline, update it to use `ShapeFragmentData`. If it's unused, remove it.
- Rename `ShapeStatic` → `Shape` if desired (optional, for familiarity).

### Phase 5: Scratch Buffer Improvement (Optional Follow-Up)

- Move `m_positionsInWS` out of `ShapeFragmentData` into a per-thread linear arena.
- Allocate on demand in `RayHit` / `SphereHit`, reset at end of frame.

---

## 5. Building / Teleport Adaptation

### 5.1 Building Base Class

`Building` currently stores `Shape* m_shape` and uses it for:
1. Rendering: `m_shape->Render(_predictionTime, mat)` — read-only, uses rest pose.
2. Hit-testing: `m_shape->RayHit(...)` / `DoesShapeHit(...)` — read-only.
3. Marker/port discovery: `m_shape->m_rootFragment->LookupMarker(...)` — read-only.
4. Light discovery: `SetShapeLights(m_shape->m_rootFragment)` — stores `ShapeMarker*` into `m_lights`.
5. Port discovery: `SetShapePorts(m_shape->m_rootFragment)` — stores `ShapeMarker*` into `BuildingPort::m_marker`, calls `marker->GetWorldMatrix(buildingMat)`.
6. Center/radius: `m_shape->CalculateCenter(mat)` — read-only.

All of these are satisfied by `ShapeStatic*` with no `ShapeInstance` needed. The `SetShapeLights`/`SetShapePorts` recursion switches from `ShapeFragment*` to `const ShapeFragmentData*`, and marker world-matrix calls become `m_shape->GetMarkerWorldMatrix(marker, buildingMat)`.

```cpp
class Building : public WorldObject
{
public:
    ShapeStatic* m_shape;                   // Shared geometry (was Shape*)
    LList<ShapeMarkerData*> m_lights;       // Was LList<ShapeMarker*>
    LList<BuildingPort*> m_ports;

    static ShapeStatic* s_controlPad;       // Was Shape*
    static ShapeMarkerData* s_controlPadStatus; // Was ShapeMarker*

    virtual void SetShape(ShapeStatic* _shape);
    void SetShapeLights(const ShapeFragmentData* _fragment);
    void SetShapePorts(const ShapeFragmentData* _fragment);

    virtual bool DoesShapeHit(ShapeStatic* _shape, Matrix34 _transform);
    // ...rest unchanged...
};

class BuildingPort
{
public:
    ShapeMarkerData* m_marker;              // Was ShapeMarker*
    // ...rest unchanged...
};
```

### 5.2 Teleport Override

`Teleport::SetShape` overrides `Building::SetShape` and does:
```cpp
void Teleport::SetShape(ShapeStatic* _shape)   // Was Shape*
{
    Building::SetShape(_shape);
    m_entrance = _shape->GetMarkerData("MarkerTeleportEntrance");
    // Was: m_shape->m_rootFragment->LookupMarker(...)
}
```

`Teleport::m_entrance` changes from `ShapeMarker*` to `ShapeMarkerData*`. Since `RadarDish` inherits from `Teleport`, its `SetShape(master)` call in Phase 3 chains through correctly.

### 5.3 RadarDish (Animating Subclass)

For the special case of `RadarDish` (and any future animating buildings), the subclass additionally stores a `ShapeInstance` member and overrides `Render` and `DoesShapeHit` to use the instance's animated transforms.

```cpp
class RadarDish : public Teleport    // Teleport : Building
{
    ShapeInstance m_shapeInstance;   // Per-instance animation state (move-only)
    int m_dishIndex;
    int m_upperMountIndex;
    ShapeMarkerData* m_focusMarker;
    // ...
};
```

`RadarDish::Render` calls `m_shapeInstance.Render(...)` instead of `m_shape->Render(...)`.
`RadarDish::SetDetail` calls `m_shapeInstance.GetMarkerWorldMatrix(m_entrance, rootMat)` to use animated transforms for the entrance position.

---

## 6. Trade-Off Summary

| Dimension | Before (GetShapeCopy) | After (ShapeInstance) |
|---|---|---|
| **Memory per animated entity** | Full geometry clone (~100 KB+) | ~90 bytes × numFragments | 
| **Disk I/O on spawn** | Full file re-parse | Zero (master already cached) |
| **Ownership model** | Ambiguous (`Shape*` may or may not be owned) | Clear: `Resource` owns masters, entities own instances |
| **Marker world-matrix** | Chases `ShapeFragment*` pointers | Indexes into `FragmentState[]` via ordinals |
| **Render indirection** | Direct `m_positions` access | Same — `ShapeFragmentData` has identical layout |
| **Code complexity** | One class does everything | Two classes with clear responsibilities |
| **Migration cost** | N/A | Medium-large — ~30-35 files touched, mostly mechanical type substitution |

---

## 7. Risks & Mitigations

| Risk | Mitigation |
|---|---|
| Fragment index goes stale if master is reloaded | Masters are loaded once and never replaced during a session. Add `DEBUG_ASSERT` in `GetState` for bounds checking. |
| `Explosion` reads `m_transform` from fragments at construction time | `Explosion` constructor receives `const ShapeFragmentData*` + optional `const FragmentState*` (nullable). For shared shapes exploding at rest pose, pass `nullptr` and use `m_baseTransform`. See Phase 2f for concrete signatures. |
| Concurrent hit-tests on shared `m_positionsInWS` scratch buffer | Currently not concurrent (single-threaded game loop). Document this constraint. Add `DEBUG_ASSERT(!m_inHitTest)` re-entrancy guard. Address in Phase 5 with per-thread arenas if multithreading is added. |
| `ShapeMarker::m_parents` is an array of raw `ShapeFragment*` | Replaced by `m_parentIndices` (int array). This is the most invasive single change but eliminates the root cause of the copy requirement. |
| `ShapeInstance` copy semantics cause double-free | Copy constructor and copy assignment are explicitly deleted. Move constructor and move assignment are provided (see §3.6). |
| `ShapeExporter` is a friend of `ShapeFragment` | Audit `ShapeExporter` usage during Phase 4. If it's part of the asset pipeline, update to use `ShapeFragmentData`. |
| ~45 `GetWorldMatrix` call sites need signature change | Non-animating callers use `ShapeStatic::GetMarkerWorldMatrix(marker, rootMat)` — a one-line substitution per site. Animating callers (RadarDish) use `ShapeInstance::GetMarkerWorldMatrix`. |
| `m_maxTriangles` not in `ShapeFragmentData` | `m_maxTriangles` is only used as a dynamic-array capacity during parsing; after load it equals `m_numTriangles`. Intentionally omitted from `ShapeFragmentData` — no migration needed. |

---

## 8. Verification Checklist

- [ ] All `GetShapeCopy` call sites migrated to `ShapeInstance`
- [ ] All `GetShape` call sites migrated to `GetShapeStatic`
- [ ] All `Shape*` members migrated to `ShapeStatic*` (Building, Entity, weapons, EntityLeg, GlobalBuilding, SphereWorld)
- [ ] All `ShapeMarker*` members migrated to `ShapeMarkerData*` (~20 fields across codebase)
- [ ] All ~45 `GetWorldMatrix`/`GetWorldTransform` call sites updated
- [ ] `RadarDish` dish rotation works identically (visual + gameplay)
- [ ] `FeedingTube` marker lookups work correctly with `ShapeMarkerData`
- [ ] `Teleport::SetShape` override compiles and `m_entrance` correctly resolved
- [ ] `ShapeMarker::GetWorldMatrix` produces identical results via index-based lookup
- [ ] `Explosion` fragments spawn correctly from both shared and instanced shapes
- [ ] Hit-testing (`RayHit`, `SphereHit`, `ShapeHit`) produces identical results
- [ ] `Building::DoesShapeHit` and all overrides compile and work with `ShapeStatic*`
- [ ] `Building::SetShapeLights`/`SetShapePorts` work with `ShapeFragmentData`
- [ ] `Building::SetDetail` port/light discovery works with `ShapeFragmentData`
- [ ] `Renderer::MarkUsedCells` works with `ShapeFragmentData`/`ShapeStatic`
- [ ] `FlushOpenGlState` / `RegenerateOpenGlState` handle `ShapeStatic` correctly
- [ ] `ShapeExporter` audited and updated or removed (Phase 4)
- [ ] `Shape::m_animating` dead code removed (Phase 4)
- [ ] No `Shape*`, `ShapeFragment*`, or `ShapeMarker*` types remain in the codebase (Phase 4)
- [ ] Debug/Release × all platforms build clean
- [ ] Memory usage decreases (verify with profiler for a level with multiple `RadarDish` entities)
