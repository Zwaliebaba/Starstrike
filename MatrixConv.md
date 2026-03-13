# Matrix Convention Standardisation Plan

## Goal

Standardise all matrix operations to **DirectX row-major, row-vector convention**,
eliminate the `ConvertToOpenGLFormat` path, and converge on `DirectXMath` types
(`XMMATRIX` / `XMFLOAT4X4`) as the canonical matrix representation.

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

### Rendering Pipeline (Post Phase 4.2)

```
Building/Entity
  │  stores (m_front, m_up, m_pos) as LegacyVector3
  ▼
Building::Render()
  │  Matrix34 mat(m_front, g_upVector, m_pos);
  │  m_shape->Render(predictionTime, mat);      ← implicit Matrix34 → Transform3D
  ▼
Shape::Render(predictionTime, Transform3D& transform)
  │  mv.Push();
  │  mv.Multiply(transform);                    ← MatrixStack::Multiply(Transform3D)
  │  m_rootFragment->Render(predictionTime);
  │  mv.Pop();
  ▼
ShapeFragment::Render(predictionTime)            (recursive)
  │  Transform3D predicted = m_transform;        ← m_transform is now Transform3D
  │  predicted.RotateAround(XMVectorScale(...)); ← SIMD via XMMatrixRotationAxis
  │  predicted.Translate(XMVectorScale(...));     ← SIMD position offset
  │  mv.Push();
  │  mv.Multiply(predicted);                     ← direct Transform3D → XMFLOAT4X4
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
| P5 | No SIMD — Matrix33/34 use scalar float arithmetic | Missed performance opportunity for hierarchy composition | ⚠️ Partially fixed — `GetWorldTransform` and `ShapeFragment::Render` use SIMD; remaining scalar sites depend on Phase 4.3+ |
| P6 | Legacy types force every render site to construct `Matrix34` from raw vectors and call `ConvertToOpenGLFormat` | Boilerplate, error-prone | ⚠️ Partially fixed — `ConvertToOpenGLFormat` eliminated; `Matrix34` construction remains in ~60 GameLogic files |
| P7 | `Matrix33` and `Matrix34` live in NeuronClient but are `#include`d by GameLogic (14+ files) — cross-project dependency on a rendering-era type | Architectural layering violation | Pending (Phase 4.8–4.9) |

---

## Migration Plan

### Phase 0 — Thread Safety & Naming (Low Risk) ✅ DONE

**Goal:** Fix the immediate thread-safety hazard and rename the conversion function
without changing any data layout or calling convention.

**Status:** Completed. All three sub-steps applied in a single pass.

**Changes made:**

| File | Change |
|---|---|
| `matrix33.h` | Removed `static float m_openGLFormat[16]`, removed `#include "stdlib.h"`, replaced `ConvertToOpenGLFormat` decl with `XMFLOAT4X4 ToXMFLOAT4X4(LegacyVector3 const* _pos = nullptr) const` |
| `matrix33.cpp` | Removed `float Matrix33::m_openGLFormat[16]` definition, replaced `ConvertToOpenGLFormat` impl with `ToXMFLOAT4X4` returning `XMFLOAT4X4` by value |
| `matrix34.h` | Removed `static float m_openGLFormat[16]`, replaced inline `ConvertToOpenGLFormat()` with inline `XMFLOAT4X4 ToXMFLOAT4X4() const` |
| `matrix34.cpp` | Removed `float Matrix34::m_openGLFormat[16]` definition |
| `opengl_directx.h` | Added `class Matrix34;` forward declaration, added `void glMultMatrixf(const Matrix34& m)` overload declaration |
| `opengl_directx.cpp` | Added `#include "matrix34.h"`, added `glMultMatrixf(const Matrix34&)` overload that calls `m.ToXMFLOAT4X4()` directly into `MatrixStack::Multiply` |
| `shape.cpp` | 2 call sites: replaced `.ConvertToOpenGLFormat()` with direct `Matrix34` pass |
| `tree.cpp` | 1 call site: replaced `.ConvertToOpenGLFormat()` with direct `Matrix34` pass |

