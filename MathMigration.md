# Math Type Migration Plan

## Goal

Replace **all** legacy and custom math types with raw DirectXMath types, eliminate the
OpenGL translation layer, and converge on a single math convention:

| Role | Type |
|---|---|
| **Storage** (struct members, serialization, arrays) | `XMFLOAT3`, `XMFLOAT4X4` |
| **Computation** (SIMD registers, function parameters) | `XMVECTOR`, `XMMATRIX` |
| **Domain operations** (orient, decompose, rotate-around, transform-point) | `Neuron::Math` free functions in `GameMath.h` |
| **Constants** | `Neuron::Math::Vector3::UP/ZERO/FORWARD` (`XMVECTORF32`) |
| **Matrix convention** | DirectX row-major, row-vector (`mul(v, M)` in HLSL) |

**Types to eliminate:**

| Type | Project | Role | Replacement |
|---|---|---|---|
| `LegacyVector3` | NeuronClient | 3-float vector (scalar) | `XMFLOAT3` |
| `Matrix33` | NeuronClient | 3×3 rotation (scalar) | `XMFLOAT4X4` (translation zeroed) |
| `Matrix34` | NeuronClient | 4×3 affine transform (scalar) | `XMFLOAT4X4` |
| `Transform3D` | NeuronCore | Row-major matrix wrapper | `XMFLOAT4X4` + `Neuron::Math` free functions |
| `GameMatrix` | NeuronCore | Transitional matrix storage | `XMFLOAT4X4` |
| `GameVector3` | NeuronCore | Transitional vector storage | `XMFLOAT3` |

---

## Current State Analysis

### Two Matrix Ecosystems

| Property | Legacy (NeuronClient) | Modern (NeuronCore) |
|---|---|---|
| Types | `Matrix33`, `Matrix34` | `XMMATRIX`, `XMFLOAT4X4` |
| Vector Type | `LegacyVector3` | `XMVECTOR` |
| Header | `matrix33.h`, `matrix34.h` | `GameMath.h` (wraps DirectXMath) |
| Storage | `LegacyVector3 r, u, f [, pos]` | `XMFLOAT4X4` (16-float row-major) |
| SIMD | None | SSE2 via `XMVECTOR` / `XMMATRIX` |
| Users | GameLogic (14 files), NeuronClient (shapes, math) | NeuronCore (GameMath.h), shaders |

### Include Chain (All Projects)

```
pch.h
 └─ NeuronClient.h (or NeuronServer.h)
     └─ NeuronCore.h
         ├─ GameMath.h          ← DirectXMath wrappers (already available everywhere)
         │   ├─ Transform3D.h   ← transitional (to be deleted)
         │   ├─ GameVector3.h   ← transitional (to be deleted)
         │   └─ GameMatrix.h    ← transitional (to be deleted)
         └─ ...
     └─ opengl_directx.h       ← GL shim macros
     └─ ...
```

Legacy headers are included **per-file** (`#include "LegacyVector3.h"`, etc.), not via `pch.h`.

### Data Layout — The Key Insight

`Matrix34` stores four `LegacyVector3` members sequentially: `r` (right axis), `u` (up
axis), `f` (forward axis), `pos` (translation). The members represent the **column
vectors** of the mathematical transformation matrix — they encode where each local
basis axis maps to in world space.

`ConvertToOpenGLFormat` packs these into a flat `float[16]`:

```
[r.x, r.y, r.z, 0,  u.x, u.y, u.z, 0,  f.x, f.y, f.z, 0,  pos.x, pos.y, pos.z, 1]
```

This byte pattern is simultaneously valid as:

- **OpenGL column-major** (columns = r, u, f, pos) — the original intent.
- **DirectX row-major** (rows = r, u, f, pos) — how the DX12 backend already
  consumes it.

Because the memory layout is identical under both interpretations, the translation
layer's `glMultMatrixf` performs a raw `memcpy` into `XMFLOAT4X4` with no transpose.
The HLSL vertex shader declares `row_major float4x4 WorldMatrix` and uses
`mul(vertex, WorldMatrix)` (row-vector convention), producing the correct
transformation.

**Bottom line: the data is already row-major-compatible.** The migration is about
removing the OpenGL conceptual framing, the translation-layer indirection, and the
thread-unsafe static buffer — not about transposing matrices.

### Operator Conventions (Post Phase 2)

| Operation | Formula | Convention |
|---|---|---|
| `Matrix33 * vec` | `dot(row_i, v)` for each row | M·v (column-vector, inverse rotation) |
| `vec * Matrix33` | `v.x*r_i + v.y*u_i + v.z*f_i` for each component | v·M (row-vector, forward rotation) ✓ |
| `Matrix34 * vec` | `d = v − pos; { r·d, u·d, f·d }` | Inverse affine (world-to-local) ✓ |
| `vec * Matrix34` | `v.x*r + v.y*u + v.z*f + pos` | v·M (row-vector, forward transform) ✓ |
| `Matrix33 * Matrix33` | Standard row-major product | Row-major ✓ |
| `Matrix34 *= Matrix34` | Standard row-major product + position | Row-major ✓ |
| `MatrixStack::Multiply` | `XMMatrixMultiply(mat, current)` | Pre-multiply (= DX local-transform-first) ✓ |
| `Transform3D * Transform3D` | `XMMatrixMultiply(lhs, rhs)` | Row-major ✓ |
| Shader | `mul(v, WorldMatrix)` | Row-vector ✓ |

### Rendering Pipeline (Target End-State)

```
Building/Entity
  │  stores (m_front, m_up, m_pos) as XMFLOAT3         ← target (currently LegacyVector3)
  ▼
Building::Render()
  │  XMMATRIX mat = OrientFU(                           ← Neuron::Math free function
  │      XMLoadFloat3(&m_front), Vector3::UP);
  │  mat.r[3] = XMVectorSetW(XMLoadFloat3(&m_pos), 1.f);
  │  m_shape->Render(predictionTime, mat);               ← Shape::Render(XMMATRIX)
  ▼
Shape::Render(predictionTime, XMMATRIX transform)
  │  mv.Push();
  │  mv.Multiply(transform);                    ← MatrixStack::Multiply(XMMATRIX)
  │  m_rootFragment->Render(predictionTime);
  │  mv.Pop();
  ▼
ShapeFragment::Render(predictionTime)            (recursive)
  │  XMFLOAT4X4 predicted = m_transform;         ← m_transform is XMFLOAT4X4
  │  RotateAround(predicted, XMVectorScale(...)); ← Neuron::Math free function
  │  Translate(predicted, XMVectorScale(...));     ← Neuron::Math free function
  │  mv.Push();
  │  mv.Multiply(XMLoadFloat4x4(&predicted));     ← load to XMMATRIX for stack
  │  RenderSlow();                               ← still immediate-mode glBegin/glEnd
  │  recurse children
  │  mv.Pop();
  ▼
uploadAndBindConstants()
  │  XMStoreFloat4x4(&cb.WorldMatrix, stack.GetTopXM());
  │  memcpy(ringBuffer, &cb, sizeof(cb));
  │  cmdList->SetGraphicsRootConstantBufferView(0, gpuAddr);
  ▼
VertexShader.hlsl
  │  row_major float4x4 WorldMatrix;
  │  float4 worldPos = mul(float4(pos, 1), WorldMatrix);   ← row-vector multiply
  ▼
Correct result
```

### Problems to Solve

| # | Issue | Impact | Status |
|---|---|---|---|
| P1 | `ConvertToOpenGLFormat` relies on `static float m_openGLFormat[16]` — **not thread-safe**, returns pointer to shared mutable state | Data corruption if two matrices convert in overlapping scopes | ✅ Fixed (Phase 0) |
| P2 | OpenGL-named functions (`glMultMatrixf`, `glPushMatrix`, etc.) obscure that the backend is DX12; every matrix operation pays an indirection through the translation layer | Readability, performance, maintenance burden | ✅ Fixed (Phase 1) — zero `gl*` matrix callers remain outside `opengl_directx.cpp` |
| P3 | `operator*(Matrix33, vec)` and `operator*(vec, Matrix33)` return **identical results** — the convention is ambiguous | Confusing; bugs if someone relies on mathematical ordering | ✅ Fixed (Phase 2) |
| P4 | `Matrix34 * vec` implements forward transform but reads as column-vector notation `M·v` — misleading | Confusing; hides the actual row-vector convention | ✅ Fixed (Phase 2) — now inverse affine |
| P5 | No SIMD — Matrix33/34 use scalar float arithmetic | Missed performance opportunity for hierarchy composition | ⚠️ Partially fixed — `GetWorldTransform` and `ShapeFragment::Render` use SIMD; remaining scalar sites depend on Phase A+ |
| P6 | Legacy types force every render site to construct `Matrix34` from raw vectors and call `ConvertToOpenGLFormat` | Boilerplate, error-prone | ⚠️ Partially fixed — `ConvertToOpenGLFormat` eliminated; `Matrix34` construction remains in ~60 GameLogic/GameRender files |
| P7 | `Matrix33` and `Matrix34` live in NeuronClient but are `#include`d by GameLogic (14+ files) — cross-project dependency on a rendering-era type | Architectural layering violation | Pending (Phase C–D) |
| P8 | Three custom wrapper types exist around DirectXMath storage: `Transform3D`, `GameMatrix`, `GameVector3`. All are thin wrappers over `XMFLOAT4X4`/`XMFLOAT3` with 1–8 line delegations to DirectXMath — unnecessary indirection | Confusion about which type to use; extra conversions; duplicated concepts; maintenance burden | ⚠️ New — see Design Decisions |

