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

### Operator Conventions (Current)

| Operation | Formula | Convention |
|---|---|---|
| `Matrix33 * vec` | `dot(row_i, v)` for each row | M·v (column-vector, inverse rotation) |
| `vec * Matrix33` | Same formula as above | **Identical** — should differ but doesn't |
| `Matrix34 * vec` | `v.x*r + v.y*u + v.z*f + pos` | v·M (row-vector, forward transform) |
| `vec * Matrix34` | Same formula as above | **Identical** — should differ but doesn't |
| `Matrix33 * Matrix33` | Standard row-major product | Row-major ✓ |
| `Matrix34 *= Matrix34` | Standard row-major product + position | Row-major ✓ |
| `MatrixStack::Multiply` | `XMMatrixMultiply(mat, current)` | Pre-multiply (= DX local-transform-first) ✓ |
| Shader | `mul(v, WorldMatrix)` | Row-vector ✓ |

### Rendering Pipeline (Current)

```
Building/Entity
  │  stores (m_front, m_up, m_pos) as LegacyVector3
  ▼
Building::Render()
  │  Matrix34 mat(m_front, g_upVector, m_pos);
  │  m_shape->Render(predictionTime, mat);
  ▼
Shape::Render(predictionTime, Matrix34& transform)
  │  glPushMatrix();
  │  glMultMatrixf(transform.ConvertToOpenGLFormat());     ← float[16] via static buffer
  │  m_rootFragment->Render(predictionTime);
  │  glPopMatrix();
  ▼
ShapeFragment::Render(predictionTime)                       (recursive)
  │  glPushMatrix();
  │  glMultMatrixf(localTransform.ConvertToOpenGLFormat()); ← float[16] via static buffer
  │  RenderSlow();                                          ← immediate-mode glBegin/glEnd
  │  recurse children
  │  glPopMatrix();
  ▼
opengl_directx.cpp — glMultMatrixf(const float* m)
  │  memcpy(&mat, m, sizeof(XMFLOAT4X4));                  ← no transpose
  │  s_pTargetMatrixStack->Multiply(mat);                   ← pre-multiply
  ▼
uploadAndBindConstants()
  │  XMStoreFloat4x4(&cb.WorldMatrix, stack.GetTopXM());   ← direct copy
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

| # | Issue | Impact |
|---|---|---|
| P1 | `ConvertToOpenGLFormat` relies on `static float m_openGLFormat[16]` — **not thread-safe**, returns pointer to shared mutable state | Data corruption if two matrices convert in overlapping scopes |
| P2 | OpenGL-named functions (`glMultMatrixf`, `glPushMatrix`, etc.) obscure that the backend is DX12; every matrix operation pays an indirection through the translation layer | Readability, performance, maintenance burden |
| P3 | `operator*(Matrix33, vec)` and `operator*(vec, Matrix33)` return **identical results** — the convention is ambiguous | Confusing; bugs if someone relies on mathematical ordering |
| P4 | `Matrix34 * vec` implements forward transform but reads as column-vector notation `M·v` — misleading | Confusing; hides the actual row-vector convention |
| P5 | No SIMD — Matrix33/34 use scalar float arithmetic | Missed performance opportunity for hierarchy composition |
| P6 | Legacy types force every render site to construct `Matrix34` from raw vectors and call `ConvertToOpenGLFormat` | Boilerplate, error-prone |
| P7 | `Matrix33` and `Matrix34` live in NeuronClient but are `#include`d by GameLogic (14+ files) — cross-project dependency on a rendering-era type | Architectural layering violation |

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

#### 4.1 — Migrate `ShapeMarker::GetWorldMatrix`

`GetWorldMatrix` composes a chain of `Matrix34` transforms via `operator*=`. Replace
with `Transform3D` composition using `XMMatrixMultiply`.

#### 4.2 — Migrate `ShapeFragment::m_transform` and animation

`ShapeFragment` stores `Matrix34 m_transform`, `LegacyVector3 m_angVel`, and
`LegacyVector3 m_vel`. The `Render` method predicts the transform:

```cpp
Matrix34 predicted = m_transform;
predicted.RotateAround(m_angVel * dt);
predicted.pos += m_vel * dt;
```

Replace with XMMATRIX rotation (using `XMMatrixRotationAxis` from `GameMath.h`) and
XMVECTOR translation.

#### 4.3 — Migrate orientation functions

`OrientRU`, `OrientRF`, `OrientUF`, `OrientUR`, `OrientFR`, `OrientFU` exist in both
`Matrix33` and `Matrix34`. Provide equivalents as free functions or `Transform3D`
factory methods that compute the orthonormalised basis via `XMVector3Cross` and
`XMVector3Normalize`.

#### 4.4 — Migrate `RotateAround*` family

`RotateAroundR/U/F/X/Y/Z(angle)` and `RotateAround(axis, angle)` are used for entity
animation and physics. Replace with `XMMatrixRotationAxis` and `XMMatrixMultiply`.

#### 4.5 — Migrate `Transpose` and `DecomposeToYDR`

- `Transpose`: `XMMatrixTranspose`.
- `DecomposeToYDR` (Euler extraction): Rewrite using `atan2f` on `XMFLOAT4X4`
  elements directly. The math is the same; only the accessors change.

#### 4.6 — Migrate `Matrix33` rotation-only usage

`Matrix33` is used in `Camera`, `Explosion`, `LaserFence`, and `Location`. These
typically need a 3×3 rotation only. Options:

- Use `XMFLOAT3X3` + helper functions (lightweight).
- Use `Transform3D` with translation = zero (uniform, slightly wasteful).

**Recommended:** `XMFLOAT3X3` for pure rotation, `Transform3D` for full affine.

#### 4.7 — Delete `matrix33.h/cpp` and `matrix34.h/cpp`

Once all 22+ consumer files have been migrated and verified (build + visual
regression test), delete the legacy files from `NeuronClient` and remove them from
`NeuronClient.vcxproj`.

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
Phase 0 ─── Thread safety + rename
   │
Phase 1 ─── Expose MatrixStack directly
   │
Phase 2 ─── Fix operator semantics ─── (can run in parallel with Phase 1)
   │
Phase 3 ─── Introduce Transform3D (depends on Phase 0 + 1)
   │
Phase 4 ─── Remove Matrix33/Matrix34 (depends on Phase 3 completing for each file)
   │
Phase 5 ─── Retire OpenGL layer (depends on Phase 1 + 4 + separate vertex buffer migration)
```

---

## Risk Assessment

| Phase | Risk | Rollback Difficulty | Build Impact | Visual Impact |
|---|---|---|---|---|
| 0 | Low | Trivial | None (API-compatible overloads) | None |
| 1 | Medium | Moderate (revert to gl* calls) | Additional `#include` | None if done correctly |
| 2 | Medium | Moderate | None (operator signatures unchanged) | **Bugs if caller direction was assumed** |
| 3 | Low–Medium | Easy (new additive type) | New header, new .cpp | None (runs alongside legacy) |
| 4 | High | Difficult (API removal) | Removes files from projects | Potential visual regressions |
| 5 | High | Difficult | Major rendering rewrite | High — immediate-mode to retained-mode |

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

