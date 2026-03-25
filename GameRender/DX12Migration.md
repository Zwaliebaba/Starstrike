# DX12 Migration — Eliminating the OpenGL Emulation Layer

## 1. Current Architecture

### 1.1 The OpenGL Emulation Layer

The game renders **all** geometry through an OpenGL-to-D3D12 translation layer
in `NeuronClient/opengl_directx.*`.  This layer reimplements ~100 `gl*`
functions on top of D3D12:

| Subsystem | Files | What it does |
|---|---|---|
| **Core translation** | `opengl_directx.h` (308 lines), `opengl_directx.cpp` (1905 lines) | CPU-side GL state (`RenderState`), vertex assembly, PSO cache (`PSOKey`→`ID3D12PipelineState`), per-draw CB upload, texture management, blend/depth/alpha/fog/lighting/material, matrix ops, `PrepareDrawState`, `issueDrawCall` |
| **Inlined hot-path** | `opengl_directx_inline.h` (109 lines) | `glVertex3f`, `glColor4f`, `glNormal3f`, `glTexCoord2f` — thin wrappers to `_impl` functions |
| **Internals** | `opengl_directx_internals.h` (27 lines) | `CustomVertex` struct (pos + normal + diffuse + uv0 + uv1 = 48 bytes), `PrepareDrawState` declaration |
| **Display lists** | `opengl_directx_dlist.h/.cpp`, `opengl_directx_dlist_dev.h/.cpp` | Records `glBegin/glEnd` into GPU vertex buffer + command list.  **Only used by `sphere_renderer.cpp`** |
| **Matrix stack** | `opengl_directx_matrix_stack.h/.cpp` | `Push/Pop/Multiply/Load/GetTopXM`.  Used by ShapeStatic, ShapeInstance, and every renderer via `glPushMatrix/glPopMatrix/glMultMatrixf` |
| **VBO extension** | `ogl_extensions.h`, `ogl_extensions_directx.cpp` | `gglGenBuffersARB/gglBindBufferARB/gglBufferDataARB` → D3D12 vertex buffers.  **Only used by `landscape_renderer.cpp` and `water.cpp`** |
| **Uber shaders** | `Shaders/VertexShader.hlsl`, `Shaders/PixelShader.hlsl`, `Shaders/PerFrameConstants.hlsli` | Fixed-function emulation (8 lights, fog, alpha test, color material, 2 texture units, tex env modes) via `DrawConstants` flags |
| **D3D12 backend** | `d3d12_backend.h` | `OpenGLTranslationState`: root signature, PSO cache, SRV allocation, sampler management, per-frame listener, default white texture |

### 1.2 How a Draw Call Works Today

```
Renderer calls gl*()          CPU-side state tracking
         │                           │
     glEnd()  ──────────────────►  issueDrawCall()
                                     │
                          ┌──────────┴──────────┐
                          │                     │
                   uploadAndBindConstants()   Upload vertices
                          │                  (ring buffer memcpy)
                          │                     │
                   buildPSOKey()                │
                   GetOrCreatePSO()             │
                          │                     │
                   PrepareDrawState() ◄─────────┘
                          │
                   cmdList->SetPipelineState()
                   cmdList->IASetVertexBuffers()
                   cmdList->DrawInstanced()
```

**Per draw call cost:**
- `DrawConstants` (672+ bytes) uploaded to ring buffer → `memcpy` + `SetGraphicsRootConstantBufferView`
- Vertex data `memcpy` to ring buffer
- `PSOKey` hash + `unordered_map` lookup
- Viewport + scissor rect set
- Texture SRV + sampler descriptor table binds

### 1.3 GL Call Distribution

| Project | `glBegin` calls | Total GL call sites | Key consumers |
|---|---|---|---|
| **GameRender** | 139 | ~1950 | 48 renderer `.cpp` files, UI framework |
| **Starstrike** | 86 | ~1380 | `taskmanager_interface_icons` (46!), `renderer.cpp`, `global_world.cpp`, `landscape_renderer.cpp`, `water.cpp`, `particle_system.cpp`, `clouds.cpp`, `explosion.cpp`, `gamecursor.cpp` |
| **NeuronClient** | ~~15~~ 14 | ~260 | ~~`ShapeStatic.cpp`~~ (migrated to `ShapeMeshCache` — Phase 0), `text_renderer.cpp`, `sphere_renderer.cpp`, `3d_sprite.cpp`, `render_utils.cpp` |
| **Total** | **~239** | **~3590** | |

### 1.4 Existing Native D3D12 Code

**`TreeRenderer`** bypasses the GL layer:

- Owns a `GraphicsPSO` with dedicated shaders (`TreeVertexShader.hlsl`/`TreePixelShader.hlsl`)
- Manages per-tree GPU vertex buffers (upload once via ring buffer, reuse across frames)
- Shares the root signature and `DrawConstants`/`SceneConstants` CB layout with the GL layer
- Resolves GL texture IDs to SRV handles via `OpenGLD3D::GetTextureSRVGPUHandle()`

**`ShapeMeshCache`** (Phase 0) bypasses the GL layer for vertex data:

- Singleton cache mapping `ShapeStatic*` → `ShapeMeshGPU` (default-heap VB + per-fragment ranges)
- Uploads shape geometry once on first render; GPU buffer reused across frames
- Still uses the uber shader and `PrepareDrawState()` for CB upload and PSO selection
- `ShapeFragmentData::RenderNative()` replaces `RenderSlow()` — issues `DrawInstanced` from GPU buffer instead of `glBegin/glEnd`

**`QuadBatcher`** (Phase 1) bypasses per-quad `glBegin/glEnd`:

- Singleton CPU-side vertex accumulator; collects `CustomVertex` quads via `Emit(v0,v1,v2,v3)`
- `Flush()` uploads all accumulated vertices to ring buffer in one `memcpy`, then issues a single `DrawInstanced`
- Still uses `PrepareDrawState()` for PSO/CB — one CB upload per flush instead of one per quad
- Used by ShadowRenderer, SpiritRenderer, DefaultBuildingRenderer, ViriiRenderer, DarwinianRenderer

**Existing D3D12 infrastructure** (all solid, no need to replace):

| Component | Purpose |
|---|---|
| `Graphics::Core` | Device, swap chain, command list, frame management, descriptor heaps |
| `UploadRingBuffer` | 64 MB per-frame ring buffer for CPU→GPU transfers |
| `GraphicsPSO` / `PSO` | RAII PSO wrapper with builder pattern |
| `RootSignature` | 5-slot root signature (b0, b1, t0, t1, sampler table) |
| `DescriptorAllocator` | SRV/CBV/UAV and sampler descriptor management |
| `ResourceStateTracker` | Automatic D3D12 resource barrier insertion |
| `IRenderBackend` | Interface for GPU resource cleanup on game-object destruction |

---

## 2. Why Remove the GL Layer?

### 2.1 Performance Costs

| Problem | Impact | Scale |
|---|---|---|
| **Per-draw `DrawConstants` upload** | 672+ bytes memcpy + CBV bind per draw | Thousands of draws/frame (every entity, every building fragment, every particle quad) |
| **CPU vertex assembly** | `glVertex3f_impl` writes to CPU array, then bulk-copies to ring buffer | `ShapeStatic::RenderSlow()` is the worst — re-submits identical mesh data every frame |
| **No batching** | Each `glBegin/glEnd` pair = one draw call | 12+ quads for a single spirit, no instancing for Darwinians/Virii |
| **PSO hash + lookup per draw** | `buildPSOKey` + `unordered_map::find` | Every single draw call |
| **Uber shader branching** | `if (LightingEnabled)`, `if (FogEnabled)`, etc. in both VS and PS | GPU divergence on every pixel/vertex |
| **Redundant state sets** | Viewport, scissor, root signature re-bound per draw even when unchanged | Adds command-list overhead |

### 2.2 Architectural Costs

- Every new rendering feature must be squeezed through the GL abstraction (no compute shaders, no indirect draws, no structured buffers).
- The GL constants/types pollute the global namespace (`GL_BLEND`, `GL_TRIANGLES`, `GLfloat`, etc.).
- New developers must understand a dead API (OpenGL 1.x fixed-function) to work on the renderer.
- No path to modern techniques (GPU-driven rendering, bindless textures, mesh shaders).

---

## 3. Migration Strategy

### Principles

1. **Incremental.** The GL layer remains functional throughout.  Each phase replaces one category of GL usage.  The game must build and run correctly after every phase.
2. **Highest-impact first.** Target the subsystems that generate the most draw calls and vertex traffic.
3. **Shared infrastructure.** New native renderers share the existing root signature, CB layout, descriptor heaps, and upload ring buffer.  Do not create parallel infrastructure.
4. **TreeRenderer is the template.** Every new native renderer follows the same pattern: own `GraphicsPSO`, GPU vertex buffers, lazy upload, `DrawConstants`-compatible CB.
5. **Measure before and after.**  Use PIX or the in-game `FrameStats` to quantify draw calls, PSO switches, upload bytes, and frame time before and after each phase.

### Phase Overview

| Phase | Target | GL calls eliminated | Estimated effort | Performance impact |
|---|---|---|---|---|
| **0** ✅ | GPU-resident ShapeStatic meshes | ~23 `m_shape->Render()` call sites (each a deep `glBegin/glEnd` tree) | **Large** | **Very High** — eliminates per-frame vertex re-upload for all 3D models |
| **1** 🔧 | Batched billboard/quad renderer | ~60 `glBegin(GL_QUADS)` for billboards | Medium | **High** — collapses thousands of spirit/darwinian/effect quads into few draw calls |
| **2** | Landscape & water native VBs | ~3 `glDrawArrays` + VBO setup | Small | Medium — already using VBOs, just remove GL indirection |
| **3** | 2D/UI rendering (SpriteBatch) | ~80 `glBegin` in UI + HUD + cursors | Medium | Medium — UI is not a hot path but it's a large GL surface |
| **4** | Particle system & effects | ~20 `glBegin` in `particle_system`, `explosion`, `clouds` | Medium | Medium–High — particles can be batched/instanced |
| **5** | Remaining renderers & cleanup | ~60 remaining `glBegin` sites | Large | Low — long tail of misc renderers |
| **6** | Remove GL emulation layer | All remaining | Small | Compile-time and code-size win |