---

### Phase 1 — Expose `MatrixStack` Directly ✅ DONE

**Goal:** Let rendering code manipulate the matrix stack via `XMFLOAT4X4` without
routing through OpenGL function names.

**Approach chosen:** Option A — accessor functions (`GetModelViewStack()`,
`GetProjectionStack()`) in `OpenGLD3D` namespace, plus convenience methods on
`MatrixStack` (`Translate`, `Scale`, `RotateAxis`, `LookAtRH`, `PerspectiveFovRH`,
`OrthoOffCenterRH`).

**All 94 `gl*` matrix call sites across 13 files migrated. Zero callers remain
outside the translation layer (`opengl_directx.cpp`).**

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

---

### Phase 2 — Fix Operator Semantics ✅ DONE

**Goal:** Make `operator*` unambiguous. In DirectX row-vector convention, the
canonical multiplication is `v * M`.

#### 2.1 — Fix `Matrix33` operators ✅

`v * M33` changed from inverse rotation (identical to `M33 * v`) to **forward
rotation** (linear combination of basis vectors):
`result = { v.x*r.x + v.y*u.x + v.z*f.x, v.x*r.y + v.y*u.y + v.z*f.y, v.x*r.z + v.y*u.z + v.z*f.z }`

`M33 * v` unchanged — inverse rotation: `{ r·v, u·v, f·v }`.

**File:** `NeuronClient/matrix33.h`

#### 2.2 — Fix `Matrix34` operators ✅

`M34 * v` changed from forward transform (identical to `v * M34`) to **inverse
affine transform** (world-to-local, valid for orthogonal R):
`d = v − pos; result = { r·d, u·d, f·d }`

`v * M34` unchanged — forward transform: `v.x*r + v.y*u + v.z*f + pos`.

**File:** `NeuronClient/matrix34.h`

#### 2.3 — Caller audit & migration ✅

Full audit found **24 call sites** across the codebase. Since both operators per
type previously produced identical results, swapping callers to the other operator
form preserves **exact** runtime behaviour with zero semantic change.

| File | Sites | Change |
|------|-------|--------|
| `Starstrike/camera.cpp` | 8 | `m_up = m_up * mat` → `m_up = mat * m_up` (8 `v * M33` → `M33 * v`) |
| `Starstrike/explosion.cpp` | 1 | `totalTransform * _frag->m_centre` → `_frag->m_centre * totalTransform` (`M34 * v` → `v * M34`) |
| `NeuronClient/shape.cpp` | 5 | `totalMatrix * m_centre` → `m_centre * totalMatrix` (5 × `M34 * v` → `v * M34`) |
| `Starstrike/explosion.cpp` | 4 | `m_rotMat * vertex` — unchanged (`M33 * v`, inverse rotation) |
| `Starstrike/explosion.cpp` | 3 | `pos * totalTransform` — unchanged (`v * M34`, forward transform) |
| `NeuronClient/shape.cpp` | 2 | `m_positions[i] * totalMatrix` — unchanged (`v * M34`, forward transform) |
| `Starstrike/renderer.cpp` | 1 | `_frag->m_centre * total` — unchanged (`v * M34`, forward transform) |

---

### Phase 3 — Introduce a Row-Major Matrix Type (Low–Medium Risk) ✅ DONE

**Goal:** Provide a modern matrix type that wraps `XMFLOAT4X4` and replaces
`Matrix33` / `Matrix34` over time.

**Status:** Completed. All four sub-steps applied.

#### 3.1 — Create `Transform3D` in NeuronCore ✅

A thin, header-only wrapper over `XMFLOAT4X4` stored in row-major DirectX convention,
placed in `NeuronCore/Transform3D.h` inside `namespace Neuron`. Included globally via
`GameMath.h` → `pch.h` chain.

API surface:
- **Construction:** `Identity()`, `FromAxes(right, up, forward, pos)`,
  `FromXMFLOAT4X4(XMFLOAT4X4)`