| File | Project | Usage Pattern |
|---|---|---|
| `building.cpp` | GameLogic | Construct from (front, up, pos), pass to Shape::Render |
| `blueprintstore.cpp` | GameLogic | GetWorldMatrix for markers, construct for rendering |
| `controltower.cpp` | GameLogic | GetWorldMatrix for ports/lights, construct for render |
| `constructionyard.cpp` | GameLogic | GetWorldMatrix for markers, construct for render |
| `armour.cpp` | GameLogic | Construct, RotateAround, render |
| `armyant.cpp` | GameLogic | Construct, render |
| `centipede.cpp` | GameLogic | Construct, render |
| `engineer.cpp` | GameLogic | Construct, render |
| `souldestroyer.cpp` | GameLogic | Construct, render |
| `sporegenerator.cpp` | GameLogic | Construct, render |
| `entity_leg.cpp` | GameLogic | GetWorldMatrix, construct from markers |
| `factory.cpp` | GameLogic | GetWorldMatrix |
| `cave.cpp` | GameLogic | GetWorldMatrix |
| `tree.cpp` | GameLogic | Construct, glMultMatrixf(mat.ConvertToOpenGLFormat()) |
| `bridge.cpp` | GameLogic | Construct, GetWorldMatrix (6 instances) |
| `airstrike.cpp` | GameLogic | Construct from predicted pos |
| `anthill.cpp` | GameLogic | Construct, render |
| `shape.cpp` | NeuronClient | Render entry point, fragment hierarchy, collision |
| `math_utils.cpp` | NeuronClient | Matrix utility functions |
| `camera.cpp` | Starstrike | Matrix33 for rotation, Matrix34 for camera transform |
| `location_input.cpp` | Starstrike | Transform construction |
| `3d_sierpinski_gasket.cpp` | Starstrike | Fractal rendering transform |
| `location.cpp` | Starstrike | Location rendering |
| `explosion.h` | Starstrike | Matrix33 member for rotation |

## Appendix B — `gl*` Matrix Function Callers

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
| `operator*(M33, v)` ≡ `operator*(v, M33)` (identical formulas) | **Correct** | Both compute `[dot(r,v), dot(u,v), dot(f,v)]` — verified in `matrix33.h` L54–65. |
| `operator*(M34, v)` ≡ `operator*(v, M34)` (identical formulas) | **Correct** | Both compute `r.x*v.x + u.x*v.y + f.x*v.z + pos.x, ...` — verified in `matrix34.h` L145–157. |
| MatrixStack pre-multiplies: `Result = M * Current` | **Correct** | `XMMatrixMultiply(mat, current)` in `opengl_directx_matrix_stack.cpp` L34. Comment confirms intent. |
| No `XMMatrixTranspose` anywhere in the pipeline | **Correct** | Grep found zero occurrences outside `MatrixConv.md`. |
| 3 call sites for `ConvertToOpenGLFormat` | **Correct** | `shape.cpp:793`, `shape.cpp:1163`, `tree.cpp:295`. |
| `Matrix33 * Matrix33` is standard row-major product | **Correct** | `result.r.x = r.x*_o.r.x + r.y*_o.u.x + r.z*_o.f.x` — verified in `matrix33.cpp` L510–525. |
| `Matrix34 *= Matrix34` is standard row-major product | **Correct** | Verified in `matrix34.cpp` L425–445. Position handled as `pos * R_other + pos_other`. |
| `FromLegacy(Matrix34)` is a memcpy | **WRONG** | `Matrix34` = 48 bytes (4×3 floats), `XMFLOAT4X4` = 64 bytes (4×4 floats). Padding insertion required. Corrected in Phase 3.1. |

### Additional Findings

1. **`InverseMultiplyVector` is dead code** — declared and implemented in both
   `Matrix33` and `Matrix34` but has **zero callers** across the entire codebase.
   Can be deleted as dead code (CI.md Phase 4).

2. **Semantic inconsistency between Matrix33 and Matrix34 operators:**
   - `Matrix33 * v` computes `[dot(r,v), dot(u,v), dot(f,v)]` = **inverse rotation**
     (projects world-space v onto local basis axes).
   - `Matrix34 * v` computes `v.x*r + v.y*u + v.z*f + pos` = **forward transform**
     (composes local basis vectors from v and adds translation).
   - These are *mathematically different* operations despite the same `operator*`
     syntax. This is the root cause of the ambiguity:
     `Matrix33::operator*` does `M^T * v` while `Matrix34::operator*` does `v * M + t`.

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
   `gluLookAt` → `Load(View)` → `glMultMatrixf(World)` → `Result = World * View` →
   shader `v * Result = v * World * View` → vertex in world, then view space. ✓
