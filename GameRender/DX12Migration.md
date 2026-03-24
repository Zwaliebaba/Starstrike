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
| **NeuronClient** | 15 | ~260 | `ShapeStatic.cpp` (the biggest: called for every 3D model), `text_renderer.cpp`, `sphere_renderer.cpp`, `3d_sprite.cpp`, `render_utils.cpp` |
| **Total** | **240** | **~3590** | |

### 1.4 Existing Native D3D12 Code

**`TreeRenderer`** is the only renderer that bypasses the GL layer:

- Owns a `GraphicsPSO` with dedicated shaders (`TreeVertexShader.hlsl`/`TreePixelShader.hlsl`)
- Manages per-tree GPU vertex buffers (upload once via ring buffer, reuse across frames)
- Shares the root signature and `DrawConstants`/`SceneConstants` CB layout with the GL layer
- Resolves GL texture IDs to SRV handles via `OpenGLD3D::GetTextureSRVGPUHandle()`

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
| **0** | GPU-resident ShapeStatic meshes | ~23 `m_shape->Render()` call sites (each a deep `glBegin/glEnd` tree) | **Large** | **Very High** — eliminates per-frame vertex re-upload for all 3D models |
| **1** | Batched billboard/quad renderer | ~60 `glBegin(GL_QUADS)` for billboards | Medium | **High** — collapses thousands of spirit/darwinian/effect quads into few draw calls |
| **2** | Landscape & water native VBs | ~3 `glDrawArrays` + VBO setup | Small | Medium — already using VBOs, just remove GL indirection |
| **3** | 2D/UI rendering (SpriteBatch) | ~80 `glBegin` in UI + HUD + cursors | Medium | Medium — UI is not a hot path but it's a large GL surface |
| **4** | Particle system & effects | ~20 `glBegin` in `particle_system`, `explosion`, `clouds` | Medium | Medium–High — particles can be batched/instanced |
| **5** | Remaining renderers & cleanup | ~60 remaining `glBegin` sites | Large | Low — long tail of misc renderers |
| **6** | Remove GL emulation layer | All remaining | Small | Compile-time and code-size win |

---

## 4. Phase Details

### Phase 0: GPU-Resident ShapeStatic Meshes

**Problem:**  `ShapeFragmentData::RenderSlow()` (NeuronClient/ShapeStatic.cpp:772–802) uses `glBegin(GL_TRIANGLES)` to submit every triangle of every fragment of every shape, **every frame**.  A building with 5 fragments × 200 triangles × 3 vertices = 3000 `glVertex3f` calls per building per frame, each going through `glVertex3f_impl` → CPU array → ring buffer → `DrawInstanced`.

There are **23 `m_shape->Render()` call sites** across GameRender, each triggering this path for every entity/building on screen.  For a scene with 50 buildings × 5 fragments, that's 250 `glBegin/glEnd` draw calls just for buildings, plus similar counts for entities.

**Solution:** Follow the `TreeRenderer` pattern:

1. **`ShapeMeshGPU` struct** — Per-ShapeStatic GPU resources:
   ```cpp
   struct FragmentGPURange {
       UINT vertexStart;
       UINT vertexCount;
   };

   struct ShapeMeshGPU {
       com_ptr<ID3D12Resource> vertexBuffer;
       D3D12_VERTEX_BUFFER_VIEW vbView;
       std::vector<FragmentGPURange> fragments; // Indexed by fragment ordinal
       int version = -1; // Matches ShapeStatic load version
   };
   ```

2. **`ShapeMeshCache`** — Singleton cache mapping `ShapeStatic*` → `ShapeMeshGPU`.  Uploads vertex data (pos + normal + color) to a default-heap vertex buffer on first render.  Since shapes are loaded once and never modified, the GPU buffer is valid for the lifetime of the shape.

3. **`ShapeStatic::RenderSlow()` replacement** — Instead of `glBegin/glEnd`, look up the GPU buffer and issue `DrawInstanced` for the fragment's vertex range:
   ```cpp
   void ShapeFragmentData::RenderNative(const FragmentState* states, float predictionTime) const
   {
       auto& cache = ShapeMeshCache::Get();
       auto* gpu = cache.Get(/* owning ShapeStatic */);
       if (!gpu) return;

       // Push transform (still uses matrix stack for now — Phase 5 removes this)
       auto& mv = OpenGLD3D::GetModelViewStack();
       mv.Push();
       mv.Multiply(predicted);

       // Bind constants + PSO + draw
       auto& range = gpu->fragments[m_fragmentIndex];
       OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
       cmdList->IASetVertexBuffers(0, 1, &gpu->vbView);
       cmdList->DrawInstanced(range.vertexCount, 1, range.vertexStart, 0);

       // Recurse into children
       for (int i = 0; i < m_childFragments.Size(); ++i)
           m_childFragments.GetData(i)->RenderNative(states, predictionTime);

       mv.Pop();
   }
   ```