---

## 4. Phase Details

### Phase 0: GPU-Resident ShapeStatic Meshes ✅ COMPLETED

**Status:** Implemented and verified.  All 3D shape models (buildings, entities, SphereWorld) now render from GPU-resident vertex buffers.  Per-frame vertex re-upload is eliminated.

#### What Was Built

| Component | Description |
|---|---|
| **`ShapeMeshCache`** (singleton) | Maps `const ShapeStatic*` → `ShapeMeshGPU`.  On first render, DFS-walks the fragment tree, assembles `CustomVertex` data (pos + negated normal + color + zero UVs), and uploads to a default-heap vertex buffer via the ring buffer.  Follows the `TreeRenderer` pattern. |
| **`ShapeMeshGPU`** | Owns `com_ptr<ID3D12Resource>` VB, `D3D12_VERTEX_BUFFER_VIEW`, and a `vector<FragmentGPURange>` indexed by fragment ordinal.  Valid for the shape's lifetime. |
| **`ShapeFragmentData::RenderNative()`** | Replaces `RenderSlow()`.  Looks up GPU data from cache, computes predicted transform, pushes matrix stack, calls `PrepareDrawState(TRIANGLELIST)` + `IASetVertexBuffers` + `DrawInstanced` for this fragment's range, recurses children, pops matrix. |
| **`ShapeFragmentData::m_ownerShape`** | Back-pointer to the owning `ShapeStatic`, set during `Load()` via DFS.  Allows `RenderNative()` to look up the cache without threading the owner through call sites. |

#### Files Changed

| File | Project | Action |
|---|---|---|
| `NeuronClient/ShapeMeshCache.h` | NeuronClient | **New** — singleton cache + `ShapeMeshGPU` + `FragmentGPURange` structs |
| `NeuronClient/ShapeMeshCache.cpp` | NeuronClient | **New** — `CountTotalTriangles` DFS, `WriteFragmentVertices` DFS, `EnsureUploaded` (default-heap VB + ring-buffer upload + barriers), `GetGPUData`, `Release`, `Shutdown` |
| `NeuronClient/ShapeStatic.h` | NeuronClient | **Modified** — added `m_ownerShape` pointer to `ShapeFragmentData`, declared `RenderNative()` |
| `NeuronClient/ShapeStatic.cpp` | NeuronClient | **Modified** — added `SetOwner` DFS in `Load()`, implemented `RenderNative()`, switched `ShapeStatic::Render()` to call `EnsureUploaded` + `RenderNative` |
| `NeuronClient/ShapeInstance.cpp` | NeuronClient | **Modified** — `Render()` calls `EnsureUploaded` + `RenderNative` instead of old GL path |
| `NeuronClient/opengl_directx.cpp` | NeuronClient | **Modified** — added `ShapeMeshCache::Get().Shutdown()` in `Direct3DShutdown()` |

#### What This Keeps from the GL Layer (temporarily)

- Matrix stack (`Push/Pop/Multiply`) — needed for fragment hierarchy transforms
- `PrepareDrawState()` — still builds PSO key and uploads `DrawConstants` per draw
- `uploadAndBindConstants()` — still uploads per-draw CB (672 bytes)
- `glEnable/glDisable(GL_COLOR_MATERIAL)` — still set before/after `Render()` calls

#### What This Eliminates

- Per-frame vertex assembly (`glVertex3f_impl` × thousands of vertices)
- Per-frame vertex ring-buffer upload for all mesh data
- `glBegin/glEnd` for all shape rendering (`RenderSlow()` no longer called)

#### Bug Found and Fixed

**Normal negation mismatch.** The GL emulation layer's `glNormal3f_impl` negates all normals (`nx = -nx`, etc.) as part of the RH→LH coordinate conversion.  `WriteFragmentVertices` initially stored normals un-negated, causing incorrect lighting.

- **Symptom:** `SphereWorld` (camera inside sphere) rendered with no visible color — outward-facing un-negated normals gave negative N·L → zero diffuse.  Other shapes (viewed from outside) appeared acceptable but had subtly incorrect lighting.
- **Fix:** Negate normals in `WriteFragmentVertices` to match `glNormal3f_impl` convention.

#### Remaining Phase 0 Overhead

Fragment transforms are computed recursively via the matrix stack.  `DrawConstants.WorldMatrix` changes per fragment, requiring a CB upload per fragment draw.  This matches the old behavior and is acceptable.  Phase 5 can reduce this with per-instance transform buffers.

---

### Phase 1: Batched Billboard/Quad Renderer 🔧 IN PROGRESS

**Status:** Core infrastructure built and verified.  Highest-impact renderers migrated (Darwinians, Virii, spirits, shadows, building lights/ports).  Remaining ~10 renderers with billboard quads are lower priority and can be migrated incrementally.

#### What Was Built