---

## Design Decisions

### Eliminate All Custom Math Types — Standardize on DirectXMath

Three custom wrapper types currently exist around `XMFLOAT4X4` and `XMFLOAT3`:

| Type | Namespace | Role | Current adoption |
|---|---|---|---|
| `Transform3D` | `Neuron` | Rendering matrix — operators + member methods | Rendering pipeline (`ShapeFragmentData`, `MatrixStack`, `Shape::Render`) |
| `GameMatrix` | `Neuron::Math` | Gameplay matrix — pure storage, no operators | `GameSimEvent` fields (~38 producer sites) |
| `GameVector3` | `Neuron::Math` | Gameplay vector — pure storage, no operators | `GameSimEvent` fields, `GameMatrix` row accessors |

All three wrap the same DirectXMath storage types (`XMFLOAT4X4`, `XMFLOAT3`) and
every method on `Transform3D` is a 1–8 line delegation to DirectXMath intrinsics.
Maintaining three wrappers creates confusion about which type to use and adds
unnecessary conversion boilerplate.

**Decision: Eliminate all three custom types. Use raw DirectXMath types everywhere.**

### XMVECTOR / XMFLOAT3 Boundary Rule

All vector and matrix math must use SIMD register types (`XMVECTOR`, `XMMATRIX`) for
computation. `XMFLOAT3` / `XMFLOAT4X4` are **storage only** — never perform
arithmetic on them. The load→compute→store boundary must be explicit.

| Context | Type | Reason |
|---|---|---|
| Struct/class members | `XMFLOAT3` / `XMFLOAT4X4` | Stable layout, serializable, no alignment requirement |
| Function parameters (non-virtual) | `FXMVECTOR` + `XM_CALLCONV` | SIMD register passing |
| Function parameters (virtual) | `const XMFLOAT3&` | Virtual dispatch cannot use `XM_CALLCONV` |
| Function return values | `XMVECTOR` or `XMFLOAT3` | `XMVECTOR` if caller will continue computing; `XMFLOAT3` if storing |
| Local temporaries in functions | `XMVECTOR` / `XMMATRIX` | **Always** — never `XMFLOAT3` for intermediate results |
| Loop bodies | `XMVECTOR` / `XMMATRIX` | Hoist loads before loop, store after |
| One-shot scalar queries | `XMFLOAT3` overload (e.g. `Length(XMFLOAT3)`) | Returns scalar — no vector kept in register |

> **Anti-pattern — do NOT add `XMFLOAT3` arithmetic operators** (`operator+`,
> `operator*`, etc.). They would hide a load→op→store per expression, defeating
> the entire purpose of the SIMD boundary. All vector arithmetic stays in `XMVECTOR`.

This table is the contract that all Phase B–C migration follows. Every migrator
should reference it when choosing types.

**Rationale:**
1. **Zero wrapper overhead.** No custom types to learn, convert between, or maintain.
   `XMFLOAT3` already has public `float x, y, z` members — the same interface as
   `GameVector3` and `LegacyVector3`. `XMFLOAT4X4` already provides 4×4 row-major
   storage. No wrapper adds value beyond what DirectXMath provides natively.
2. **XMMATRIX overloads already exist.** `Shape::Render(XMMATRIX)` and
   `MatrixStack::Multiply(XMMATRIX)` are already in place from Phase 3.
   `Transform3D` was a stepping stone to get there — the rendering pipeline can use
   `XMMATRIX` directly.
3. **`Matrix34::ToXMFLOAT4X4()` bridges legacy directly to DirectXMath.** No
   intermediate type is needed. Code migrating from `Matrix34` targets
   `XMFLOAT4X4`/`XMMATRIX` in one step rather than passing through `Transform3D`.
4. **Free functions are the established pattern.** `Neuron::Math` already provides
   `RotateAroundX/Y/Z`, `Cross`, `Dotf`, `Length`, `Normalize`, etc. as free
   functions on `XMVECTOR`/`XMMATRIX`. `Transform3D`'s member methods
   (`RotateAround`, `Translate`, `Orthonormalize`, `TransformPoint`, etc.) are
   migrated to `Neuron::Math` free functions operating on `XMFLOAT4X4&` or
   `FXMMATRIX`. This consolidates all math in one place.
5. **Adoption is shallow enough to pivot.** `Transform3D` has ~185 references across
   ~30 files. `GameMatrix`/`GameVector3` have ~38 `GameSimEvent` producer sites.
   Both are manageable migrations, and most sites involve simple type substitution
   (`Transform3D` → `XMFLOAT4X4`, `GameVector3` → `XMFLOAT3`).

**Migration steps (integrated into Phases A–D below):**
1. Add `Neuron::Math` free-function equivalents of all `Transform3D` member methods:
   `RotateAround(XMFLOAT4X4&, FXMVECTOR)`, `Translate(XMFLOAT4X4&, FXMVECTOR)`,
   `Orthonormalize(XMFLOAT4X4&)`, `IsIdentity(const XMFLOAT4X4&)`, plus
   `OrientFU`/`DecomposeToYDR`/etc. returning `XMMATRIX`/`XMFLOAT3`.
2. Migrate `ShapeFragmentData::m_transform` from `Transform3D` to `XMFLOAT4X4`.
   Update prediction code to use free functions.
3. Remove `Shape::Render(Transform3D)` overload — callers pass `XMMATRIX` directly.
4. Remove `MatrixStack::Multiply(Transform3D)` overload — callers load to `XMMATRIX`.
5. Migrate `GameSimEvent` fields from `GameMatrix`/`GameVector3` to
   `XMFLOAT4X4`/`XMFLOAT3`.
6. Delete `Transform3D.h`, `GameMatrix.h`, `GameVector3.h` alongside the legacy
   headers in Phase D.

### MatrixStack — Keep for Now

The `MatrixStack` (push/multiply/pop pattern inherited from OpenGL's
`glPushMatrix`/`glPopMatrix`) was evaluated for removal to simplify the
architecture.

**Current usage:** 21 push/pop pairs across 8 files. Mostly shallow (1–2 levels).
The deepest user is `ShapeFragmentData::Render` which walks the fragment tree
recursively (typically 2–4 levels). An identity-check optimization skips stack
operations for rest-pose fragments.

**Alternative — explicit transform passing:**
```
void ShapeFragmentData::Render(FXMMATRIX _parentWorld, ...) {
    XMMATRIX world = XMMatrixMultiply(predicted, _parentWorld);
    UploadWorldMatrix(world);
    RenderGeometry();
    for (child : children) child->Render(world, ...);
}
```

This eliminates global mutable state and is more testable, but it requires:
1. `RenderSlow()` (the immediate-mode GL shim) to accept the world matrix
   explicitly — but `RenderSlow` is being **replaced entirely** by retained-mode
   vertex buffers in Phase E.1.
2. Camera/projection setup and screen-space queries (`GetTop()`) to use a
   different mechanism.

**Decision: Keep `MatrixStack` through Phase C; replace as part of Phase E
(GL layer retirement).** Removing the stack before removing the GL shim would
require two separate rewrites of the same call sites. Deferring the stack
replacement to Phase E means each call site is touched **once**.

**Phase A–C guideline:** New code should prefer passing `XMMATRIX` explicitly
wherever possible. The `Shape::Render(XMMATRIX)` entry point pushes to the stack
internally — callers should not manipulate the stack directly. Minimise new
`mv.Push()`/`mv.Pop()` pairs outside the shape rendering pipeline.

**Phase E plan:** When retained-mode rendering replaces `RenderSlow`,
convert `ShapeFragmentData::Render` to the explicit `FXMMATRIX` transform-passing
pattern above. The `MatrixStack` then becomes a thin wrapper used only for legacy
UI and camera queries, and can be removed once those are migrated.

---

## Completed Work

### Phase 0 — Thread Safety & Naming ✅ DONE

Fixed the immediate thread-safety hazard and renamed the conversion function
without changing any data layout or calling convention.

| File | Change |
|---|---|
| `matrix33.h` | Removed `static float m_openGLFormat[16]`, replaced `ConvertToOpenGLFormat` decl with `XMFLOAT4X4 ToXMFLOAT4X4(LegacyVector3 const* _pos = nullptr) const` |
| `matrix33.cpp` | Removed `float Matrix33::m_openGLFormat[16]` definition, replaced `ConvertToOpenGLFormat` impl with `ToXMFLOAT4X4` returning `XMFLOAT4X4` by value |
| `matrix34.h` | Removed `static float m_openGLFormat[16]`, replaced inline `ConvertToOpenGLFormat()` with inline `XMFLOAT4X4 ToXMFLOAT4X4() const` |
| `matrix34.cpp` | Removed `float Matrix34::m_openGLFormat[16]` definition |
| `opengl_directx.h` | Added `class Matrix34;` forward declaration, added `void glMultMatrixf(const Matrix34& m)` overload declaration |
| `opengl_directx.cpp` | Added `#include "matrix34.h"`, added `glMultMatrixf(const Matrix34&)` overload that calls `m.ToXMFLOAT4X4()` directly into `MatrixStack::Multiply` |
| `shape.cpp` | 2 call sites: replaced `.ConvertToOpenGLFormat()` with direct `Matrix34` pass |
| `tree.cpp` | 1 call site: replaced `.ConvertToOpenGLFormat()` with direct `Matrix34` pass |