- **Access:** `Right()`, `Up()`, `Forward()`, `Position()` (return `XMVECTOR`)
- **Operations:** `TransformPoint(FXMVECTOR)`, `TransformNormal(FXMVECTOR)`,
  `operator*`, `operator*=` (composition)
- **Conversion:** `AsXMFLOAT4X4()`, `AsXMMATRIX()`

**Key design point:** `FromLegacy(Matrix34)` is NOT provided in `Transform3D` itself
(NeuronCore cannot depend on NeuronClient's `Matrix34`). Instead, the implicit
conversion operator on `Matrix34` (Phase 3.2) handles this at the NeuronClient layer.

| File | Change |
|---|---|
| `NeuronCore/Transform3D.h` | New file — `Neuron::Transform3D` struct |
| `NeuronCore/GameMath.h` | Added `#include "Transform3D.h"` |
| `NeuronCore/NeuronCore.vcxproj` | Added `Transform3D.h` to `ClInclude` |

#### 3.2 — Add implicit conversion from `Matrix34` to `Transform3D` ✅

| File | Change |
|---|---|
| `NeuronClient/matrix34.h` | Added `operator Neuron::Transform3D() const` using existing `ToXMFLOAT4X4()` |

#### 3.3 — Adopt `Transform3D` in rendering code ✅

Added `MatrixStack::Multiply(const Neuron::Transform3D&)` overload and
`Shape::Render(float, const Neuron::Transform3D&)` overload. All 4 call sites that
previously called `.ToXMFLOAT4X4()` now use implicit `Matrix34 → Transform3D`
conversion.

| File | Change |
|---|---|
| `NeuronClient/opengl_directx_matrix_stack.h` | Added forward decl `namespace Neuron { struct Transform3D; }`, added `Multiply(const Neuron::Transform3D&)` declaration |
| `NeuronClient/opengl_directx_matrix_stack.cpp` | Added `#include "Transform3D.h"`, implemented `Multiply(const Neuron::Transform3D&)` delegating to `XMFLOAT4X4` overload |
| `NeuronClient/shape.h` | Added `void Render(float, const Neuron::Transform3D&)` declaration |
| `NeuronClient/shape.cpp` | `ShapeFragment::Render`: `mv.Multiply(predictedTransform.ToXMFLOAT4X4())` → `mv.Multiply(predictedTransform)` (implicit conversion) |
| `NeuronClient/shape.cpp` | `Shape::Render(Matrix34)`: now delegates to `Shape::Render(Transform3D)` overload |
| `NeuronClient/shape.cpp` | New `Shape::Render(Transform3D)`: primary implementation using `mv.Multiply(_transform)` |
| `NeuronClient/opengl_directx.cpp` | `glMultMatrixf(Matrix34)`: `Multiply(m.ToXMFLOAT4X4())` → `Multiply(m)` (implicit conversion) |
| `GameLogic/tree.cpp` | `mv.Multiply(mat.ToXMFLOAT4X4())` → `mv.Multiply(mat)` (implicit conversion) |

#### 3.4 — Migrate GameLogic files incrementally

No explicit migration needed at this stage. Since `Matrix34` has an implicit
conversion to `Transform3D`, all existing callers of `Shape::Render(Matrix34)` work
unchanged. Callers can optionally be updated to construct `Transform3D` directly, but
this is deferred to Phase 4 when `Matrix34` usage is removed file-by-file.

---

### Phase 4 — Remove Legacy Types (High Risk, Long-Term)

**Goal:** Delete `Matrix33` and `Matrix34` once all consumers have migrated to
`Transform3D` or raw `XMMATRIX`.

#### 4.1 — Migrate `ShapeMarker::GetWorldMatrix` ✅

Added a SIMD-accelerated `GetWorldTransform(const Transform3D&)` overload that
composes the hierarchy using `XMLoadFloat4x4` / `XMMatrixMultiply` /
`XMStoreFloat4x4`. The legacy `GetWorldMatrix(const Matrix34&)` now delegates to it
via `Matrix34(GetWorldTransform(_rootTransform))`.

New `XMFLOAT3` accessors (`RightF3`, `UpF3`, `ForwardF3`, `PositionF3`) added to
`Transform3D` for caller bridging. An `explicit Matrix34(const Transform3D&)`
constructor added for the reverse conversion. The `explicit` qualifier prevents
implicit bidirectional conversions that would cause overload ambiguity.

| File | Change |
|---|---|
| `NeuronCore/Transform3D.h` | Added `RightF3()`, `UpF3()`, `ForwardF3()`, `PositionF3()` returning `XMFLOAT3` |
| `NeuronClient/matrix34.h` | Added `explicit Matrix34(const Neuron::Transform3D&)` constructor |
| `NeuronClient/shape.h` | Added `GetWorldTransform(const Neuron::Transform3D&)` declaration |
| `NeuronClient/shape.cpp` | New `GetWorldTransform` — SIMD hierarchy composition; `GetWorldMatrix` delegates to it |

#### 4.2 — Migrate `ShapeFragment::m_transform` and animation ✅

Changed `ShapeFragment::m_transform` from `Matrix34` to `Neuron::Transform3D`.
Added four methods to `Transform3D`:

- **`Orthonormalize()`** — SIMD re-orthogonalisation matching `Matrix34::Normalise()`
  convention (forward first, right = cross(up, forward), up = cross(forward, right)).
- **`RotateAround(FXMVECTOR _axis)`** — orientation-only rotation where the axis
  magnitude encodes the angle. Implemented as post-multiply by
  `XMMatrixRotationAxis` with position row saved and restored.
- **`Translate(FXMVECTOR _offset)`** — adds offset to position row.
- **`IsIdentity()`** — approximate identity comparison via `XMMatrixIsIdentity`.

`ShapeFragment::Render` prediction now uses the SIMD path:

```cpp
Neuron::Transform3D predicted = m_transform;
predicted.RotateAround(XMVectorScale(
  XMVectorSet(m_angVel.x, m_angVel.y, m_angVel.z, 0.0f), _predictionTime));
predicted.Translate(XMVectorScale(
  XMVectorSet(m_vel.x, m_vel.y, m_vel.z, 0.0f), _predictionTime));
```

Hit-test methods, `CalculateCentre`, `CalculateRadius` use explicit `Matrix34`
construction from the `Transform3D * Matrix34` composition result. Method signatures
still take `const Matrix34&` for callers that have not yet migrated.

`GetWorldTransform` simplified: `m_parents[i]->m_transform.m` loads directly without
`static_cast`.

| File | Change |
|---|---|
| `NeuronCore/Transform3D.h` | Added `Orthonormalize()`, `RotateAround(FXMVECTOR)`, `Translate(FXMVECTOR)`, `IsIdentity()` |
| `NeuronClient/matrix34.h` | Changed `Matrix34(const Neuron::Transform3D&)` to `explicit` |
| `NeuronClient/shape.h` | `ShapeFragment::m_transform`: `Matrix34` → `Neuron::Transform3D` |
| `NeuronClient/shape.cpp` | Constructors: `Transform3D::Identity()` init, parse into `m._XX` fields, `Orthonormalize()`; `WriteToFile`: `F3` accessors; `Render`: SIMD prediction; hit-tests: `Matrix34(m_transform * _transform)` |
| `GameLogic/radardish.cpp` | `.r` → `RightF3()` + `LegacyVector3`; `RotateAround(LV3)` → `RotateAround(XMVectorSet(...))` |
| `Starstrike/explosion.cpp` | 2 sites: `Matrix34 x = ...` → `Matrix34 x(...)` (explicit ctor) |
| `Starstrike/renderer.cpp` | 1 site: same explicit ctor pattern |

#### 4.3 — Migrate orientation functions

`OrientRU`, `OrientRF`, `OrientUF`, `OrientUR`, `OrientFR`, `OrientFU` exist in both
`Matrix33` and `Matrix34`. Provide equivalents as free functions or `Transform3D`
factory methods that compute the orthonormalised basis via `XMVector3Cross` and
`XMVector3Normalize`.

Callers span ~15 GameLogic files (building construction, entity orientation,
weapons). Most construct a `Matrix34` from two axes then pass it to `Shape::Render`.
Since `Shape::Render` now accepts `Transform3D`, these callers can construct
`Transform3D::FromAxes(...)` directly once the orient helpers exist.

#### 4.4 — Migrate `RotateAround*` family

`RotateAroundR/U/F(angle)` rotate around local basis axes. Used in camera, entity
animation, and physics across ~20 call sites in 15 files. `RotateAroundX/Y/Z(angle)`
rotate around world axes. Used mainly in camera.cpp (21 sites) and
constructionyard/darwinian/researchitem/spam.

**Note:** `Transform3D::RotateAround(FXMVECTOR)` (Phase 4.2) covers the
general-axis case. The local-basis-axis variants (`RotateAroundR/U/F`) require
extracting the basis vector from the matrix first, then calling
`RotateAround`. The world-axis variants (`RotateAroundX/Y/Z`) can use
`XMMatrixRotationX/Y/Z` directly.

**Caveat:** `Matrix34::RotateAroundY(angle)` uses the opposite sign convention from
`Matrix34::RotateAround(Y_axis, angle)` — they rotate in opposite directions for the
same angle. When migrating callers that use named-axis variants, the sign must be
preserved (negate if switching to the general `XMMatrixRotationAxis` path, or use the
matching `XMMatrixRotationY` intrinsic directly).

#### 4.5 — Migrate `Transpose` and `DecomposeToYDR`

- `Transpose`: `XMMatrixTranspose`.
- `DecomposeToYDR` (Euler extraction): Rewrite using `atan2f` on `XMFLOAT4X4`
  elements directly. The math is the same; only the accessors change.

`DecomposeToYDR` is called in `camera.cpp`, `researchitem.cpp`, `darwinian.cpp`,
`spam.cpp`, and `mine.cpp`. `Transpose` is called in `camera.cpp` only.

#### 4.6 — Migrate `Matrix33` rotation-only usage

`Matrix33` is used in `camera.cpp` (2 sites), `explosion.h/cpp` (rotation member +
usage), and `matrix34.h` (`GetOr()` helper). `LaserFence` and `Location` no longer
reference `Matrix33` directly.

Options:

- Use `XMFLOAT3X3` + helper functions (lightweight).
- Use `Transform3D` with translation = zero (uniform, slightly wasteful).

**Recommended:** `XMFLOAT3X3` for pure rotation, `Transform3D` for full affine.

#### 4.7 — Migrate `ShapeMarker::m_transform` to `Transform3D`

`ShapeMarker` still stores `Matrix34 m_transform`. Its constructors parse `front`,
`up`, `pos` from text files (same pattern as `ShapeFragment`). `GetWorldTransform`
already casts it to `Transform3D`. Migration follows the same pattern as Phase 4.2:
change the member type, update parsing to set `m._XX` fields directly, and update
`WriteToFile` to use `F3` accessors.

`GetWorldMatrix` can then be removed (or remain as a thin delegation for callers
that still need `Matrix34`).

#### 4.8 — Migrate remaining GameLogic `Matrix34` construction sites

The following files construct `Matrix34(front, up, pos)` and pass it to
`Shape::Render` or `GetWorldMatrix`. Each can be migrated to construct `Transform3D`
directly once orient helpers (Phase 4.3) are available:

| File | Sites | Pattern |
|---|---|---|
| `building.cpp` | 13 | Construct from `(m_front, m_up, m_pos)`, render + GetWorldMatrix |
| `constructionyard.cpp` | 16 | GetWorldMatrix for markers, construct for render |
| `controltower.cpp` | 14 | GetWorldMatrix for ports/lights, construct for render |
| `mine.cpp` | 13 | GetWorldMatrix + RotateAround + render |
| `rocket.cpp` | 13 | GetWorldMatrix + construct for render |
| `radardish.cpp` | 11 | GetWorldMatrix, RotateAround (already partially migrated) |
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
| `renderer.cpp` | 3 | Fragment transform composition (1 site already migrated) |
| `tripod.cpp` | 3 | Construct + RotateAround |
| `triffid.cpp` | 15 | Construct, RotateAround, render (heaviest single-file user) |
| Other files (≤2 sites each) | ~15 | `airstrike`, `anthill`, `darwinian`, `factory`, `lander`, `lasertrooper`, `library`, `location_input`, `safearea`, `scripttrigger`, `teleport`, `wall`, `3d_sierpinski_gasket`, `ai` |

**Total:** ~60 files, ~350+ `Matrix34` references remaining.

#### 4.9 — Delete `matrix33.h/cpp` and `matrix34.h/cpp`

Once all consumer files have been migrated and verified (build + visual
regression test), delete the legacy files from `NeuronClient` and remove them from
`NeuronClient.vcxproj`.

**Prerequisite check (current state):**
- `Matrix34`: ~60 consumer files, ~350 references remaining.
- `Matrix33`: 4 consumer files (`camera.cpp`, `explosion.h/cpp`, `matrix34.h`).
- `InverseMultiplyVector`: still dead code in both types (zero callers).

---

### Phase 5 — Retire the OpenGL Translation Layer (High Risk, Long-Term)

**Goal:** Remove all `gl*` function calls from game and rendering code. The DX12
backend becomes the only code path.

#### 5.1 — Replace immediate-mode rendering

`ShapeFragment::RenderSlow` uses `glBegin/glEnd/glVertex3fv/glNormal3fv/glColor4ub`.
These are translated into vertex buffer submissions by `opengl_directx.cpp`. Replace
with pre-built vertex buffers uploaded once at shape load time, drawn via
`DrawIndexedInstanced`.

This is the single largest rendering change in the codebase and should be its own
project (see CI.md Phase 5/6 for overlap).

#### 5.2 — Replace matrix stack with render context

Once all callers use `MatrixStack` / `Transform3D` directly (Phase 1), the `gl*`
matrix functions in `opengl_directx.cpp` become dead code. Remove them and their
declarations from `opengl_directx.h`.

#### 5.3 — Remove `opengl_directx.h/cpp` entirely

After all OpenGL emulation functions are replaced (matrix, state, immediate-mode
drawing), delete the translation layer. This removes ~2000 lines of compatibility
code.

---

## Dependency Graph

```
Phase 0 ─── Thread safety + rename                                       ✅
   │
Phase 1 ─── Expose MatrixStack directly                                   ✅
   │
Phase 2 ─── Fix operator semantics ─── (ran in parallel with Phase 1)     ✅
   │
Phase 3 ─── Introduce Transform3D (depends on Phase 0 + 1)               ✅
   │
Phase 4.1 ── GetWorldMatrix → GetWorldTransform                          ✅
   │
Phase 4.2 ── ShapeFragment::m_transform → Transform3D                    ✅
   │
Phase 4.3 ── Orient* factory methods on Transform3D ─────────────────┐
   │                                                                  │
Phase 4.4 ── RotateAround* family (camera, entities)                 │
   │                                                                  │
Phase 4.5 ── Transpose + DecomposeToYDR                              ├── can run in parallel
   │                                                                  │
Phase 4.6 ── Matrix33 (camera, explosion)                            │
   │                                                                  │
Phase 4.7 ── ShapeMarker::m_transform → Transform3D                 │
   │                                                                  │
Phase 4.8 ── GameLogic Matrix34 construction sites (~60 files) ──────┘
   │
Phase 4.9 ── Delete matrix33.h/cpp + matrix34.h/cpp
   │
Phase 5 ─── Retire OpenGL layer (depends on Phase 4.9 + separate vertex buffer migration)
```

---

## Risk Assessment

| Phase | Risk | Status | Rollback Difficulty | Build Impact | Visual Impact |
|---|---|---|---|---|---|
| 0 | Low | ✅ Done | Trivial | None (API-compatible overloads) | None |
| 1 | Medium | ✅ Done | Moderate (revert to gl* calls) | Additional `#include` | None |
| 2 | Medium | ✅ Done | Moderate | None (operator signatures unchanged) | None (verified) |
| 3 | Low–Medium | ✅ Done | Easy (new additive type) | New header, new .cpp | None |
| 4.1 | Low | ✅ Done | Easy (new additive overload) | None | None |
| 4.2 | Medium | ✅ Done | Moderate (revert member type) | 3 external files updated | None if correct |
| 4.3–4.5 | Medium | Pending | Moderate | Transform3D API additions | Potential if sign conventions wrong |
| 4.6 | Medium | Pending | Moderate | Matrix33 removal | Potential camera/explosion regressions |
| 4.7–4.8 | High | Pending | Difficult (~60 files) | Removes Matrix34 dependency | Potential visual regressions |
| 4.9 | High | Pending | Difficult (API removal) | Removes files from projects | None if all consumers migrated |
| 5 | High | Pending | Difficult | Major rendering rewrite | High — immediate-mode to retained-mode |

---

## Testing Strategy

Each phase should be validated by:

1. **Build all configurations** (Debug, Release) before and after.
2. **Visual regression** — capture reference screenshots of representative scenes
   (camera angles, building hierarchies, entity animations, trees, text) before the
   change. Compare after.
3. **Matrix value validation** — add a temporary `ASSERT` in `uploadAndBindConstants`
   that compares the old path's `WorldMatrix` with the new path's output. Values must
   be bit-identical (not just approximately equal) since no data transformation
   changes.
4. **Operator audit** (Phase 2) — for every `Matrix33/34 * vec` call site, add a
   comment confirming whether forward or inverse transform was intended. Run the game
   and verify entity positions, collision detection, and camera behaviour.

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
| `entity_leg.cpp` | GameLogic | GetWorldMatrix, construct from markers | No |
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
| `glLoadMatrixd` | text_renderer.cpp | 2 calls |
| `glTranslatef` | camera.cpp, global_world.cpp, startsequence.cpp | Multiple |
| `glRotatef` | camera.cpp | Multiple |
| `glScalef` | sphere_renderer.cpp | 1 call |
| `gluLookAt` | camera.cpp | 1 call |
| `gluPerspective` | renderer.cpp | 1 call |
| `gluOrtho2D` | text_renderer.cpp, renderer.cpp | 2 calls |

## Appendix C — Validation Notes

The following claims were verified against the source code:

| Claim | Verdict | Evidence |
|---|---|---|
| ConvertToOpenGLFormat output is row-major-compatible (no transpose) | **Correct** | Flat `float[16]` packs `[r, 0, u, 0, f, 0, pos, 1]`; `glMultMatrixf` does raw `memcpy` into `XMFLOAT4X4`; shader uses `row_major float4x4` + `mul(v, M)`. Self-consistent. |
| `operator*(M33, v)` ≡ `operator*(v, M33)` (identical formulas) | **Was correct, fixed in Phase 2** | Pre-fix: both computed `[dot(r,v), dot(u,v), dot(f,v)]`. Post-fix: `v * M33` now computes forward rotation (linear combination of basis rows). |
| `operator*(M34, v)` ≡ `operator*(v, M34)` (identical formulas) | **Was correct, fixed in Phase 2** | Pre-fix: both computed forward transform. Post-fix: `M34 * v` now computes inverse affine (world-to-local). |
| MatrixStack pre-multiplies: `Result = M * Current` | **Correct** | `XMMatrixMultiply(mat, current)` in `opengl_directx_matrix_stack.cpp` L34. Comment confirms intent. |
| No `XMMatrixTranspose` anywhere in the pipeline | **Correct** | Grep found zero occurrences outside `MatrixConv.md`. |
| 3 call sites for `ConvertToOpenGLFormat` | **Correct (historical)** | All 3 eliminated in Phase 0. |
| `Matrix33 * Matrix33` is standard row-major product | **Correct** | `result.r.x = r.x*_o.r.x + r.y*_o.u.x + r.z*_o.f.x` — verified in `matrix33.cpp` L510–525. |
| `Matrix34 *= Matrix34` is standard row-major product | **Correct** | Verified in `matrix34.cpp` L425–445. Position handled as `pos * R_other + pos_other`. |
| `FromLegacy(Matrix34)` is a memcpy | **WRONG** | `Matrix34` = 48 bytes (4×3 floats), `XMFLOAT4X4` = 64 bytes (4×4 floats). Padding insertion required. Corrected in Phase 3.1. |
| `Transform3D::RotateAround` matches `Matrix34::RotateAround(LV3)` | **Correct** | Both apply Rodrigues' rotation to each basis row independently. `Transform3D` uses post-multiply by `XMMatrixRotationAxis` + position restore. Verified: `Matrix34::FastRotateAround` formula `v' = v·cos + (axis×v)·sin + axis·(axis·v)·(1-cos)` is equivalent to `v * R_row_major` where R = `XMMatrixRotationAxis(axis, angle)`. |
| `RotateAroundY(θ)` ≠ `RotateAround(Y_axis, θ)` | **Correct — opposite signs** | `RotateAroundY` rotates each row by `Ry(-θ)` while `RotateAround(Y, θ)` rotates by `Ry(+θ)`. They produce opposite rotations for the same angle. Must preserve sign when migrating callers of named-axis variants. |
| `Matrix34 → Transform3D → Matrix34` round-trip is lossless | **Correct** | `ToXMFLOAT4X4` inserts `[0,0,0,1]` w-column; `Matrix34(Transform3D)` extracts xyz from each row. No precision change; w components are generated/discarded deterministically. |

### Additional Findings

1. **`InverseMultiplyVector` is dead code** — declared and implemented in both
   `Matrix33` and `Matrix34` but has **zero callers** across the entire codebase.
   Can be deleted as dead code (Phase 4.9).

2. **Semantic inconsistency between Matrix33 and Matrix34 operators** — **RESOLVED
   in Phase 2.** `M33 * v` = inverse rotation, `v * M33` = forward rotation.
   `M34 * v` = inverse affine, `v * M34` = forward transform. Both types now follow
   the standard row-vector convention.

3. **Normal transform caveat:** The vertex shader computes normals as
   `mul(normal, (float3x3)WorldMatrix)` (VertexShader.hlsl L109). This is correct
   only when the world matrix contains no non-uniform scaling. If non-uniform scale
   is ever introduced, the inverse-transpose must be used. This is orthogonal to the
   matrix convention migration but should be kept in mind.

4. **`building.h` exposes `Matrix34` in its public API** (`Matrix34 m_mat` in
   `BuildingPort`, `DoesShapeHit(Shape*, Matrix34)` parameter). Any file that
   includes `building.h` transitively depends on `matrix34.h`. This widens the
   migration surface beyond files that explicitly `#include "matrix34.h"`.

5. **Multiply-order is self-consistent:** GPU pipeline verified end-to-end:
   `LookAtRH` → `Load(View)` → `mv.Multiply(World)` → `Result = World * View` →
   shader `v * Result = v * World * View` → vertex in world, then view space. ✓

6. **`ShapeFragment::m_transform` has no external setters** — verified via
   `find_symbol` and grep. All reads/writes are within `shape.cpp` (constructors,
   parsers, render, hit-tests) plus 3 external composition sites
   (`radardish.cpp`, `explosion.cpp`, `renderer.cpp`), all of which were updated in
   Phase 4.2. `m_angVel` and `m_vel` similarly have no access outside `shape.cpp` +
   `radardish.cpp`.

7. **`explicit Matrix34(Transform3D)` prevents ambiguity** — without `explicit`,
   bidirectional implicit conversions (`Matrix34 → Transform3D` via `operator` +
   `Transform3D → Matrix34` via constructor) would create overload resolution
   ambiguity in expressions like `Matrix34 * Transform3D`. The `explicit` qualifier
   forces callers to write `Matrix34(expr)` for the reverse direction.