| Component | Description |
|---|---|
| **`QuadBatcher`** (singleton) | CPU-side vertex accumulator.  `Emit(v0,v1,v2,v3)` decomposes a quad into 2 triangles (6 `CustomVertex` entries), appending to a `std::vector`.  `Flush()` uploads to the ring buffer and issues a single `DrawInstanced` via `PrepareDrawState(TRIANGLELIST)`.  Callers manage GL state; the batcher is intentionally "dumb". |
| **`RecordBatchedDraw`** | New function in `opengl_directx.h/.cpp` that increments `FrameStats::drawCalls` and `uploadBytes` for batched draws, keeping metrics accurate without going through `issueDrawCall`. |
| **Static helpers** | `PackColorBGRA(r,g,b,a)`, `PackColorF(r,g,b,a)`, `MakeVertex(x,y,z,color,u,v)` — used by all migrated renderers to construct `CustomVertex` values without touching the GL color/texcoord state. |

#### Design Decisions

1. **"Dumb accumulator" pattern.**  The batcher does NOT manage GL state, texture binds, or blend modes.  Callers set GL state before emitting quads, then call `Flush()`.  This keeps the batcher simple and lets it coexist with unmigrated GL code during the transition.

2. **Flush-before-state-change discipline.**  Migrated renderers call `batcher.Flush()` before any GL state change (e.g., `glDepthMask`, `glBlendFunc`, `glBindTexture`).  This ensures accumulated quads are drawn with the correct PSO.  The pattern:
   ```
   Emit quads (same state)...
   Flush()                          // draw accumulated quads
   glDepthMask(false);              // change state
   glBegin/glEnd                    // rare inline draw
   glDepthMask(true);               // restore state
   Emit more quads...               // resume accumulating
   ```

3. **Per-pass flush in callers.**  `Team::RenderVirii()`, `Team::RenderDarwinians()`, and `Location::RenderSpirits()` call `QuadBatcher::Get().Flush()` after their entity loops, before restoring GL state.  This collapses N entity draws into 1 draw call.

4. **Rare quads stay inline.**  DarwinianRenderer's shadow, blue glow, GodDish shadow, and santa hat quads use different GL states (depthMask, blend func, texture).  They remain as `glBegin/glEnd` inline draws.  They're rare per-frame and the overhead is negligible.  They can be migrated in Phase 5 or when the renderer is restructured for multi-pass rendering.

#### Files Changed

| File | Project | Action |
|---|---|---|
| `NeuronClient/QuadBatcher.h` | NeuronClient | **New** — singleton class with `Emit`, `Flush`, `GetCount`, `PackColorBGRA`, `PackColorF`, `MakeVertex` |
| `NeuronClient/QuadBatcher.cpp` | NeuronClient | **New** — `Emit` decomposes quad to 6 vertices; `Flush` does ring-buffer `Allocate` + `memcpy` + `PrepareDrawState(TRIANGLELIST)` + `IASetVertexBuffers` + `DrawInstanced` + `RecordBatchedDraw` |
| `NeuronClient/opengl_directx.h` | NeuronClient | **Modified** — added `RecordBatchedDraw(unsigned int _uploadBytes)` declaration |
| `NeuronClient/opengl_directx.cpp` | NeuronClient | **Modified** — added `RecordBatchedDraw` implementation (increments `FrameStats::drawCalls` and `uploadBytes`) |
| `GameRender/ShadowRenderer.cpp` | GameRender | **Modified** — shadow color computed once in `Begin()`; `Render()` emits quad via `Emit()`; `End()` calls `Flush()` |
| `GameRender/SpiritRenderer.cpp` | GameRender | **Modified** — inner + outer glow quads emitted via `Emit()` per spirit |
| `GameRender/DefaultBuildingRenderer.cpp` | GameRender | **Modified** — `RenderLights`: GL state set once, all light quads batched (10 per light × N lights), single `Flush()`.  `RenderPorts`: split into shape pass + batched status-light quad pass |
| `GameRender/ViriiRenderer.cpp` | GameRender | **Modified** — removed `glBegin/glEnd`; worm segments and glow quads emitted via `Emit()` per history entry |
| `GameRender/DarwinianRenderer.cpp` | GameRender | **Modified** — main sprite quad and dead fragment quads (3 per dead darwinian) use `Emit()`.  Shadow, glow, GodDish shadow, and santa hat remain inline `glBegin/glEnd` with `Flush()` guards |
| `Starstrike/location.cpp` | Starstrike | **Modified** — added `#include "QuadBatcher.h"` and `QuadBatcher::Get().Flush()` after spirit rendering loop |
| `Starstrike/team.cpp` | Starstrike | **Modified** — added `#include "QuadBatcher.h"` and `QuadBatcher::Get().Flush()` after both `RenderVirii` and `RenderDarwinians` entity loops |

#### What This Keeps from the GL Layer (temporarily)

- `PrepareDrawState()` — `Flush()` calls it to build PSO, upload `DrawConstants` CB, and bind textures/samplers.  One CB upload per `Flush()` instead of one per quad.
- `glEnable/glDisable/glBlendFunc/glDepthMask` — callers still use these to configure render state before `Flush()`.
- `glBindTexture` — callers still bind textures via GL API.
- Inline `glBegin/glEnd` for rare quads (DarwinianRenderer shadow/glow/GodDish/santa).