### Phase 1 — Expose `MatrixStack` Directly ✅ DONE

Let rendering code manipulate the matrix stack via `XMFLOAT4X4` without
routing through OpenGL function names.

**All 94 `gl*` matrix call sites across 13 files migrated. Zero callers remain
outside the translation layer (`opengl_directx.cpp`).**

<details>
<summary>Full file list (13 files)</summary>

| File | Changes |
|---|---|
| `opengl_directx_matrix_stack.h` | Added 6 convenience methods: `Translate`, `Scale`, `RotateAxis`, `LookAtRH`, `PerspectiveFovRH`, `OrthoOffCenterRH` |
| `opengl_directx_matrix_stack.cpp` | Implemented all 6 methods using DirectXMath |
| `opengl_directx.h` | Added `#include "opengl_directx_matrix_stack.h"`, added `namespace OpenGLD3D { MatrixStack& GetModelViewStack(); MatrixStack& GetProjectionStack(); }` |
| `opengl_directx.cpp` | Implemented `GetModelViewStack()` / `GetProjectionStack()` returning file-static stacks |
| `NeuronClient/shape.cpp` | 2 sites: `glMatrixMode`/`glPushMatrix`/`glMultMatrixf`/`glPopMatrix` → `mv.Push()`/`mv.Multiply()`/`mv.Pop()` |
| `NeuronClient/sphere_renderer.cpp` | `Sphere::Render`: `glPushMatrix`/`glTranslatef`/`glScalef`/`glPopMatrix` → `mv.Push()`/`mv.Translate()`/`mv.Scale()`/`mv.Pop()` |
| `NeuronClient/text_renderer.h` | Changed `double m_projectionMatrix[16]`/`m_modelviewMatrix[16]` → `XMFLOAT4X4` |
| `NeuronClient/text_renderer.cpp` | `BeginText2D`/`EndText2D`: `glGetDoublev`/`glLoadMatrixd`/`gluOrtho2D` → direct `GetTop()`/`Load()`/`OrthoOffCenterRH()` |
| `GameLogic/tree.cpp` | `RenderAlphas`: `glPushMatrix`/`glMultMatrixf`/`glScalef`/`glPopMatrix` → `mv.Push()`/`mv.Multiply()`/`mv.Scale()`/`mv.Pop()` |
| `GameLogic/feedingtube.cpp` | `RenderSignal`: 6× `glTranslatef` → `mv.Translate()`, removed `glMatrixMode` |
| `GameLogic/radardish.cpp` | `RenderSignal`: 6× `glTranslatef` → `mv.Translate()`, removed `glMatrixMode` |
| `Starstrike/camera.cpp` | 5 push/pop pairs → `mv.Push()`/`mv.Pop()`; `SetupModelviewMatrix`: `gluLookAt` → `mv.LookAtRH()`; `Get2DScreenPos`/`GetClickRay`: `glGetDoublev` → direct stack `GetTop()` |
| `Starstrike/renderer.cpp` | `SetupProjMatrixFor3D`: `gluPerspective` → `proj.PerspectiveFovRH()`; `SetupMatricesFor2D`: `gluOrtho2D` → `proj.OrthoOffCenterRH()`; `UpdateTotalMatrix`: `glGetDoublev` → direct stack `GetTop()` |
| `Starstrike/global_internet.cpp` | `Render`: `glPushMatrix`/`glScalef`/`glPopMatrix` → `mv.Push()`/`mv.Scale()`/`mv.Pop()` |
| `Starstrike/global_world.cpp` | 2 push/pop pairs → `mv.Push()`/`mv.Scale()`/`mv.Pop()`; removed bare `glMatrixMode(GL_MODELVIEW)` |
| `Starstrike/startsequence.cpp` | Ortho setup: `glMatrixMode`/`glLoadIdentity`/`gluOrtho2D` → `mv.LoadIdentity()`/`proj.OrthoOffCenterRH()`; push/scale/pop migrated |
| `Starstrike/3d_sierpinski_gasket.cpp` | `glScalef` → `GetModelViewStack().Scale()` |
| `Starstrike/taskmanager_interface_icons.cpp` | `SetupRenderMatrices`: `glMatrixMode`/`glLoadIdentity`/`gluOrtho2D` → `mv.LoadIdentity()`/`proj.OrthoOffCenterRH()` |

</details>

### Phase 2 — Fix Operator Semantics ✅ DONE

Made `operator*` unambiguous. In DirectX row-vector convention, the
canonical multiplication is `v * M`.

- `v * M33`: changed from inverse rotation (identical to `M33 * v`) to **forward
  rotation** (linear combination of basis vectors).
- `M34 * v`: changed from forward transform (identical to `v * M34`) to **inverse
  affine transform** (world-to-local, valid for orthogonal R).
- `M33 * v`, `v * M34`, `M33 * M33`, `M34 *= M34`: unchanged.

**24 call sites audited.** Since both operators per type previously produced
identical results, swapping callers to the other form preserves exact runtime
behaviour.

<details>
<summary>Caller migration (24 sites)</summary>

| File | Sites | Change |
|------|-------|--------|
| `Starstrike/camera.cpp` | 8 | `m_up = m_up * mat` → `m_up = mat * m_up` (8 `v * M33` → `M33 * v`) |
| `Starstrike/explosion.cpp` | 1 | `totalTransform * _frag->m_centre` → `_frag->m_centre * totalTransform` (`M34 * v` → `v * M34`) |
| `NeuronClient/shape.cpp` | 5 | `totalMatrix * m_centre` → `m_centre * totalMatrix` (5 × `M34 * v` → `v * M34`) |
| `Starstrike/explosion.cpp` | 4 | `m_rotMat * vertex` — unchanged (`M33 * v`, inverse rotation) |
| `Starstrike/explosion.cpp` | 3 | `pos * totalTransform` — unchanged (`v * M34`, forward transform) |
| `NeuronClient/shape.cpp` | 2 | `m_positions[i] * totalMatrix` — unchanged (`v * M34`, forward transform) |
| `Starstrike/renderer.cpp` | 1 | `_frag->m_centre * total` — unchanged (`v * M34`, forward transform) |

</details>

### Phase 3 — Introduce Transform3D ✅ DONE

Created a thin, header-only wrapper over `XMFLOAT4X4` in `NeuronCore/Transform3D.h`
inside `namespace Neuron`. Adopted in `Shape::Render` and `ShapeFragment`.

- **3.1** — `Transform3D` created with `Identity()`, `FromAxes()`, `Right/Up/Forward/Position()`, `TransformPoint`, `operator*`, `AsXMFLOAT4X4/AsXMMATRIX`.
- **3.2** — `Matrix34` gained `operator Neuron::Transform3D() const` (implicit conversion).
- **3.3** — `Shape::Render(XMMATRIX)` overload added. `Shape::Render(const Matrix34&)` now delegates via `Matrix34 → Transform3D → XMMATRIX` implicit conversion chain. All 4 `.ToXMFLOAT4X4()` call sites now use implicit conversion.
- **3.4** — No explicit migration needed; `Matrix34` implicit conversion bridges existing callers.

### Phase 3.1 — GetWorldMatrix → GetWorldTransform ✅ DONE

Added SIMD-accelerated `GetWorldTransform(const Transform3D&)` overload.
Legacy `GetWorldMatrix(const Matrix34&)` now delegates to it.
New `XMFLOAT3` accessors (`RightF3`, `UpF3`, `ForwardF3`, `PositionF3`) added to
`Transform3D`. An `explicit Matrix34(const Transform3D&)` constructor added for
reverse conversion.

### Phase 3.2 — ShapeFragment::m_transform → Transform3D ✅ DONE

Changed `ShapeFragment::m_transform` from `Matrix34` to `Neuron::Transform3D`.
Added `Orthonormalize()`, `RotateAround(FXMVECTOR)`, `Translate(FXMVECTOR)`,
`IsIdentity()` to `Transform3D`. SIMD prediction path in `ShapeFragment::Render`.

### Transitional Types ✅ DONE

- **`GameVector3.h`** created in NeuronCore — pure storage type wrapping `XMFLOAT3`.
  `Load()` → `XMVECTOR`, `GetData()` → `float*`, `x/y/z` field access. No arithmetic
  operators. **Transitional — to be deleted in Phase D.**
- **`GameMatrix.h`** created in NeuronCore — pure storage type wrapping `XMFLOAT4X4`.
  `Load()` → `XMMATRIX`, row accessors, `Identity()`. No math methods.
  **Transitional — to be deleted in Phase D.**
- Implicit conversion operators added: `LegacyVector3` ↔ `GameVector3`,
  `Matrix34` ↔ `GameMatrix`, `Matrix33` ↔ `GameMatrix`.

---

## Remaining Migration Phases

### Phase A — Free Functions & Convenience Overloads (Low–Medium Risk)

**Goal:** Add all `Neuron::Math` free functions needed to replace legacy member
methods and `Transform3D` member methods. This is the critical unblock for Phases B–C.

**Status: NEXT** (Phase 1.3 of MathPlan and Phase 4.3–4.5 of MatrixConv are the same work.)

#### A.1 — `XMFLOAT3` convenience overloads in `GameMath.h`