4. **Vertex format** — Reuse the existing `CustomVertex` layout (48 bytes) initially so the uber shader works without changes.  The GPU buffer stores pre-computed pos/normal/color per vertex.  UVs are zero (shapes don't use textures — they use vertex colors with `GL_COLOR_MATERIAL`).

**What this keeps from the GL layer (temporarily):**
- Matrix stack (`glPushMatrix/glPopMatrix`) — needed for fragment hierarchy
- `PrepareDrawState` — still builds PSO and uploads `DrawConstants`
- `uploadAndBindConstants` — still uploads per-draw CB

**What this eliminates:**
- Per-frame vertex assembly (`glVertex3f_impl` × thousands)
- Per-frame vertex ring-buffer upload for mesh data
- `glBegin/glEnd` for all shape rendering

**Files changed:**

| File | Project | Action |
|---|---|---|
| `NeuronClient/ShapeMeshCache.h` | NeuronClient | New — singleton cache + `ShapeMeshGPU` struct |
| `NeuronClient/ShapeMeshCache.cpp` | NeuronClient | New — upload logic (follows TreeRenderer pattern) |
| `NeuronClient/ShapeStatic.h` | NeuronClient | Add `RenderNative()` to `ShapeFragmentData` |
| `NeuronClient/ShapeStatic.cpp` | NeuronClient | Implement `RenderNative()`, keep `RenderSlow()` as fallback |
| `NeuronClient/ShapeInstance.cpp` | NeuronClient | `Render()` calls `RenderNative()` instead of `RenderSlow()` |
| `NeuronClient/opengl_directx.cpp` | NeuronClient | Expose `PrepareDrawState` and ring-buffer CB upload as public functions for native callers |

**Verification:** Render any level with buildings and entities.  All 3D models must look identical.  Use `FrameStats` to confirm draw call count is unchanged but `uploadBytes` drops significantly (vertex data no longer uploaded per frame).

**Risk:** Fragment transforms are computed recursively using the matrix stack.  The `DrawConstants.WorldMatrix` changes per fragment, so we still need a CB upload per fragment draw.  This is not a regression — it matches the current behavior — but it means the full CB-per-draw overhead remains until Phase 5 introduces per-instance transform buffers.

---

### Phase 1: Batched Billboard/Quad Renderer

**Problem:** Darwinians, Virii, spirits, shadows, weapon effects, and building lights/ports all draw camera-aligned quads via individual `glBegin(GL_QUADS)...glEnd()` calls.  A scene with 2000 Darwinians generates 2000+ draw calls, each with its own CB upload and PSO bind.

**Solution:** A `QuadBatcher` that collects quads into a CPU-side vertex array and flushes them in a single draw call per unique PSO state:

```cpp
// NeuronClient/QuadBatcher.h
class QuadBatcher
{
public:
    static QuadBatcher& Get();

    // Begin a batch.  All subsequent EmitQuad calls use this state.
    void Begin(D3D12_GPU_DESCRIPTOR_HANDLE textureSRV, BlendMode blend);

    // Add one quad (4 vertices → 6 indices, pre-decomposed to 2 triangles).
    void EmitQuad(FXMVECTOR pos, float halfW, float halfH,
                  uint32_t colorBGRA, float u0, float v0, float u1, float v1);

    // Flush all accumulated quads as a single draw call.
    void Flush();

private:
    std::vector<CustomVertex> m_vertices;
    D3D12_GPU_DESCRIPTOR_HANDLE m_textureSRV;
    BlendMode m_blend;
};
```

**Usage in Virii/Darwinian pass:**

```cpp
void Team::RenderVirii(float predictionTime)
{
    auto& batcher = QuadBatcher::Get();
    batcher.Begin(viriiTextureSRV, BlendMode::Additive);

    for (int i = 0; i < m_others.Size(); ++i)
    {
        Entity* e = m_others[i];
        if (e->m_type != Entity::TypeVirii) continue;
        // Compute pos, size, color from entity
        batcher.EmitQuad(pos, halfSize, halfSize, color, 0, 0, 1, 1);
    }

    batcher.Flush(); // ONE draw call for all virii
}
```

**Impact:** Collapses thousands of draw calls into a handful (one per texture × blend mode combination per pass).  Eliminates thousands of CB uploads, PSO lookups, and vertex buffer binds.

**Depends on:** Nothing (can be done in parallel with Phase 0).  However, the `BillboardHelper` from `Render.md §2.3` is a stepping stone — if that's implemented first, `QuadBatcher` replaces the GL calls inside `BillboardHelper`.

**Files changed:**

| File | Project | Action |
|---|---|---|
| `NeuronClient/QuadBatcher.h` | NeuronClient | New |
| `NeuronClient/QuadBatcher.cpp` | NeuronClient | New — accumulate + flush logic |
| `GameRender/DarwinianRenderer.cpp` | GameRender | Replace `glBegin/glEnd` with `QuadBatcher::EmitQuad` |
| `GameRender/ViriiRenderer.cpp` | GameRender | Replace `glBegin/glEnd` with `QuadBatcher::EmitQuad` |
| `GameRender/SpiritRenderer.cpp` | GameRender | Replace `glBegin/glEnd` with `QuadBatcher::EmitQuad` |
| `GameRender/ShadowRenderer.cpp` | GameRender | Replace `glBegin/glEnd` with `QuadBatcher::EmitQuad` |
| `GameRender/DefaultBuildingRenderer.cpp` | GameRender | `RenderLights`/`RenderPorts` use `QuadBatcher` |
| (+ ~10 more renderers) | GameRender | Migrate billboard quads |

**Verification:** Visual comparison on a stress level.  Confirm `FrameStats::drawCalls` drops by an order of magnitude for passes 3–4.

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

| Step | Phase | Dependencies | Risk | Effort |
|---|---|---|---|---|
| 1 | **Phase 0** — ShapeMeshCache | None | Medium (recursive transform, vertex format) | Large |
| 2 | **Phase 1** — QuadBatcher | None (parallel with 0) | Low | Medium |
| 3 | **Phase 2** — Landscape/Water | None (parallel with 0–1) | Low | Small |
| 4 | **Phase 3** — SpriteBatch for UI | None | Low | Medium |
| 5 | **Phase 4** — ParticleBatcher | Phase 1 (shares batcher pattern) | Low | Medium |
| 6 | **Phase 5** — Matrix stack removal + RenderMode PSOs | Phases 0–4 complete | High (touches everything) | Large |
| 7 | **Phase 6** — Delete GL layer | All above complete | Low (mechanical) | Small |

Phases 0, 1, and 2 can proceed in parallel on separate branches.  Phase 5 is the riskiest and should be done last, after all other GL consumers are gone.

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
| **ShapeStatic vertex format mismatch** — Current `CustomVertex` has 48 bytes (pos+normal+color+uv0+uv1). Shapes only use pos+normal+color. Wasting 16 bytes/vertex on unused UVs. | 0 | Use `CustomVertex` initially for compatibility with uber shader input layout.  Optimize vertex format in Phase 5 with a dedicated shape shader. |
| **Fragment transform accumulation** — ShapeStatic fragments form a tree.  Each fragment's transform is relative to its parent. The matrix stack handles this naturally.  Replacing it requires explicit parent-child matrix multiplication. | 0, 5 | Phase 0 keeps the matrix stack.  Phase 5 replaces it with explicit `XMMATRIX worldMatrix = parentWorld * localTransform` passed down the recursion.  This is actually simpler and avoids global mutable state. |
| **Display list removal breaks `sphere_renderer.cpp`** — Only user of display lists. | 5 | `sphere_renderer` already generates vertices procedurally.  Replace with a static GPU vertex buffer (upload once at init, like TreeRenderer). |
| **`glCopyTexImage2D` is a GPU→GPU copy** — Used for render-to-texture effects.  Cannot be replaced with a simple upload. | 3+ | Extract into `TextureManager::CopyFromBackBuffer()`.  The D3D12 implementation is already correct (barrier → CopyTextureRegion → barrier). |
| **Per-draw CB upload still needed after Phase 0** — ShapeStatic fragments each have a unique world matrix, so DrawConstants must still be uploaded per fragment. | 0 | Accepted.  The CB upload overhead is small compared to the eliminated vertex upload.  Phase 5 can further reduce this with per-instance transform buffers for shapes with many fragments. |
| **Regression in visual output** — The uber shader has specific fixed-function emulation behavior (GL-style lighting model, tex env modes, alpha test).  Native shaders must match exactly. | All | Keep uber shaders as the reference.  New specialized shaders should produce identical output for their use case.  Always A/B compare visually. |

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