#### What This Eliminates

- Per-quad `glBegin/glEnd` → `issueDrawCall` for all high-frequency billboard quads (spirits, shadows, darwinian sprites, virii worm segments, building lights/ports).
- Per-quad `DrawConstants` upload (672 bytes × N quads → 672 bytes × 1 flush per pass).
- Per-quad PSO hash + lookup.
- Per-quad vertex ring-buffer upload (N × 6 × 48 bytes → 1 bulk upload per flush).
- Per-quad viewport/scissor/root-signature re-bind.

#### Batching Impact (estimated)

| Pass | Before (draw calls) | After (draw calls) | Reduction |
|---|---|---|---|
| Spirits (2000 spirits × 2 quads) | ~4000 | **1** | 4000× |
| Darwinian main sprites (500 darwinians) | ~500 | **1** | 500× |
| Virii (100 virii × ~10 segments × 2 quads) | ~100 | **1** | 100× |
| Shadows (200 entities) | ~200 | **1** | 200× |
| Building lights (50 buildings × 10 quads) | ~500 | **1** | 500× |

#### Remaining Phase 1 Work

The following renderers still use `glBegin(GL_QUADS)` for billboard-style quads and are candidates for `QuadBatcher` migration.  They are lower frequency than the migrated renderers:

| Renderer | Project | Quads/frame | Priority |
|---|---|---|---|
| `ArmyAntRenderer` | GameRender | Low (few ants) | Low |
| `CentipedeRenderer` | GameRender | Low | Low |
| `SporeGeneratorRenderer` | GameRender | Medium | Medium |
| `TriffidRenderer` | GameRender | Low | Low |
| `PowerupRenderer` | GameRender | Low | Low |
| `SoulDestroyer`/`SpiderRenderer` | GameRender | Low | Low |
| DarwinianRenderer rare quads (shadow, glow, GodDish, santa) | GameRender | Low (conditional) | Deferred to Phase 5 |

These can be migrated incrementally without blocking Phase 2.

**Verification:** Visual comparison on a stress level.  Confirm `FrameStats::drawCalls` drops by an order of magnitude for spirit/darwinian/virii passes.

---

### Phase 2: Landscape & Water Native Vertex Buffers

**Problem:** `landscape_renderer.cpp` and `water.cpp` already use VBO extensions (`gglGenBuffersARB/gglBindBufferARB/gglBufferDataARB`), but these go through the GL VBO emulation in `ogl_extensions_directx.cpp` which wraps D3D12 vertex buffers.  They then call `glDrawArrays` which re-assembles vertices through the `CustomVertex` scratch buffer.

**Solution:** Replace the VBO extension calls with direct `ID3D12Resource` vertex buffers and native draw calls.  These two files are self-contained and use a simple pattern (create VBO once, draw strips repeatedly), making them ideal early migration targets.

**Approach:**
1. Landscape: Replace `gglGenBuffersARB` + `gglBufferDataARB` with direct `CreateCommittedResource` + ring-buffer upload.  Replace `glDrawArrays(GL_TRIANGLE_STRIP, ...)` with `cmdList->IASetVertexBuffers` + `cmdList->DrawInstanced`.  Create a `LandscapePSO` (dedicated PSO, may use simplified shader without lighting).
2. Water: Same pattern.  Water uses alpha blending and may need its own PSO variant.

**Impact:** Eliminates the `glDrawArrays` per-vertex CPU loop (1738–1861 in opengl_directx.cpp) for landscape/water.  Also allows retiring the entire VBO extension emulation (`ogl_extensions_directx.cpp`).

**Files changed:**

| File | Project | Action |
|---|---|---|
| `Starstrike/landscape_renderer.cpp` | Starstrike | Replace VBO + glDrawArrays with native D3D12 |
| `Starstrike/water.cpp` | Starstrike | Replace VBO + glDrawArrays with native D3D12 |
| `NeuronClient/ogl_extensions_directx.cpp` | NeuronClient | Can be deleted after migration |

---

### Phase 3: 2D / UI Rendering via SpriteBatch

**Problem:** The UI framework (`darwinia_window.cpp`, `input_field.cpp`, `scrollbar.cpp`, `drop_down_menu.cpp`, `mainmenus.cpp`, `profilewindow.cpp`), the HUD (`taskmanager_interface_icons.cpp` — **46 `glBegin` calls alone**), cursors (`gamecursor.cpp`, `gamecursor_2d.cpp`), and the start sequence all use `glBegin(GL_QUADS)` for 2D textured/colored rectangles.

**Solution:** Per the copilot instructions, `SpriteBatch` is the target for new textured-quad rendering.  Implement or extend `SpriteBatch` as a 2D draw-call batcher:

- Collects 2D quads (screen-space position, UV, color, texture) into a per-frame vertex buffer
- Sorts by texture to minimize SRV switches
- Uses an orthographic projection PSO (no lighting, no fog, alpha blend)
- Flushes at end of UI pass