Add trivial load→delegate→store wrappers for existing `XMVECTOR` functions:
```cpp
[[nodiscard]] inline float Length(XMFLOAT3 const& _v) noexcept { return Length(XMLoadFloat3(&_v)); }
[[nodiscard]] inline float LengthSquare(XMFLOAT3 const& _v) noexcept { return LengthSquare(XMLoadFloat3(&_v)); }
[[nodiscard]] inline float Dotf(XMFLOAT3 const& _a, XMFLOAT3 const& _b) noexcept { ... }
[[nodiscard]] inline XMFLOAT3 Cross(XMFLOAT3 const& _a, XMFLOAT3 const& _b) noexcept { ... }
[[nodiscard]] inline XMFLOAT3 Normalize(XMFLOAT3 const& _v) noexcept { ... }
[[nodiscard]] inline XMFLOAT3 SetLength(XMFLOAT3 const& _v, float _len) noexcept { ... }
```

During the transition period, `GameVector3` implicitly converts to/from `XMFLOAT3`,
so these overloads serve both types.

#### A.2 — Orient helpers (replaces Matrix33/34 `Orient*` members)

```cpp
// Neuron::Math free functions (GameMath.h)
[[nodiscard]] XMMATRIX XM_CALLCONV OrientFU(FXMVECTOR _front, FXMVECTOR _up) noexcept;
// ... OrientRU, OrientRF, OrientUF, OrientUR, OrientFR similarly
```

Callers that currently write:
```cpp
Matrix34 mat(m_front, g_upVector, m_pos);
m_shape->Render(predictionTime, mat);
```
migrate directly to raw DirectXMath:
```cpp
XMMATRIX mat = OrientFU(XMLoadFloat3(&m_front), Vector3::UP);
mat.r[3] = XMVectorSetW(XMLoadFloat3(&m_pos), 1.f);
m_shape->Render(predictionTime, mat);   // uses existing XMMATRIX overload
```

Callers span ~15 GameLogic files (building construction, entity orientation, weapons).

#### A.3 — Extract `Transform3D` member methods to free functions

```cpp
void XM_CALLCONV RotateAround(XMFLOAT4X4& _m, FXMVECTOR _axis) noexcept;
void XM_CALLCONV RotateAround(XMFLOAT4X4& _m, FXMVECTOR _localAxis, float _angle) noexcept;
void XM_CALLCONV Translate(XMFLOAT4X4& _m, FXMVECTOR _offset) noexcept;
void Orthonormalize(XMFLOAT4X4& _m) noexcept;
[[nodiscard]] bool IsIdentity(const XMFLOAT4X4& _m) noexcept;
```

These are direct extractions of the 1–8 line implementations already in `Transform3D.h`.
The `Transform3D` member methods can then delegate to these free functions during the
transition, or be removed outright once callers migrate.

**Caveat — RotateAroundY sign convention:** `Matrix34::RotateAroundY(angle)` uses
the opposite sign convention from `Matrix34::RotateAround(Y_axis, angle)` — they
rotate in opposite directions for the same positive angle. When migrating callers
that use named-axis variants, the sign must be preserved.

Before migrating each call site, determine which convention is intended:

| Legacy call | DirectXMath equivalent | Sign |
|---|---|---|
| `mat.RotateAroundY(angle)` | `XMMatrixRotationY(-angle)` | Negate |
| `mat.RotateAround(Y_axis, angle)` | `XMMatrixRotationAxis(Y_axis, angle)` | Same |
| `mat.RotateAroundX(angle)` | Audit per-site | Audit |
| `mat.RotateAroundZ(angle)` | Audit per-site | Audit |

Call sites using named-axis variants (`RotateAroundY`, etc.) are found in:
`triffid.cpp`, `mine.cpp`, `radardish.cpp`, `officer.cpp`, `engineer.cpp`,
`spam.cpp`, `sporegenerator.cpp`, `armour.cpp`. Each site must be individually
audited and visually verified after migration.

#### A.4 — Domain-specific free functions

```cpp
[[nodiscard]] XMFLOAT3 XM_CALLCONV DecomposeToYDR(FXMMATRIX _m) noexcept;
[[nodiscard]] XMMATRIX XM_CALLCONV GetOrientation(FXMMATRIX _m) noexcept;  // copy with translation zeroed
[[nodiscard]] XMMATRIX XM_CALLCONV NormaliseMatrix(FXMMATRIX _m) noexcept;
[[nodiscard]] XMVECTOR XM_CALLCONV HorizontalAndNormalise(FXMVECTOR _v) noexcept;
[[nodiscard]] XMFLOAT3 TransformPoint(const XMFLOAT4X4& _m, const XMFLOAT3& _v) noexcept;
[[nodiscard]] XMFLOAT3 TransformVector(const XMFLOAT4X4& _m, const XMFLOAT3& _v) noexcept;
[[nodiscard]] XMFLOAT3 InverseTransformPoint(const XMFLOAT4X4& _m, const XMFLOAT3& _v) noexcept;
void ToColumnMajor(const XMFLOAT4X4& _m, float (&_out)[16]) noexcept;
```

`DecomposeToYDR` is called in `camera.cpp`, `researchitem.cpp`, `darwinian.cpp`,
`spam.cpp`, and `mine.cpp`. `Transpose` uses `XMMatrixTranspose` directly (no wrapper).

> **Design principle**: Every math operation has exactly one implementation — the
> `XMVECTOR`/`XMMATRIX` version. The `XMFLOAT3`/`XMFLOAT4X4` convenience overloads
> are trivial load→delegate→store wrappers. If you need to add a new math operation,
> add it once as an `XMVECTOR`/`XMMATRIX` overload, then optionally add an
> `XMFLOAT3`/`XMFLOAT4X4` convenience wrapper.

#### Phase A Verification

- Full rebuild of all projects (Debug x64, Release x64).
- No runtime changes — purely additive API.
- Unit tests:
  - `XMFLOAT3` convenience overloads: `Length`, `Dotf`, `Cross`, `Normalize`, `SetLength` match known values.
  - `TransformPoint`/`TransformVector` match `Matrix34 * LegacyVector3` for known inputs.
  - `ToColumnMajor` matches legacy `ConvertToOpenGLFormat()` output for a known matrix.
  - `OrientFU` matches legacy `Matrix34::OrientFU` for known inputs.
  - `RotateAround`, `Translate`, `Orthonormalize`, `IsIdentity` match `Transform3D` member methods.

---

### Phase B — Core Header Migration (High Risk)

**Goal:** Change the canonical member types in `WorldObject`, `Entity`, `Building`,
and `Camera` from `LegacyVector3` / `Matrix34` to `XMFLOAT3` / `XMFLOAT4X4`.

> **Strategy**: Thanks to the implicit conversions (Phase 2 / transitional types),
> changing a member from `LegacyVector3` to `XMFLOAT3` is source-compatible for
> **field access**: any code that writes `obj.m_pos.x` still works (`XMFLOAT3` has
> public `x, y, z`). **However**, because `XMFLOAT3` has no arithmetic operators,
> any code that performs math directly on members (`m_pos + m_vel * dt`,
> `m_pos * otherVec` for dot) will fail to compile. These sites must be fixed in the
> same commit. Compiler errors force the correct migration pattern
> (load → compute → store).

#### B.0 — Pre-migration audit

Search for `m_pos +`, `m_pos -`, `m_pos *`, `m_vel +`, `m_vel -`, `m_vel *`,
`m_front +`, `m_front -`, `m_front *`, `m_front ^`, `m_up ^` and all `LegacyVector3`
arithmetic on members (cross via `^`, dot via `*`). Also search for virtual
function signatures using `LegacyVector3` in `Entity` subclasses. Produce a list
of files and line counts per header. If < 50 sites per header change, proceed with
compiler-error-driven migration. If > 100, batch the header change with the `.cpp`
fixups.

> **Note — virtual function parameters:** Virtual methods cannot use `XM_CALLCONV`
> because the calling convention is fixed by the vtable. Virtual function signatures
> must use `const XMFLOAT3&` (not `FXMVECTOR`) for vector parameters. Non-virtual
> free functions and final overrides in leaf classes may use `FXMVECTOR` +
> `XM_CALLCONV`.

Phase B should be executed **one header per branch/commit**, with CI validation
between each, in the following order:

1. **B.1** — `WorldObject` (highest blast radius, most downstream fixups)
2. **B.2** — `Entity` (virtual signature cascade — must follow WorldObject since Entity inherits from it)
3. **B.3** — `Building` (depends on WorldObject being done)
4. **B.4** — `Camera` (independent, can be parallel with B.3)

#### B.1 — Migrate `WorldObject`

**File**: `GameLogic/worldobject.h`

- Change `LegacyVector3 m_pos;` → `XMFLOAT3 m_pos;`.
- Change `LegacyVector3 m_vel;` → `XMFLOAT3 m_vel;`.
- Fix all `.cpp` files that perform arithmetic directly on `m_pos`/`m_vel`:
  ```cpp
  // Before (LegacyVector3 operators):
  m_pos += m_vel * dt;
  // After (XMVECTOR via Neuron::Math):
  XMVECTOR pos = XMLoadFloat3(&m_pos);
  pos = XMVectorMultiplyAdd(XMLoadFloat3(&m_vel), XMVectorReplicate(dt), pos);
  XMStoreFloat3(&m_pos, pos);
  ```

#### B.2 — Migrate `Entity`

**File**: `GameLogic/entity.h`

Entity has **4 `LegacyVector3` members** and **7 virtual function signatures** using
`LegacyVector3`. This is the highest-risk header because of the virtual signature
cascade — every override in every subclass must change atomically.