**Impact:** Eliminates ~80 `glBegin` call sites in UI/HUD code.  UI is not a frame-time bottleneck, but it's a large chunk of the GL API surface.

---

### Phase 4: Particle System & Effects

**Problem:** `particle_system.cpp`, `explosion.cpp`, `clouds.cpp`, `3d_sierpinski_gasket.cpp`, and effect renderers (`MiscEffectRenderer`, `WeaponRenderer`) use `glBegin/glEnd` for particles, beams, and volumetric effects.

**Solution:** A `ParticleBatcher` similar to `QuadBatcher` but supporting:
- Point sprites (for particles) — or camera-aligned quads with instancing
- Line lists (for beams, lasers — `WeaponRenderer::RenderLaser`)
- Triangle strips (for ribbons)

Group particles by texture and blend mode, flush per group.

**Impact:** Collapses many small draw calls into batched draws.  Particles are a significant visual element and can number in the thousands.

---

### Phase 5: Remaining Renderers & Matrix Stack Removal

**Problem:** After phases 0–4, the remaining GL usage is:
- Matrix stack operations (`glPushMatrix/glPopMatrix/glMultMatrixf/glTranslatef/glRotatef/glScalef`) — used everywhere, especially in ShapeStatic fragment hierarchy
- `glEnable/glDisable/glBlendFunc/glDepthMask` for PSO state — used by every renderer
- `glColor4f/glMaterialfv/glLightfv` for material/lighting setup
- Per-draw `DrawConstants` upload via `uploadAndBindConstants()`

**Solution:** Replace the matrix stack with explicit `XMMATRIX` parameters:

1. **ShapeStatic** already computes `predicted` transform per fragment.  Instead of pushing onto the matrix stack, accumulate a world matrix and pass it to the draw call.
2. **Renderers** compute a `Matrix34` or `XMMATRIX` for the entity and pass it to `ShapeInstance::Render`.  Instead of calling `glMultMatrixf(mat)` + `glPushMatrix()`, pass the matrix directly to a new draw function that writes it into `DrawConstants.WorldMatrix`.

Replace GL state calls with explicit PSO selection:

```cpp
enum class RenderMode : uint8_t {
    OpaqueLit,          // Depth write on, blend off, lighting on
    AlphaBlendLit,      // Depth write off, blend alpha, lighting on
    AdditiveUnlit,      // Depth write off, blend additive, lighting off
    OpaqueUnlit,        // Depth write on, blend off, lighting off
    WireframeUnlit,     // Fill mode wireframe
};

// Pre-built PSOs for each mode (created at init)
ID3D12PipelineState* GetModePSO(RenderMode mode);
```

Renderers call `GetModePSO(RenderMode::OpaqueLit)` instead of `glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND)`.

**Impact:** Eliminates the entire `RenderState` struct, `buildPSOKey()`, per-draw PSO hash lookup.  Pre-built PSOs are selected by enum, not computed from scattered state bits.

---

### Phase 6: Remove the GL Emulation Layer

**Precondition:** All `gl*` call sites across all projects have been migrated.

**Action:** Delete the following files:

| File | Lines |
|---|---|
| `NeuronClient/opengl_directx.h` | 308 |
| `NeuronClient/opengl_directx.cpp` | 1905 |
| `NeuronClient/opengl_directx_inline.h` | 109 |
| `NeuronClient/opengl_directx_internals.h` | 27 |
| `NeuronClient/opengl_directx_dlist.h` | 78 |
| `NeuronClient/opengl_directx_dlist.cpp` | ~100 |
| `NeuronClient/opengl_directx_dlist_dev.h` | 40 |
| `NeuronClient/opengl_directx_dlist_dev.cpp` | ~80 |
| `NeuronClient/opengl_directx_matrix_stack.h` | ~40 |
| `NeuronClient/opengl_directx_matrix_stack.cpp` | ~60 |
| `NeuronClient/ogl_extensions.h` | ~110 |
| `NeuronClient/ogl_extensions_directx.cpp` | ~140 |
| **Total** | **~3000 lines** |

Also remove:
- All `GL_*` constant `#define`s
- All `GL*` typedefs
- The `OpenGLTranslationState` class and its `g_glState` global (or refactor it into a proper `RenderBackend` if any of its infrastructure — root signature, SRV allocation — is still needed)

**Retain:** The `PerFrameConstants.hlsli` / `DrawConstants` / `SceneConstants` CB layout if native renderers still use it.  The uber shaders can be retired in favor of specialized shaders, but this is optional (they work fine as-is for shape rendering).

---

## 5. What to Keep from the GL Layer

Not everything needs replacing.  Some subsystems should be **extracted and promoted** rather than deleted:

| Subsystem | Current location | Recommendation |
|---|---|---|
| **Texture resource management** (`glGenTextures/glDeleteTextures/gluBuild2DMipmaps/glCopyTexImage2D`) | `opengl_directx.cpp:1337–1585` | Extract into a standalone `TextureManager` class.  The D3D12 texture creation, SRV allocation, and ring-buffer upload logic is correct and needed regardless of GL layer. |
| **`MatrixStack`** | `opengl_directx_matrix_stack.h/.cpp` | Keep as a utility class (rename to `Neuron::Math::MatrixStack`).  ShapeStatic's recursive fragment rendering naturally needs a stack.  Remove the GL function wrappers (`glPushMatrix/glPopMatrix`), keep the class. |
| **`FrameStats`** | `opengl_directx.h` | Extract into its own header.  Useful for profiling native renderers too. |
| **Root signature** | `d3d12_backend.h` | Keep.  All native renderers (TreeRenderer, ShapeMeshCache, QuadBatcher) should share it. |
| **Sampler management** | `d3d12_backend.h` + `opengl_directx.cpp:391–412` | Extract the static sampler array.  The 6 sampler combinations cover all current use cases. |

---

## 6. Shader Strategy

### Current State

- **Uber vertex shader** — Handles lighting (8 lights), normal transform, color material, fog distance.  All features controlled by `DrawConstants` flags.
- **Uber pixel shader** — Handles 2 texture units with tex-env modes (modulate/replace/decal), fog blending, alpha test.  All features controlled by `DrawConstants` flags.
- **Tree vertex shader** — Simplified (pos + texcoord only, uses `MatDiffuse` for color).
- **Tree pixel shader** — Simplified (one texture sample × color, fog, fade).

### Recommended Approach

**Keep the uber shaders for shape rendering (Phase 0).** They work correctly, match the CB layout, and avoid a shader rewrite during the most complex migration phase.

**Introduce specialized shaders incrementally:**

| Shader pair | For | Simplifications vs. uber |
|---|---|---|
| `BillboardVS/PS` | QuadBatcher (Phase 1) | No lighting, no normals, no color material.  Just pos + color + texcoord → screen.  Fog optional. |
| `LandscapeVS/PS` | Landscape (Phase 2) | Vertex-lit at upload time.  No per-pixel lighting.  Fog + texture only. |
| `SpriteVS/PS` | SpriteBatch (Phase 3) | Orthographic projection, no fog, no lighting.  Just textured quads with alpha blend. |
| `ParticleVS/PS` | ParticleBatcher (Phase 4) | Same as Billboard but may support point sprites or vertex-animated UV. |

Each new shader pair **shares the same `PerFrameConstants.hlsli` CB layout** so the root signature is unchanged.  Unused fields (e.g., `Lights[8]` in billboard shader) are simply ignored — the CB layout is fixed.

---

## 7. Recommended Execution Order

| Step | Phase | Dependencies | Risk | Effort | Status |
|---|---|---|---|---|---|
| 1 | **Phase 0** — ShapeMeshCache | None | Medium (recursive transform, vertex format) | Large | ✅ Done |
| 2 | **Phase 1** — QuadBatcher | None | Low | Medium | 🔧 In progress (core + 5 renderers done) |
| 3 | **Phase 2** — Landscape/Water | None (parallel with 1) | Low | Small | **Next** |
| 4 | **Phase 3** — SpriteBatch for UI | None | Low | Medium | |
| 5 | **Phase 4** — ParticleBatcher | Phase 1 (shares batcher pattern) | Low | Medium | |
| 6 | **Phase 5** — Matrix stack removal + RenderMode PSOs | Phases 0–4 complete | High (touches everything) | Large | |
| 7 | **Phase 6** — Delete GL layer | All above complete | Low (mechanical) | Small | |

Phases 1 and 2 can proceed in parallel on separate branches.  Phase 5 is the riskiest and should be done last, after all other GL consumers are gone.

---

## 8. Migration Metrics

Track these before starting and after each phase:

| Metric | How to measure | Current (estimated) |
|---|---|---|
| `FrameStats::drawCalls` | In-game HUD / PIX | ~500–2000/frame (varies by scene) |
| `FrameStats::uploadBytes` | In-game HUD | ~2–5 MB/frame (vertex + CB data) |
| `FrameStats::psoSwitches` | In-game HUD | ~50–200/frame |
| Frame time (CPU) | PIX / QueryPerformanceCounter | Measure on stress level |
| Frame time (GPU) | PIX GPU capture | Measure on stress level |
| `gl*` call count in source | `Select-String -Pattern "glBegin" \| Measure-Object` | 240 `glBegin` sites |
| GL emulation LOC | Line count of `opengl_directx*` files | ~3000 lines |

**Target after full migration:**
- `glBegin` call count: **0**
- GL emulation LOC: **0**
- Draw calls: **50–200/frame** (10× reduction via batching)
- Upload bytes: **< 1 MB/frame** (vertex data in GPU-resident buffers)

---

## 9. Risks & Mitigations