**Members:**
- Change `LegacyVector3 m_spawnPoint;` → `XMFLOAT3 m_spawnPoint;`.
- Change `LegacyVector3 m_front;` → `XMFLOAT3 m_front;`.
- Change `LegacyVector3 m_angVel;` → `XMFLOAT3 m_angVel;`.
- Change `LegacyVector3 m_centerPos;` → `XMFLOAT3 m_centerPos;`.

**Virtual signatures** (all params → `const XMFLOAT3&`, returns → `XMFLOAT3`):
- `virtual void Attack(const XMFLOAT3& pos);`
- `virtual void SetWaypoint(const XMFLOAT3& _waypoint);`
- `virtual XMFLOAT3 PushFromObstructions(const XMFLOAT3& pos, bool killem = true);`
- `virtual XMFLOAT3 PushFromCliffs(const XMFLOAT3& pos, const XMFLOAT3& oldPos);`
- `virtual XMFLOAT3 PushFromEachOther(const XMFLOAT3& _pos);`
- `bool RayHit(const XMFLOAT3& _rayStart, const XMFLOAT3& _rayDir);`
- `virtual XMFLOAT3 GetCameraFocusPoint();`

**Override cascade** — the following subclasses override these virtuals and must be
updated in the same commit:
- `Darwinian` — overrides `PushFromObstructions`
- `Engineer` — overrides `SetWaypoint`
- `Armour` — overrides `SetWayPoint` (note: different casing — `SetWayPoint` vs `SetWaypoint`)
- Any other subclass overrides found during audit.

**Strategy:** Flag-day approach — change all signatures + all overrides in one commit.
This is safer than a bridged approach because `XMFLOAT3` has the same field layout as
`LegacyVector3` (`float x, y, z`), and the implicit conversion chain
`XMFLOAT3 → GameVector3 → LegacyVector3` can bridge callers that haven't migrated yet
(direction: callers pass `XMFLOAT3` to a function expecting `const XMFLOAT3&` — no
conversion needed). Inside each override body, load to `XMVECTOR` for computation.

#### B.3 — Migrate `Building`

**File**: `GameLogic/building.h`

- Change `LegacyVector3 m_front;` → `XMFLOAT3 m_front;`.
- Change `LegacyVector3 m_up;` → `XMFLOAT3 m_up;`.
- Change `LegacyVector3 m_centerPos;` → `XMFLOAT3 m_centerPos;`.

#### B.4 — Migrate `Camera`

**File**: `Starstrike/camera.h`

- Change `LegacyVector3` members/params to `XMFLOAT3`.
- `GetRight()` currently computes `m_up ^ m_front` — replace with `Cross(m_up, m_front)`.

#### Phase B Verification

- Full rebuild of all projects (Debug x64, Release x64).
- All game systems still function: camera movement, building placement, entity
  movement, control tower reprogramming, rendering.
- Spot-check that GL shim interop (`glVertex3fv`-style calls via `&pos.x`) compiles
  and produces correct visuals.

---

### Phase C — File-by-File Migration (Low Per-File Risk, Large Scope)

**Goal:** Convert individual `.cpp` files from legacy types to raw DirectXMath. Also
migrate `Transform3D` usage, `GameMatrix`/`GameVector3` usage, `Matrix33` usage, and
`ShapeMarker::m_transform`.

This is the long tail — can be done incrementally by any team member.

#### Migration Pattern (per file)

1. Replace `#include "LegacyVector3.h"` / `#include "matrix34.h"` / `#include "matrix33.h"` (if not already via pch).
2. Replace local variable types: `LegacyVector3 foo` → `XMVECTOR` for temporaries used in computation. Only use `XMFLOAT3` for storage that must persist (struct members, serialization, arrays).
3. Replace `Matrix34 mat(...)` → `XMMATRIX mat = OrientFU(...)` with position row set.
4. Replace `vec ^ vec` → `XMVector3Cross(a, b)` (when already `XMVECTOR`), or `Cross(a, b)` for one-shot `XMFLOAT3` inputs.
5. Replace `vec * vec` (dot) → `Dotf(a, b)` (scalar result, `XMFLOAT3` overload OK).
6. Replace `vec.Mag()` → `Length(vec)` (scalar result, `XMFLOAT3` overload OK), `vec.Normalise()` → `XMVector3Normalize(v)` (when already `XMVECTOR`), or `Normalize(vec)` for `XMFLOAT3`.
7. Replace `mat.ConvertToOpenGLFormat()` → `float gl[16]; ToColumnMajor(mat, gl);`.
8. Replace `mat.OrientFU(f, u)` → `OrientFU(XMLoadFloat3(&f), XMLoadFloat3(&u))`.
9. Replace `mat * vec` / `vec * mat` → `XMVector3TransformCoord(v, m)` (when already `XMVECTOR`/`XMMATRIX`), or `TransformPoint(mat, vec)` for `XMFLOAT3`/`XMFLOAT4X4` one-shot.
10. Replace `g_upVector` → `Vector3::UP` (XMVECTOR) or `XMFLOAT3(0.f, 1.f, 0.f)` (storage).
11. Replace `g_zeroVector` → `Vector3::ZERO` or `XMFLOAT3(0.f, 0.f, 0.f)`.
12. Replace `g_identityMatrix34` → identity `XMFLOAT4X4` or `XMMatrixIdentity()`.
13. **For math-heavy functions, hoist loads to function scope** — `XMLoadFloat3` at entry, compute entirely in `XMVECTOR`/`XMMATRIX`, `XMStoreFloat3` once at exit.

#### Canonical Migration Patterns

Reference these templates when migrating arithmetic from `LegacyVector3` to
`XMVECTOR`. All intermediate computation stays in SIMD registers.

**Pattern 1 — Simple Advance:**
```cpp
// Before:
m_pos += m_vel * dt;
m_vel.y -= GRAVITY * dt;
// After:
XMVECTOR pos = XMLoadFloat3(&m_pos);
XMVECTOR vel = XMLoadFloat3(&m_vel);
pos = XMVectorMultiplyAdd(vel, XMVectorReplicate(dt), pos);
vel = XMVectorSubtract(vel, XMVectorSet(0.f, GRAVITY * dt, 0.f, 0.f));
XMStoreFloat3(&m_pos, pos);
XMStoreFloat3(&m_vel, vel);
```

**Pattern 2 — Distance check (one-shot scalar):**
```cpp
// Before:
float dist = (target->m_pos - m_pos).Mag();
// After (XMFLOAT3 overload is fine — returns scalar, no vector kept):
float dist = Length(XMVectorSubtract(XMLoadFloat3(&target->m_pos), XMLoadFloat3(&m_pos)));
```

**Pattern 3 — Normalize direction:**
```cpp
// Before:
m_front = (m_wayPoint - m_pos).Normalise();
// After:
XMStoreFloat3(&m_front, XMVector3Normalize(
    XMVectorSubtract(XMLoadFloat3(&m_wayPoint), XMLoadFloat3(&m_pos))));
```

**Pattern 4 — Multi-operation function (hoist loads, compute in-register):**
```cpp
// Before:
LegacyVector3 toTarget = target->m_pos - m_pos;
float dist = toTarget.Mag();
LegacyVector3 dir = toTarget / dist;
m_pos += dir * speed * dt;
// After:
XMVECTOR myPos    = XMLoadFloat3(&m_pos);
XMVECTOR toTarget = XMVectorSubtract(XMLoadFloat3(&target->m_pos), myPos);
float dist        = Length(toTarget);
XMVECTOR dir      = XMVectorScale(toTarget, 1.f / dist);
myPos = XMVectorMultiplyAdd(dir, XMVectorReplicate(speed * dt), myPos);
XMStoreFloat3(&m_pos, myPos);
```

**Pattern 5 — Cross / Dot (in-register):**
```cpp
// Before:
LegacyVector3 right = m_front ^ g_upVector;
float alignment = m_front * targetDir;
// After:
XMVECTOR front = XMLoadFloat3(&m_front);
XMVECTOR right = XMVector3Cross(front, Vector3::UP);
float alignment = Dotf(front, targetDir);  // targetDir already XMVECTOR
```

**Pattern 6 — Matrix construction + render:**
```cpp
// Before:
Matrix34 mat(m_front, g_upVector, m_pos);
m_shape->Render(predictionTime, mat);
// After:
XMMATRIX mat = OrientFU(XMLoadFloat3(&m_front), Vector3::UP);
mat.r[3] = XMVectorSetW(XMLoadFloat3(&m_pos), 1.f);
m_shape->Render(predictionTime, mat);
```

> **Key rule:** Local variables used in computation are `XMVECTOR` / `XMMATRIX`,
> never `XMFLOAT3`. If a value is computed and immediately stored to a member, the
> store happens via `XMStoreFloat3`. If a value is computed and used in further
> computation, it stays as `XMVECTOR` until it needs to be stored.

#### C.1 — `Matrix33` rotation-only sites (4 files)

Use `XMFLOAT4X4` with translation zeroed (no separate 3×3 type). The `GetOr()`
helper becomes `GetOrientation(FXMMATRIX)` free function.

| File | Key changes |
|---|---|
| `camera.cpp` | 2 `Matrix33` sites → `XMFLOAT4X4` |
| `explosion.h/cpp` | `Matrix33` rotation member → `XMFLOAT4X4` |
| `matrix34.h` | `GetOr()` → `GetOrientation()` free function |