| Risk | Phase | Mitigation |
|---|---|---|
| **ShapeStatic vertex format mismatch** — Current `CustomVertex` has 48 bytes (pos+normal+color+uv0+uv1). Shapes only use pos+normal+color. Wasting 16 bytes/vertex on unused UVs. | 0 ✅ | Accepted for Phase 0 — used `CustomVertex` with zero UVs for compatibility with uber shader input layout.  Optimize vertex format in Phase 5 with a dedicated shape shader. |
| **Fragment transform accumulation** — ShapeStatic fragments form a tree.  Each fragment's transform is relative to its parent. The matrix stack handles this naturally.  Replacing it requires explicit parent-child matrix multiplication. | 0 ✅, 5 | Phase 0 keeps the matrix stack (confirmed working).  Phase 5 replaces it with explicit `XMMATRIX worldMatrix = parentWorld * localTransform` passed down the recursion. |
| **Normal negation mismatch** — The GL emulation layer negates normals in `glNormal3f_impl` (RH→LH conversion).  Native vertex upload must replicate this negation or lighting breaks. | 0 ✅ | **Encountered and fixed.**  `WriteFragmentVertices` must negate normals.  Future native renderers that bypass GL must apply the same negation.  Symptom was most visible for inside-sphere rendering (SphereWorld) where all normals faced away from camera. |
| **Display list removal breaks `sphere_renderer.cpp`** — Only user of display lists. | 5 | `sphere_renderer` already generates vertices procedurally.  Replace with a static GPU vertex buffer (upload once at init, like TreeRenderer). |
| **`glCopyTexImage2D` is a GPU→GPU copy** — Used for render-to-texture effects.  Cannot be replaced with a simple upload. | 3+ | Extract into `TextureManager::CopyFromBackBuffer()`.  The D3D12 implementation is already correct (barrier → CopyTextureRegion → barrier). |
| **Per-draw CB upload still needed after Phase 0** — ShapeStatic fragments each have a unique world matrix, so DrawConstants must still be uploaded per fragment. | 0 ✅ | Accepted (confirmed in practice).  The CB upload overhead is small compared to the eliminated vertex upload.  Phase 5 can further reduce this with per-instance transform buffers for shapes with many fragments. |
| **Regression in visual output** — The uber shader has specific fixed-function emulation behavior (GL-style lighting model, tex env modes, alpha test).  Native shaders must match exactly. | All | Keep uber shaders as the reference.  New specialized shaders should produce identical output for their use case.  Always A/B compare visually. |
| **Flush-before-state-change discipline** — If a migrated renderer emits quads to the batcher and then changes GL state without flushing first, the accumulated quads will be drawn with the wrong PSO at next flush. | 1 🔧 | Established pattern: always call `batcher.Flush()` before any `glDepthMask`, `glBlendFunc`, `glBindTexture`, or `glDisable(GL_DEPTH_TEST)` call.  DarwinianRenderer's rare quads (shadow, glow, GodDish, santa) flush before each state change. |
| **`std::clamp` name collision** — Naming a lambda `clamp` inside a `static` member function causes MSVC parse errors when `<algorithm>` is in scope via `pch.h`. | 1 🔧 | **Encountered and fixed.**  Renamed lambda to `toByte` in `QuadBatcher::PackColorF`.  Lesson: avoid naming locals after well-known `std::` functions when pch pulls in standard headers. |
| **Interleaved inline GL draws + batched draws** — DarwinianRenderer's rare quads (shadow, glow) use `glBegin/glEnd` inline, interleaved with batched `Emit()` calls for main sprites.  Both paths write to the same D3D12 command list. | 1 🔧 | Safe: inline GL draws go through `issueDrawCall` → separate `DrawInstanced`; batched draws go through `Flush` → separate `DrawInstanced`.  Both read GL state at draw time, not at emit time.  Command list executes in order. |

---

## 10. Relationship to Render.md Recommendations

The existing `Render.md` recommendations remain valid and are **complementary** to this migration:

| Render.md § | Relationship to DX12 migration |
|---|---|
| **2.1** — Eliminate `const_cast` | Independent.  Fix before or during Phase 0. |
| **2.2** — Split `GameRender.cpp` | Independent.  Do anytime. |
| **2.3** — `BillboardHelper` | **Superseded by Phase 1 (`QuadBatcher`).**  If BillboardHelper is implemented first, QuadBatcher replaces the GL calls inside it.  If not, skip BillboardHelper and go directly to QuadBatcher. |
| **2.4** — Consolidate spirits | Depends on 2.3 or Phase 1.  Simpler if QuadBatcher exists. |
| **2.5** — Templatize registries | Independent.  Do anytime. |
| **2.6** — GL state contracts | **Becomes obsolete after Phase 5** (no more GL state).  Still useful during the transitional period. |
| **2.7** — VS project filters | Independent.  Do anytime. |

---

## 11. Out of Scope (Future Opportunities)

These become possible *after* the GL layer is removed but are not part of this plan:

- **GPU-driven rendering** — Indirect draw calls with GPU-side culling.
- **Bindless textures** — Eliminate per-draw SRV binding via SM 6.6 resource descriptors.
- **Mesh shaders** — Replace input assembler + vertex shader with mesh shaders for complex geometry.
- **Compute-based particle system** — GPU simulation + rendering with no CPU readback.
- **Structured buffer instancing** — Replace per-instance CB uploads with a structured buffer containing all instance transforms, indexed by `SV_InstanceID`.