#### C.2 — `ShapeMarker::m_transform` → `XMFLOAT4X4`

Same pattern as Phase 3.2. Change member type, update parse/write, remove legacy
`GetWorldMatrix` overload.

#### C.3 — `Transform3D` → `XMFLOAT4X4` in rendering pipeline

- `ShapeFragmentData::m_transform` → `XMFLOAT4X4`. Prediction uses free functions.
- Remove `Shape::Render(const Matrix34&)` overload — callers use `XMMATRIX` directly.
- Remove any remaining `Transform3D`-accepting overloads — callers load to `XMMATRIX`.

#### C.4 — `GameSimEvent` fields → `XMFLOAT4X4`/`XMFLOAT3`

Migrate `GameMatrix`/`GameVector3` fields to raw DirectXMath. ~38 producer sites.

After C.4 completes, verify that `DrainSimEvents()` in `Starstrike/main.cpp` no
longer relies on implicit `GameVector3→LegacyVector3` / `GameMatrix→Matrix34`
conversions (previously tracked as SimEvent.md Phase 2 Step 8 — resolved
automatically once downstream APIs accept the new types).

#### C.5 — GameLogic Matrix34 construction sites (~60 files)

The bulk migration. Each file constructs `Matrix34(front, up, pos)` and passes to
`Shape::Render` or `GetWorldMatrix`. Migrate to `XMMATRIX` via `OrientFU()` +
position row.

| File | Sites | Pattern |
|---|---|---|
| `triffid.cpp` | 15 | Construct, RotateAround, render (heaviest single-file user) |
| `constructionyard.cpp` | 16 | GetWorldMatrix for markers, construct for render |
| `controltower.cpp` | 14 | GetWorldMatrix for ports/lights, construct for render |
| `building.cpp` | 13 | Construct from `(m_front, m_up, m_pos)`, render + GetWorldMatrix |
| `mine.cpp` | 13 | GetWorldMatrix + RotateAround + render |
| `rocket.cpp` | 13 | GetWorldMatrix + construct for render |
| `radardish.cpp` | 11 | GetWorldMatrix, RotateAround (partially migrated) |
| `gunturret.cpp` | 10 | GetWorldMatrix for barrels/muzzles |
| `bridge.cpp` | 9 | GetWorldMatrix × 6 |
| `armour.cpp` | 9 | Construct, RotateAround, render |
| `souldestroyer.cpp` | 9 | Construct, render |
| `spiritreceiver.cpp` | 9 | GetWorldMatrix |
| `generator.cpp` | 9 | GetWorldMatrix |
| `blueprintstore.cpp` | 8 | GetWorldMatrix, construct for rendering |
| `entity_leg.cpp` | 8 | GetWorldMatrix, construct from markers |
| `insertion_squad.cpp` | 8 | GetWorldMatrix |
| `laserfence.cpp` | 8 | GetWorldMatrix |
| `incubator.cpp` | 7 | GetWorldMatrix |
| `feedingtube.cpp` | 7 | GetWorldMatrix |
| `staticshape.cpp` | 7 | Construct, render |
| `sporegenerator.cpp` | 6 | Construct, RotateAround, render |
| `researchitem.cpp` | 6 | GetWorldMatrix + DecomposeToYDR |
| `math_utils.cpp` | 6 | Matrix utility functions |
| `spawnpoint.cpp` | 5 | GetWorldMatrix |
| `weapons.cpp` | 5 | Construct + GetWorldMatrix |
| `officer.cpp` | 5 | GetWorldMatrix + RotateAround |
| `explosion.cpp` | 5 | Construct from fragment transforms (2 sites already migrated) |
| `spider.cpp` | 5 | Construct, render |
| `spam.cpp` | 5 | Construct + RotateAround + DecomposeToYDR |
| `switch.cpp` | 4 | GetWorldMatrix |
| `trunkport.cpp` | 4 | GetWorldMatrix |
| `entity.cpp` | 4 | Construct, render |
| `armyant.cpp` | 4 | Construct, render |
| `centipede.cpp` | 4 | Construct, render |
| `engineer.cpp` | 4 | Construct, RotateAround, render |
| `global_world.cpp` | 4 | Construct, render |
| `cave.cpp` | 3 | GetWorldMatrix |
| `renderer.cpp` | 3 | Fragment transform (1 site already migrated) |
| `tripod.cpp` | 3 | Construct + RotateAround |
| Other files (≤2 sites) | ~15 | `airstrike`, `anthill`, `darwinian`, `factory`, `lander`, `lasertrooper`, `library`, `location_input`, `safearea`, `scripttrigger`, `teleport`, `wall`, `3d_sierpinski_gasket`, `ai` |

**Total:** ~60 files, ~350+ `Matrix34` references remaining.

#### C.6 — `LegacyVector3` in NeuronClient & Starstrike

| File pattern | Key changes |
|---|---|
| `NeuronClient/3d_sprite.cpp` | Render code with `LegacyVector3`. |
| `NeuronClient/bitmap.cpp` | Minimal math. |
| `NeuronClient/math_utils.h/.cpp` | Change all function signatures to `const XMFLOAT3&`/`const XMFLOAT4X4&`. |
| `Starstrike/camera.cpp` | Core camera math; many `LegacyVector3` ops. |
| `Starstrike/explosion.cpp` | Particle positions. |
| Other ~20 files | Low per file. |

#### C.7 — GameRender `*Renderer.cpp` files (20+ files)

| File pattern | Key changes |
|---|---|
| `GameRender/*Renderer.cpp` | Replace `LegacyVector3`/`Matrix34` locals and params, `ConvertToOpenGLFormat` → `ToColumnMajor`, `g_upVector` → `Vector3::UP`. |
| `GameRender/BuildingRenderer.cpp` | Base renderer class, heavier math usage. |
| `GameRender/EntityRenderer.cpp` | Base renderer class. |

#### Phase C Verification

- Incremental builds after each file.
- Full build after completing each subsystem batch.
- Run game and spot-check affected systems.

---

### Phase D — Delete All Legacy & Transitional Types (Medium–High Risk)

**Goal:** Delete old headers, transitional wrapper types, and implementation files
once all consumers have migrated to raw DirectXMath types.

#### D.1 — Verify no remaining legacy operator usage

Final codebase search for `operator*` between two vector types (dot product) and
`operator^` (cross product). These operators exist only on `LegacyVector3` — any
remaining usage will be a compile error once the headers are deleted.

#### D.2 — Delete files and add temporary aliases

**Step 1 — Delete source files:**

**Legacy files** (NeuronClient):
- `NeuronClient/LegacyVector3.h`
- `NeuronClient/vector3.cpp`
- `NeuronClient/matrix33.h`
- `NeuronClient/matrix33.cpp`
- `NeuronClient/matrix34.h`
- `NeuronClient/matrix34.cpp`

**Transitional files** (NeuronCore):
- `NeuronCore/Transform3D.h`
- `NeuronCore/GameMatrix.h`
- `NeuronCore/GameVector3.h`

Remove from `.vcxproj` files. Remove `#include` directives from `GameMath.h`.

**Step 2 — Add temporary aliases** (default — always add these on deletion):
```cpp
// In GameMath.h — temporary aliases to catch stragglers. Remove once grep confirms zero usage.
namespace Neuron::Math { using GameVector3 = DirectX::XMFLOAT3; }
namespace Neuron::Math { using GameMatrix = DirectX::XMFLOAT4X4; }
namespace Neuron { using Transform3D = DirectX::XMFLOAT4X4; }
```

**Step 3 — Full rebuild.** The aliases catch any remaining references.

**Step 4 — Grep for alias usage.** Fix any remaining references to use raw
DirectXMath types.

**Step 5 — Remove aliases.** Final cleanup.

#### D.3 — Remove global constants

- `g_upVector` → `Neuron::Math::Vector3::UP` or `XMFLOAT3(0.f, 1.f, 0.f)`.
- `g_zeroVector` → `Neuron::Math::Vector3::ZERO` or `XMFLOAT3(0.f, 0.f, 0.f)`.
- `g_identityMatrix34` → `XMMatrixIdentity()` or identity `XMFLOAT4X4`.

#### D.4 — Fix `Vector2` ↔ `LegacyVector3` coupling

Replace `LegacyVector3` forward declarations and conversion constructors in
`vector2.h` with `XMFLOAT3` equivalents.

#### D.5 — Prerequisite check (current state)

| Type | Consumer files | References |
|---|---|---|
| `Matrix34` | ~60 | ~350 |
| `Matrix33` | 4 | `camera.cpp`, `explosion.h/cpp`, `matrix34.h` |
| `Transform3D` | ~30 | ~185 (rendering pipeline + ShapeFragment) |
| `GameMatrix` | ~38 | `GameSimEvent` producer sites + `GameSimEvent.h` fields |
| `GameVector3` | — | `GameSimEvent.h` fields + `GameMatrix.h` row accessors |

#### Phase D Verification

- Full rebuild of all projects in all configurations.
- Full game playthrough of at least one level to verify no regressions.
- Verify no remaining references to deleted files.

---

### Phase E — Retire the OpenGL Translation Layer & MatrixStack (High Risk)

**Goal:** Remove all `gl*` function calls from game and rendering code. The DX12
backend becomes the only code path. Replace the global `MatrixStack` with explicit
transform passing.

> **Scope note:** Phase E is a major rendering rewrite that is substantially larger
> than Phases A–D combined. It should be planned and tracked as a **separate project**
> with its own detailed breakdown, milestones, and success criteria. The subsections
> below are high-level goals only — not implementation-ready plans.

#### E.1 — Replace immediate-mode rendering

`ShapeFragment::RenderSlow` uses `glBegin/glEnd/glVertex3fv/glNormal3fv/glColor4ub`.
These are translated into vertex buffer submissions by `opengl_directx.cpp`. Replace
with pre-built vertex buffers uploaded once at shape load time, drawn via
`DrawIndexedInstanced`.

This is the single largest rendering change in the codebase and should be its own
project.

#### E.2 — Replace matrix stack with explicit transform passing

Once retained-mode rendering replaces `RenderSlow`, convert the fragment hierarchy
from implicit stack accumulation to explicit transform passing:

```cpp
void ShapeFragmentData::Render(FXMMATRIX _parentWorld, ...) {
    XMMATRIX world = XMMatrixMultiply(predicted, _parentWorld);
    UploadWorldMatrix(world);
    DrawRetainedGeometry();
    for (child : children) child->Render(world, ...);
}
```

This eliminates the 21 push/pop pairs and removes the global mutable state dependency.
Camera and projection setup migrate to a `RenderContext` struct holding view and
projection matrices (`XMFLOAT4X4`) explicitly.

#### E.3 — Remove `opengl_directx.h/cpp` entirely

After all OpenGL emulation functions are replaced (matrix, state, immediate-mode
drawing), delete the translation layer (~2000 lines of compatibility code).

---

### Phase F — Optimization Pass (Ongoing, Reduced Scope)

With the pure-storage-type approach, Phase C migration naturally pushes math into
`XMVECTOR`/`XMMATRIX` registers. Phase F focuses on **batch-level** optimizations.

#### F.1 — Profile hot loops

Profile the game to identify math-heavy hot spots (entity `Advance()` loops,
particle systems, collision detection in `math_utils`).

#### F.2 — Batch SIMD in hot loops

In identified hot loops, hoist `Load()` calls out of inner loops and keep data in
`XMVECTOR`/`XMMATRIX` across multiple operations. Since Phase C already uses
`XMVECTOR` for computation, the main gain is reducing load/store traffic.

#### F.3 — Consider SoA layout for entity arrays

For very hot paths (e.g., Darwinian AI with thousands of entities), consider
Structure-of-Arrays layout (`x[]`, `y[]`, `z[]` separate arrays). Only if justified
by profiling data.

---

## Dependency Graph

```
Phase 0 ─── Thread safety + rename                                       ✅
   │
Phase 1 ─── Expose MatrixStack directly                                   ✅
   │
Phase 2 ─── Fix operator semantics ─── (ran in parallel with Phase 1)     ✅
   │
Phase 3 ─── Introduce Transform3D + GetWorldTransform + ShapeFragment     ✅
   │         + Transitional types (GameVector3, GameMatrix)               ✅
   │         + Implicit conversions (LegacyVector3↔GameVector3, etc.)     ✅
   │
Phase A ── Free functions + convenience overloads ───────────────────┐   NEXT
   │       (Orient*, RotateAround, Translate, Orthonormalize,       │
   │        DecomposeToYDR, XMFLOAT3 overloads, Transform3D        │
   │        method extractions)                                     │
   │                                                                 │
 Phase B ── Core headers (sequential, one per commit):               │
   │       B.1 WorldObject → XMFLOAT3 (highest blast radius)        │
   │       B.2 Entity → XMFLOAT3 + virtual signature cascade        │
   │       B.3 Building → XMFLOAT3                                  │
   │       B.4 Camera → XMFLOAT3 (parallel with B.3)               │
   │                                                                 │
Phase C ── File-by-file migration (load→compute→store rewrite) ────┘
   │       C.1 Matrix33 (4 files)                           ← parallel
   │       C.2 ShapeMarker::m_transform                     ← parallel
   │       C.3 Transform3D → XMFLOAT4X4 in rendering       ← parallel
   │       C.4 GameSimEvent fields                          ← parallel
   │       C.5 GameLogic Matrix34 sites (~60 files)         ← parallel
   │       C.6 NeuronClient/Starstrike LegacyVector3       ← parallel
   │       C.7 GameRender *Renderer.cpp (20+ files)         ← parallel
   │
Phase D ── Delete all legacy + transitional types
   │
Phase E.1 ─ Replace immediate-mode rendering (RenderSlow → retained-mode)
   │
Phase E.2 ─ Replace MatrixStack with explicit transform passing
   │
Phase E.3 ─ Remove opengl_directx.h/cpp entirely
   │
Phase F ── Optimization pass (profile-driven)
```

---

## Risk Assessment

| Phase | Risk | Status | Rollback | Build Impact | Visual Impact |
|---|---|---|---|---|---|
| 0 | Low | ✅ Done | Trivial | None | None |
| 1 | Medium | ✅ Done | Moderate | Additional `#include` | None |
| 2 | Medium | ✅ Done | Moderate | None | None (verified) |
| 3 | Low–Medium | ✅ Done | Easy | New headers | None |
| 3.1 | Low | ✅ Done | Easy | None | None |
| 3.2 | Medium | ✅ Done | Moderate | 3 external files | None if correct |
| A | Medium | **Next** | Easy (additive) | `GameMath.h` additions | None (additive API) |
| B.1 | **High** | Pending | Moderate | Full rebuild from WorldObject.h | Potential if inline math breaks |
| B.2 | **High** | Pending | Moderate | Full rebuild + virtual cascade | Potential if overrides missed |
| B.3 | High | Pending | Moderate | Rebuild from Building.h | Potential if inline math breaks |
| B.4 | Medium | Pending | Moderate | Rebuild Starstrike only | Potential |
| C | Medium | Pending | Low per file | Low per file | Potential per file |
| D | High | Pending | Difficult | Full rebuild | None if all migrated |
| E.1 | High | Not started | Difficult | Major rewrite | High |
| E.2 | Medium | Not started | Moderate | Stack removal | None if transforms correct |
| E.3 | Low | Not started | Easy | File deletion | None |
| F | Low | Not started | Low per change | Profile-driven | None |

### Key Risks

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| `operator*` (dot) / `operator^` (cross) compile errors in Phase B | Blocks build | High | Pre-migration audit (B.0). These operators live only on `LegacyVector3`. Compiler errors force migration to `Dotf()`/`Cross()` or `XMVector3Dot()`/`XMVector3Cross()`. Fix in same commit. |
| No arithmetic operators on `XMFLOAT3` | More verbose code (load→compute→store) | Certain | This is intentional — see XMVECTOR boundary rule. Use canonical migration patterns. Hoist loads to function scope. Do **not** add `XMFLOAT3` arithmetic operators. |
| Entity virtual signature cascade (Phase B.2) | All subclass overrides must change atomically | High | Audit all overrides before touching `Entity.h`. Flag-day commit. CI validation. Virtual params use `const XMFLOAT3&` (not `FXMVECTOR`). |
| `RotateAroundY` sign convention mismatch | Incorrect rotation direction | Medium | Sign audit table in Phase A.3. Verify per-site with visual regression. |
| Massive rebuild from header changes | Lost developer time | High | Phase B triggers full rebuild per header. One header per branch/commit with CI between each. |
| `Matrix34` row-major layout vs DirectXMath | Incorrect transforms | Medium | Both row-major. Unit test: `TransformPoint(XMFLOAT4X4, point)` matches `Matrix34(front, up, pos) * point`. |
| Both `mat * vec` and `vec * mat` compute same result in legacy | Incorrect if only one migrated | Medium | Both are identical (affine transform). Replace all with `XMVector3TransformCoord` (in-register) or `TransformPoint` (storage). |

---

## Testing Strategy

Each phase should be validated by:

1. **Build all configurations** (Debug x64, Release x64).
2. **Visual regression** — capture reference screenshots of representative scenes
   before the change. Compare after.
3. **Matrix value validation** — add a temporary `ASSERT` in `uploadAndBindConstants`
   that compares the old path's `WorldMatrix` with the new path's output. Values must
   be bit-identical.
4. **Operator audit** (Phase 2, completed) — for every `Matrix33/34 * vec` call site,
   confirm whether forward or inverse transform was intended.
5. **Unit tests** (VS Native Unit Test Framework):
   - `XMFLOAT3` convenience overloads match known values.
   - `TransformPoint`/`TransformVector` match legacy `Matrix34 * LegacyVector3`.
   - `ToColumnMajor` matches legacy `ConvertToOpenGLFormat()`.
   - `OrientFU` matches legacy `Matrix34::OrientFU`.
   - `RotateAround`, `Translate`, `Orthonormalize`, `IsIdentity` match `Transform3D` members.
   - Round-trip conversions: `LegacyVector3 → GameVector3 → LegacyVector3`, `Matrix34 → GameMatrix → Matrix34`.

---

## Status Summary

| Phase | Status | Blocking? | Notes |
|---|---|---|---|
| 0 — Thread safety | ✅ Done | — | `ConvertToOpenGLFormat` static buffer eliminated. |
| 1 — Expose MatrixStack | ✅ Done | — | All 94 `gl*` matrix callers migrated. Zero remain. |
| 2 — Fix operators | ✅ Done | — | `M*v` = inverse, `v*M` = forward. 24 sites audited. |
| 3 — Transform3D + transitional types | ✅ Done | — | Type created; adopted in Shape, MatrixStack, ShapeFragment. GameVector3/GameMatrix created (transitional). Implicit conversions added. |
| 3.1 — GetWorldTransform | ✅ Done | — | SIMD hierarchy composition. Legacy delegates to it. |
| 3.2 — ShapeFragment | ✅ Done | — | `m_transform` → `Transform3D`. SIMD prediction. |
| **A — Free functions** | **Next** | **Yes** — blocks B, C | Orient helpers, Transform3D extractions, XMFLOAT3 overloads, domain functions. |
| B.1 — WorldObject | Pending | Yes — blocked by A | `m_pos`, `m_vel` → `XMFLOAT3`. Highest blast radius. |
| B.2 — Entity | Pending | Yes — blocked by B.1 | 4 members + 7 virtual signatures → `XMFLOAT3`. Override cascade in Darwinian, Engineer, Armour. |
| B.3 — Building | Pending | Yes — blocked by B.1 | `m_front`, `m_up`, `m_centerPos` → `XMFLOAT3`. |
| B.4 — Camera | Pending | No (parallel with B.3) | `LegacyVector3` members/params → `XMFLOAT3`. |
| C — File-by-file | Pending | Blocked by A | ~80+ files across all projects. Matrix33, ShapeMarker, Transform3D→XMFLOAT4X4, GameSimEvent, Matrix34 sites, LegacyVector3, GameRender. |
| D — Delete everything | Pending | Blocked by C | Legacy + transitional headers deleted. Zero custom types. |
| E.1 — Retained-mode | Not started | No | Replaces `RenderSlow` / immediate-mode GL. |
| E.2 — Explicit transforms | Not started | Yes — blocked by E.1 | Replaces `MatrixStack` in fragment hierarchy. |
| E.3 — Delete GL layer | Not started | Yes — blocked by E.2 | Removes `opengl_directx.h/cpp` (~2000 LOC). |
| F — Optimization | Not started | No | Profile-driven batch SIMD. |

### Key Simplifications

1. **Zero custom types**: All wrapper types (`Transform3D`, `GameMatrix`, `GameVector3`)
   eliminated alongside legacy types (`Matrix34`, `Matrix33`, `LegacyVector3`). End
   state: pure DirectXMath.
2. **`Matrix33` → `XMFLOAT4X4` (not `XMFLOAT3X3`)**: No new type. 4 remaining sites
   use `XMFLOAT4X4` with translation zeroed.
3. **MatrixStack kept through Phase C**: Tightly coupled to GL shim. Deferred to E.2.
4. **Phase A is the critical unblock**: Free functions enable the bulk migration.
5. **`Matrix34::ToXMFLOAT4X4()` is the direct legacy bridge**: No intermediate needed.

### Remaining Phase A Items (Blocking B–C)

- [ ] `XMFLOAT3` convenience overloads: `Length`, `LengthSquare`, `Dotf`, `Cross`, `Normalize`, `SetLength`
- [ ] Orient free functions: `OrientFU/RU/UF/UR/FR/RF`
- [ ] Transform3D extractions: `RotateAround(XMFLOAT4X4&, FXMVECTOR)`, `Translate(XMFLOAT4X4&, FXMVECTOR)`, `Orthonormalize(XMFLOAT4X4&)`, `IsIdentity(const XMFLOAT4X4&)`
- [ ] Domain functions: `TransformPoint`, `TransformVector`, `InverseTransformPoint`, `DecomposeToYDR`, `NormaliseMatrix`, `HorizontalAndNormalise`, `ToColumnMajor`, `GetOrientation`
- [ ] Unit tests for all of the above

---

## Success Criteria

- [ ] All math operations consolidated as `Neuron::Math` free functions in `GameMath.h`.
- [ ] All `LegacyVector3`, `Matrix33`, `Matrix34` members in core types migrated to `XMFLOAT3`/`XMFLOAT4X4`.
- [ ] All `.cpp` files use `XMFLOAT3`/`XMFLOAT4X4` (storage) and `XMVECTOR`/`XMMATRIX` (computation).
- [ ] Legacy headers (`LegacyVector3.h`, `matrix33.h`, `matrix34.h`) deleted.
- [ ] Transitional headers (`Transform3D.h`, `GameMatrix.h`, `GameVector3.h`) deleted.
- [ ] Global constants (`g_upVector`, `g_zeroVector`, `g_identityMatrix34`) replaced.
- [ ] All build configurations compile and link cleanly with zero new warnings.
- [ ] Game runs identically — no visual, gameplay, or behavioral regressions.
- [ ] Unit tests pass for all new free functions.
- [ ] `math_utils.h` functions migrated to `XMFLOAT3`/`XMFLOAT4X4` types.

---

## Estimated Scope

| Phase | Files Changed | Effort | Risk |
|---|---|---|---|
| A | 1–2 modified (`GameMath.h`, `GameMath.cpp` for non-inline impls) | Small (1–2 days) | Low |
| B | 4 headers + associated `.cpp` fixups (WorldObject, Entity, Building, Camera) | Medium–High (3–5 days) | High — Entity virtual cascade |
| C | ~80–100 `.cpp` files (function-by-function rewrite to load→compute→store) | Large (2–4 weeks) | Low per file |
| D | ~9 deleted/modified + temporary aliases | Small (1 day) | Medium |
| E | Major rendering rewrite (separate project) | Large | High |
| F | Varies (profile-driven) | Ongoing | Low |

> **Phase C scope note:** Without `XMFLOAT3` arithmetic operators (see XMVECTOR
> boundary rule), Phase C is a function-by-function rewrite, not a type rename.
> Each function’s math flow must be restructured to load→compute→store. The
> canonical migration patterns above reduce cognitive load, but the per-file effort
> is higher than a simple search-and-replace. Plan for parallel work across team
> members on C.5/C.6/C.7.

---

## Appendix A — Files Affected by Matrix34

| File | Project | Usage Pattern | Migrated? |
|---|---|---|---|
| `building.cpp` | GameLogic | Construct from (front, up, pos), pass to Shape::Render | No |
| `blueprintstore.cpp` | GameLogic | GetWorldMatrix for markers, construct for rendering | No |
| `controltower.cpp` | GameLogic | GetWorldMatrix for ports/lights, construct for render | No |
| `constructionyard.cpp` | GameLogic | GetWorldMatrix for markers, construct for render | No |
| `armour.cpp` | GameLogic | Construct, RotateAround, render | No |
| `armyant.cpp` | GameLogic | Construct, render | No |
| `centipede.cpp` | GameLogic | Construct, render | No |
| `engineer.cpp` | GameLogic | Construct, render | No |
| `souldestroyer.cpp` | GameLogic | Construct, render | No |
| `sporegenerator.cpp` | GameLogic | Construct, render | No |
| `entity.cpp` | GameLogic | Construct, render, virtual impl (PushFromObstructions etc.) | No |
| `entity_leg.cpp` | GameLogic | GetWorldMatrix, construct from markers | No |
| `darwinian.cpp` | GameLogic | PushFromObstructions override, construct, orientation | No |
| `factory.cpp` | GameLogic | GetWorldMatrix | No |
| `cave.cpp` | GameLogic | GetWorldMatrix | No |
| `tree.cpp` | GameLogic | Construct, mv.Multiply(mat) | No |
| `bridge.cpp` | GameLogic | Construct, GetWorldMatrix (6 instances) | No |
| `airstrike.cpp` | GameLogic | Construct from predicted pos | No |
| `anthill.cpp` | GameLogic | Construct, render | No |
| `shape.cpp` | NeuronClient | `m_transform` migrated to Transform3D; hit-test sigs still Matrix34 | **Partial** |
| `math_utils.cpp` | NeuronClient | Matrix utility functions | No |
| `camera.cpp` | Starstrike | Matrix33 for rotation, Matrix34 for camera transform | No |
| `location_input.cpp` | Starstrike | Transform construction | No |
| `3d_sierpinski_gasket.cpp` | Starstrike | Fractal rendering transform | No |
| `location.cpp` | Starstrike | Location rendering | No |
| `explosion.h/cpp` | Starstrike | Matrix33 member for rotation; fragment transform (2 sites migrated) | **Partial** |
| `radardish.cpp` | GameLogic | RotateAround migrated to SIMD; construction sites remain | **Partial** |
| `renderer.cpp` | Starstrike | Fragment transform (1 site migrated); Matrix34 params remain | **Partial** |

## Appendix B — `gl*` Matrix Function Callers (Historical — All Migrated in Phase 1)

| Function | File | Count |
|---|---|---|
| `glPushMatrix` / `glPopMatrix` | camera.cpp | 5 pairs |
| | global_world.cpp | 2 pairs |
| | global_internet.cpp | 1 pair |
| | startsequence.cpp | 1 pair |
| | tree.cpp | 1 pair |
| | shape.cpp | 2 pairs |
| | sphere_renderer.cpp | 1 pair |
| `glMultMatrixf` | shape.cpp | 2 calls |
| | tree.cpp | 1 call |

---

## Data & Content Dependencies

None. This is a code-only migration; no data files, assets, or scripts are affected.
Serialization formats (`Building::Read`/`Building::Write` in text files) use `float`
values and `atoi`/`printf`, which operate on scalar fields — these remain unchanged
since `XMFLOAT3` exposes `x`, `y`, `z` (same as `LegacyVector3` and `GameVector3`).

---

## Date

Created: 2026-03-23 (combined from MatrixConv.md and MathPlan.md)
