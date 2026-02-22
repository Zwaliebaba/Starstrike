# OpenGL 1.x → DirectX 11 Migration Plan

## 1. Executive Summary

The Starstrike codebase has been **fully migrated from OpenGL 1.x to DirectX 11**. Zero OpenGL headers, libraries, API calls, or GL type aliases remain in any compiled source file. The project links exclusively against `d3d11.lib` and `dxgi.lib`.

The migration was completed in 10 phases, replacing ~95 source files worth of `gl*`/`glu*` calls with a thin D3D11 abstraction layer (`RenderDevice`, `ImRenderer`, `RenderStates`, `TextureManager`). The landscape uses a static `ID3D11Buffer` vertex buffer; all other rendering goes through `ImRenderer`'s immediate-mode emulation.

**Remaining work** (Phase 10 deferred items): batch optimisation, `Camera::Get2DScreenPos()` migration, water multitexture lightmap.

---

## 2. Guiding Principles

1. **Thin abstraction layer** — Introduce a `RenderDevice` abstraction in NeuronClient that wraps D3D11. Do NOT build a fully generic RHI. The abstraction exists only to localise D3D11 calls and let game code stop calling `gl*` directly.
2. **Phase-by-phase** — Each phase produces a compilable, runnable build. No "big bang" rewrite.
3. **Preserve legacy patterns** — Keep `new`/`delete`, global pointers (`g_renderDevice`), raw C strings, and existing naming conventions as prescribed by `.github/copilot-instructions.md`.
4. **Compile-gate with `USE_DIRECT3D11`** — New code lives behind `#ifdef USE_DIRECT3D11` / `#else` (legacy OpenGL). Old `USE_DIRECT3D` (D3D9) blocks have been removed (Phase 0).

---

## 3. Phase 0 — Preparation & Cleanup ✅ COMPLETED

All `#ifdef USE_DIRECT3D` / D3D9 code paths have been removed. Zero `USE_DIRECT3D` references remain in compiled code.

### 3.1 D3D9-Only Files Emptied (content wrapped in `#if 0`)

| File | What was removed |
|---|---|
| `NeuronClient\shader.h` | D3D9 `Shader` class (now empty header with include guard) |
| `NeuronClient\shader.cpp` | Entire D3D9 shader implementation |
| `NeuronClient\texture.h` | D3D9 `Texture` class (now empty header with include guard) |
| `NeuronClient\texture.cpp` | Entire D3D9 texture implementation |
| `Starstrike\deform.h` | D3D9 `DeformEffect` class (now empty header with include guard) |
| `Starstrike\deform.cpp` | Entire D3D9 deform effect implementation |
| `Starstrike\water_reflection.h` | D3D9 `WaterReflectionEffect` class (now empty header with include guard) |
| `Starstrike\water_reflection.cpp` | Entire D3D9 water reflection implementation |

### 3.2 File Deleted

- `NeuronClient\opengl_directx_internals.h` — was already empty

### 3.3 Mixed Files Cleaned (D3D9 `#ifdef` blocks removed, OpenGL paths kept)

| File | Blocks removed |
|---|---|
| `NeuronClient\bitmap.cpp` | `#if !defined USE_DIRECT3D` conditional in `ConvertToTexture()` |
| `NeuronClient\debug_render.cpp` | D3D9 label string in `PrintMatrices()` |
| `NeuronClient\ogl_extensions.cpp` | `#if !defined USE_DIRECT3D` wrapper (now unconditional) |
| `NeuronClient\window_manager.cpp` | `#if !defined USE_DIRECT3D` wrapper (now unconditional) |
| `Starstrike\landscape_renderer.h` | `IDirect3DVertexBuffer9` forward decl, `RenderModeVertexBufferDirect3D` enum, `ReleaseD3D*` methods |
| `Starstrike\landscape_renderer.cpp` | 7 D3D9 blocks (vertex decl, stream source, DrawPrimitive, normal flip, colour conversion, release functions) |
| `Starstrike\location.cpp` | 4 D3D9 blocks (specular enable/disable, clip plane, deform effect) |
| `Starstrike\water.h` | `IDirect3DVertexBuffer9*` member, `ReleaseD3D*` methods |
| `Starstrike\water.cpp` | 17 D3D9 blocks (vertex buffer, vertex declaration, colour table, draw primitives, hw vertex processing guards) |
| `GameLogic\feedingtube.cpp` | 3 D3D9 blocks (SwapToViewMatrix/SwapToModelMatrix, thisDistance override) |
| `GameLogic\radardish.cpp` | 4 D3D9 blocks (vertex decl include, SwapToViewMatrix/SwapToModelMatrix, thisDistance override) |
| `GameLogic\spiritreceiver.cpp` | 4 D3D9 blocks (water reflection guards, batched spirit rendering) |
| `GameLogic\tree.cpp` | 3 D3D9 blocks (D3D9 include, colour hacks in display lists) |

### 3.4 Stale References Removed

| Item | File |
|---|---|
| `#define D3D_DEBUG_INFO` | `NeuronClient\NeuronClient.h` |
| `#include "water_reflection.h"` | `GameLogic\spiritreceiver.cpp`, `Starstrike\water.cpp` |

### 3.5 OpenGL Includes Retained (for now)
`NeuronClient\NeuronClient.h` still contains the GL includes and link pragmas. These will be removed in Phase 9.
```cpp
#include <GL/gl.h>
#include <GL/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
```

**Verification:** Build passes. No `USE_DIRECT3D`, `IDirect3D*`, `OpenGLD3D`, `D3DCOLOR`, or `opengl_directx_internals` references remain in any compiled code.

---

## 4. Phase 1 — D3D11 Device & Swap Chain Bootstrap ✅ COMPLETED

The D3D11 device and swap chain are created alongside the existing OpenGL context. OpenGL continues to handle all rendering during the transition. The D3D11 device is available via `g_renderDevice` for use by future phases.

### 4.1 New Files Created

| File | Project | Purpose |
|---|---|---|
| `NeuronClient\render_device.h` | NeuronClient | `RenderDevice` class declaration |
| `NeuronClient\render_device.cpp` | NeuronClient | D3D11 device, context, swap chain init/teardown |

### 4.2 Implementation Details

- `RenderDevice` class with `Initialise()`, `Shutdown()`, `BeginFrame()`, `EndFrame()`, `OnResize()`.
- Uses `D3D11CreateDeviceAndSwapChain` with `D3D_FEATURE_LEVEL_11_0`.
- Creates depth-stencil buffer (`DXGI_FORMAT_D24_UNORM_S8_UINT`) and render target view.
- Debug layer enabled in `_DEBUG` builds, with fallback retry without it.
- Links `d3d11.lib`, `dxgi.lib` via `#pragma comment(lib, ...)`.
- Global `g_renderDevice` pointer follows existing singleton pattern.

### 4.3 Integration

**File:** `NeuronClient\window_manager.cpp`

- `CreateWin()`: After `EnableOpenGL()`, creates `g_renderDevice` and calls `Initialise()` with the same HWND, dimensions, and vsync settings. Falls back gracefully if D3D11 init fails.
- `DestroyWin()`: Deletes `g_renderDevice` before `DisableOpenGL()`.
- OpenGL context (`EnableOpenGL`/`DisableOpenGL`/`Flip`) remains active — all current rendering still goes through OpenGL.

### 4.4 Remaining Phase 1 Work (deferred to later phases)

The following items from the original plan are deferred because they would break OpenGL rendering before the `ImRenderer` replacement layer exists:

- Removing `EnableOpenGL()` / `DisableOpenGL()` / `wglCreateContext` → Phase 9
- Replacing `glClearColor` + `glClear` with `g_renderDevice->BeginFrame()` → Phase 6
- Replacing `g_windowManager->Flip()` with `g_renderDevice->EndFrame()` → Phase 6
- Removing `InitialiseOGLExtensions()` / `BuildOpenGlState()` → Phase 9

**Verification:** Build passes. Game runs normally with OpenGL rendering. `DebugTrace` confirms D3D11 device creation in Output window.

---

## 5. Phase 2 — Immediate-Mode Replacement Layer ✅ COMPLETED

The `ImRenderer` class provides a drop-in replacement for OpenGL immediate-mode rendering (`glBegin`/`glEnd` with interleaved vertex attributes). It batches vertices into a dynamic D3D11 vertex buffer and flushes on `End()`.

### 5.1 New Files Created

| File | Project | Purpose |
|---|---|---|
| `NeuronClient\im_renderer.h` | NeuronClient | `ImRenderer` class + `PrimitiveType` enum |
| `NeuronClient\im_renderer.cpp` | NeuronClient | Vertex batching, shader compilation, draw |

### 5.2 Implementation Details

- **Shaders** pre-compiled from `.hlsl` files to `.h` headers via Visual Studio `FxCompile` build action. No runtime `D3DCompile` or `d3dcompiler.lib` dependency.
  - `Shaders\im_defaultVS.hlsl` → `CompiledShaders\im_defaultVS.h` (`g_pim_defaultVS[]`, vs_5_0, entry `main`)
  - `Shaders\im_coloredPS.hlsl` → `CompiledShaders\im_coloredPS.h` (`g_pim_coloredPS[]`, ps_5_0, entry `main`)
  - `Shaders\im_texturedPS.hlsl` → `CompiledShaders\im_texturedPS.h` (`g_pim_texturedPS[]`, ps_5_0, entry `main`)
  - `im_renderer.cpp` includes the generated `.h` files and calls `CreateVertexShader`/`CreatePixelShader` directly with the bytecode arrays.
- **Dynamic vertex buffer**: 65536 vertices, `D3D11_USAGE_DYNAMIC` / `MAP_WRITE_DISCARD`.
- **Default sampler state**: `MIN_MAG_MIP_LINEAR`, `ADDRESS_CLAMP`, bound automatically when a texture is active.
- **Constant buffer**: single `float4x4 gWorldViewProj`, transposed for HLSL column-major.
- **Primitive conversion** in `End()`:
  - `PRIM_QUADS` → triangle list (0-1-2, 0-2-3 per quad) — 203 call sites
  - `PRIM_QUAD_STRIP` → triangle list — 3 call sites
  - `PRIM_TRIANGLE_FAN` → triangle list — 1 call site
  - `PRIM_LINE_LOOP` → line strip + closing vertex — 22 call sites
  - `PRIM_TRIANGLES`, `PRIM_TRIANGLE_STRIP`, `PRIM_LINES`, `PRIM_LINE_STRIP`, `PRIM_POINTS` → native D3D11 topologies
- **Vertex attribute setters** cover all GL variants found in codebase: `Vertex2f/2fv/2i/3f/3fv`, `Color3f/3ub/3ubv/4f/4fv/4ub/4ubv`, `TexCoord2f/2i`, `Normal3f/3fv`.
- **Texture binding**: `BindTexture(ID3D11ShaderResourceView*)` / `UnbindTexture()` selects textured vs colored pixel shader. Sampler is bound automatically.
- **Matrix state**: `SetProjectionMatrix`, `SetViewMatrix`, `SetWorldMatrix` — WVP computed and uploaded in `Flush()`.
- **Modelview matrix stack** (added per review finding #1):
  - `PushMatrix()` / `PopMatrix()` — mirrors OpenGL `GL_MODELVIEW` stack (max depth 32)
  - `LoadIdentity()` — resets world matrix
  - `MultMatrixf(const float* m)` — column-major 4×4 input (OpenGL layout), transposed to row-major for DirectXMath, post-multiplied onto world matrix (matches `glMultMatrixf` semantics)
  - `Translatef(x, y, z)`, `Rotatef(angleDeg, x, y, z)`, `Scalef(sx, sy, sz)` — convenience wrappers
  - Covers 30 `glPushMatrix`/`glPopMatrix` call sites, 6 `glTranslatef`, 2 `glScalef`, 4 `glMultMatrixf` across `camera.cpp`, `shape.cpp`, `sphere_renderer.cpp`, `tree.cpp`, `global_world.cpp`, `global_internet.cpp`, `startsequence.cpp`, `debug_render.cpp`, `feedingtube.cpp`, `radardish.cpp`
  - `GetProjectionMatrix()` / `GetWorldMatrix()` accessors for save/restore patterns

### 5.3 Integration

**File:** `NeuronClient\window_manager.cpp`

- `CreateWin()`: After `RenderDevice` init, creates `g_imRenderer` and calls `Initialise()`.
- `DestroyWin()`: Deletes `g_imRenderer` before `g_renderDevice`.

### 5.4 Status

The `ImRenderer` is fully functional but not yet called by any game code. OpenGL continues to handle all rendering. Game code migration to use `g_imRenderer->*` calls will happen in Phases 6–8.

**Verification:** Build passes. `DebugTrace` confirms "ImRenderer initialised" in Output window.

---

## 6. Phase 3 — Render State Abstraction ✅ COMPLETED

### 6.1 New Files Created

| File | Project | Purpose |
|---|---|---|
| `NeuronClient\render_states.h` | NeuronClient | `RenderStates` class, `BlendStateId`/`DepthStateId`/`RasterStateId` enums |
| `NeuronClient\render_states.cpp` | NeuronClient | Pre-built `ID3D11BlendState`, `ID3D11DepthStencilState`, `ID3D11RasterizerState` objects |

### 6.2 State Objects Pre-Created

All state objects are created at startup in `RenderStates::Initialise()` and applied via `SetBlendState(ctx, id)`, `SetDepthState(ctx, id)`, `SetRasterState(ctx, id)`. Current state is queryable via `GetCurrentBlendState()` etc. for save/restore patterns.

**Blend States** (`BlendStateId` enum, 7 entries):

| Enum | OpenGL Equivalent | D3D11 Src/Dest | Usage |
|---|---|---|---|
| `BLEND_DISABLED` | `glDisable(GL_BLEND)` | — | Default |
| `BLEND_ALPHA` | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` | `SRC_ALPHA / INV_SRC_ALPHA` | Standard alpha |
| `BLEND_ADDITIVE` | `GL_SRC_ALPHA, GL_ONE` | `SRC_ALPHA / ONE` | Particles, lasers |
| `BLEND_ADDITIVE_PURE` | `GL_ONE, GL_ONE` | `ONE / ONE` | Pure additive |
| `BLEND_SUBTRACTIVE_COLOR` | `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR` | `SRC_ALPHA / INV_SRC_COLOR` | Text shadow |
| `BLEND_MULTIPLICATIVE` | `GL_ZERO, GL_SRC_COLOR` | `ZERO / SRC_COLOR` | Multiplicative |
| `BLEND_DEST_MASK` | `GL_ZERO, GL_ONE_MINUS_SRC_ALPHA` | `ZERO / INV_SRC_ALPHA` | Dest masking |

**Depth-Stencil States** (`DepthStateId` enum, 3 entries):

| Enum | OpenGL Equivalent | D3D11 |
|---|---|---|
| `DEPTH_ENABLED_WRITE` | `glEnable(GL_DEPTH_TEST)` + `glDepthMask(true)` | `DepthEnable=TRUE, LESS_EQUAL, WriteMask=ALL` |
| `DEPTH_ENABLED_READONLY` | `glEnable(GL_DEPTH_TEST)` + `glDepthMask(false)` | `DepthEnable=TRUE, LESS_EQUAL, WriteMask=ZERO` |
| `DEPTH_DISABLED` | `glDisable(GL_DEPTH_TEST)` | `DepthEnable=FALSE` |

**Rasteriser States** (`RasterStateId` enum, 2 entries):

| Enum | OpenGL Equivalent | D3D11 |
|---|---|---|
| `RASTER_CULL_BACK` | `glEnable(GL_CULL_FACE)` / `glFrontFace(GL_CCW)` | `CullMode=BACK, FrontCCW=TRUE` |
| `RASTER_CULL_NONE` | `glDisable(GL_CULL_FACE)` | `CullMode=NONE` |

### 6.2.1 Integration

**File:** `NeuronClient\window_manager.cpp`

- `CreateWin()`: After `ImRenderer` init, creates `g_renderStates` and calls `Initialise()`.
- `DestroyWin()`: Deletes `g_renderStates` before `g_imRenderer`.

**Shader-based states** (no D3D11 fixed-function equivalent):

### 6.3 Alpha Test
D3D11 has no fixed-function alpha test. Add a `float alphaClipThreshold` to the per-frame constant buffer. In the pixel shader:
```hlsl
if (color.a <= gAlphaClipThreshold) discard;
```
Set `gAlphaClipThreshold = -1.0` to disable, `0.01` to mimic `glAlphaFunc(GL_GREATER, 0.01)`.

### 6.4 Fog
Add fog parameters to the constant buffer:
```hlsl
float gFogStart;
float gFogEnd;
float4 gFogColor;
```
Compute in pixel shader:
```hlsl
float fogFactor = saturate((gFogEnd - dist) / (gFogEnd - gFogStart));
color.rgb = lerp(gFogColor.rgb, color.rgb, fogFactor);
```

### 6.5 Migrate `Renderer::SetOpenGLState()` (deferred to Phase 6)

**File:** `Starstrike\renderer.cpp` — `SetOpenGLState()` (lines 730-792)

Replace all `gl*` calls with `g_renderStates->SetBlendState(ctx, BLEND_ALPHA)`, `g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_WRITE)`, `g_renderStates->SetRasterState(ctx, RASTER_CULL_BACK)`, and `g_imRenderer->SetFog(...)` etc.

### 6.6 Migrate `Renderer::CheckOpenGLState()` (deferred to Phase 6)

**File:** `Starstrike\renderer.cpp` — `CheckOpenGLState()` (lines 619-728)

This function is a debug validator. It currently returns immediately (`return;` on line 621). Replace its body with D3D11 equivalents or simply leave it as a no-op initially. Remove all `glGet*` calls.

### 6.7 Remove `SetObjectLighting()` / `UnsetObjectLighting()` (deferred to Phase 6)

**File:** `Starstrike\renderer.cpp` (lines 794-819)

Replace `glMaterialfv` / `glEnable(GL_LIGHTING)` / `glEnable(GL_LIGHT0)` calls with a constant buffer update that enables/disables lighting in the shader.

### 6.8 `glLineWidth` / `glPointSize` (36 call sites)

D3D11 **does not support line widths > 1.0px**. The codebase uses `glLineWidth` at 36 call sites (values 1.0–5.0) in UI borders, debug rendering, laser fences, and taskmanager icons, plus 1 `glPointSize(3.0f)`.

**Strategy:** Accept 1px lines initially (visual regression but functional). For Phase 10, thick lines can be emulated by expanding line primitives into screen-space quads in `ImRenderer::End()` (geometry expansion, similar to the quad→triangle conversion). Add a `SetLineWidth(float)` method to `ImRenderer` that stores the desired width; when width > 1.0 and the primitive is `PRIM_LINES` / `PRIM_LINE_STRIP` / `PRIM_LINE_LOOP`, convert to triangles in `End()`.

### 6.9 Save/Restore State Pattern (`text_renderer.cpp`)

The text renderer uses RAII classes (`SaveGLBlendFunc`, `SaveGLColour`, `SaveGLEnabled`, `SaveGLTex2DParamI`, `SaveGLFontDrawAttributes`) that **query GL state on construction and restore it on destruction** via `glGetIntegerv`/`glGetFloatv`. D3D11 has no state query API.

**Strategy:** Replace these RAII guards with explicit save/restore using `ImRenderer` accessors:
- `GetWorldMatrix()` / `GetProjectionMatrix()` already exist
- Track current blend/depth/raster state in `RenderStates` and provide `GetCurrentBlendState()` etc. ✅ implemented
- Or simply replace the RAII guards with explicit set/reset at each `DrawText*` call site

**Verification:** Build passes. `DebugTrace` confirms "RenderStates initialised (7 blend, 3 depth, 2 raster)". All state objects are pre-built and ready for use when game code migration begins in Phase 6. OpenGL continues to handle all rendering. The actual migration of `SetOpenGLState()`, `CheckOpenGLState()`, `SetObjectLighting()`, fog, and lighting calls will happen as part of Phase 6 (core 3D rendering migration) where they can be tested with visible output.

---

## 7. Phase 4 — Texture System Migration ✅ COMPLETED

### 7.1 New Files Created

| File | Project | Purpose |
|---|---|---|
| `NeuronClient\texture_manager.h` | NeuronClient | `TextureManager` class, `SamplerId` enum |
| `NeuronClient\texture_manager.cpp` | NeuronClient | D3D11 texture creation, SRV management, sampler states |

### 7.2 Implementation Details

- **`TextureManager`** class with `CreateTexture()`, `DestroyTexture()`, `DestroyAll()`, `GetSRV()`, `GetSampler()`.
- **Texture registry**: `std::vector<TextureEntry>` indexed by `int` handles (index 0 reserved as invalid). Free-list for ID reuse.
- **`CreateTexture(pixels, width, height, mipmapping)`**: Creates `ID3D11Texture2D` + `ID3D11ShaderResourceView`. For mipmapped textures, uses `D3D11_RESOURCE_MISC_GENERATE_MIPS` + `GenerateMips()`.
- **4 pre-built sampler states** (`SamplerId` enum):
  - `SAMPLER_LINEAR_CLAMP` — `MIN_MAG_MIP_LINEAR`, `CLAMP`
  - `SAMPLER_LINEAR_WRAP` — `MIN_MAG_MIP_LINEAR`, `WRAP`
  - `SAMPLER_NEAREST_CLAMP` — `MIN_MAG_MIP_POINT`, `CLAMP`
  - `SAMPLER_NEAREST_WRAP` — `MIN_MAG_MIP_POINT`, `WRAP`
- Global `g_textureManager` pointer, created/destroyed in `window_manager.cpp`.

### 7.3 `BitmapRGBA::ConvertToTexture()` Modified

**File:** `NeuronClient\bitmap.cpp`

Always creates the **OpenGL texture first** (since OpenGL still renders during the transition), then mirrors the pixel data into a D3D11 texture at the **same ID** via `g_textureManager->CreateTextureAt(glTexId, ...)`. Returns the OpenGL texture ID. This ensures:
- `glBindTexture(GL_TEXTURE_2D, id)` works for current OpenGL rendering
- `g_imRenderer->BindTexture(id)` will work for D3D11 rendering (when game code is migrated)

> **Note:** An earlier version used a D3D11-only early-return path that broke OpenGL rendering (textures appeared as white cubes). The dual-creation approach fixes this.

### 7.4 `ImRenderer` Extended

**File:** `NeuronClient\im_renderer.h` / `im_renderer.cpp`

- `BindTexture(int textureId)` — looks up SRV from `g_textureManager->GetSRV(id)`.
- `SetSampler(SamplerId id)` — sets a sampler state from `g_textureManager->GetSampler(id)`.

### 7.4.1 `TextureManager` Extended

**File:** `NeuronClient\texture_manager.h` / `texture_manager.cpp`

- `CreateTextureAt(int id, ...)` — creates a D3D11 texture stored at a **specific ID** (the OpenGL texture ID). Grows the internal vector as needed. Used during the transition period so both OpenGL and D3D11 textures share the same handle.

### 7.5 `Resource` Updated

**File:** `NeuronClient\resource.cpp`

- `DeleteTexture()` — always deletes via `glDeleteTextures` AND `g_textureManager->DestroyTexture()` (both sides cleaned up).
- `FlushOpenGlState()` — same dual cleanup: deletes GL textures and D3D11 textures for each entry.

### 7.6 Remaining Phase 4 Work (deferred to Phase 6–8)

The following will be done as part of the per-file migration in later phases:
- Replacing `glBindTexture(GL_TEXTURE_2D, id)` → `g_imRenderer->BindTexture(id)` at ~130 call sites
- Replacing `glTexParameteri` filter/wrap calls → `g_imRenderer->SetSampler(...)` at ~78 call sites
- Replacing `glEnable(GL_TEXTURE_2D)` / `glDisable(GL_TEXTURE_2D)` → `BindTexture`/`UnbindTexture`

**Verification:** Build passes. Textures are now created as D3D11 resources via `TextureManager` and are ready for use by `ImRenderer::BindTexture(int)` when game code migration begins.

---

## 8. Phase 5 — Matrix System Migration ✅ COMPLETED

### 8.1 Implementation Details

All D3D11 matrix paths are set **alongside** the existing OpenGL matrix calls, keeping OpenGL rendering functional while the `ImRenderer` matrices are prepared for future phases.

**Coordinate convention:** All `XMMatrix*RH` variants are used (Option A from the original plan) to preserve OpenGL's right-handed coordinate system and existing winding order. No vertex data changes required.

### 8.2 Files Modified

**`Starstrike\renderer.cpp`:**
- `SetupProjMatrixFor3D()` — After the OpenGL `gluPerspective`, calls `g_imRenderer->SetProjectionMatrix(XMMatrixPerspectiveFovRH(...))` with the same FOV, aspect, near, far parameters. FOV converted from degrees to radians.
- `SetupMatricesFor2D()` — After the OpenGL `gluOrtho2D`, calls `g_imRenderer->SetProjectionMatrix(XMMatrixOrthographicOffCenterRH(...))` and resets view/world to identity.
- `UpdateTotalMatrix()` — When `g_imRenderer` is available, computes `WVP = world * view * proj` from the stored ImRenderer matrices, transposes to column-major doubles (OpenGL layout), and stores in `m_totalMatrix[]`. Falls back to `glGetDoublev` otherwise.
- Added `#include "render_device.h"`, `"render_states.h"`, `"im_renderer.h"`.

**`Starstrike\camera.cpp`:**
- `SetupModelviewMatrix()` — After the OpenGL `gluLookAt`, calls `g_imRenderer->SetViewMatrix(XMMatrixLookAtRH(...))` and resets world matrix to identity. Uses the same eye/target/up values.
- `Get2DScreenPos()` — Kept using `gluProject` exclusively. An earlier D3D11 path was removed because (a) `gluProject` uses OpenGL's Y-up screen convention (Y=0 at bottom) while the D3D11 replacement used Y=0 at top, inverting mouse controls, and (b) the ImRenderer matrices may be stale when `Get2DScreenPos` is called during input processing (outside the render loop). Will be migrated in Phase 9 when OpenGL is fully removed.
- Added `#include "im_renderer.h"`.

**`NeuronClient\text_renderer.h`:**
- Added `#include <DirectXMath.h>`.
- Added `DirectX::XMMATRIX m_savedProjMatrix`, `m_savedViewMatrix`, `m_savedWorldMatrix` members for save/restore.

**`NeuronClient\text_renderer.cpp`:**
- `BeginText2D()` — Saves current `g_imRenderer` matrices, sets ortho projection matching the OpenGL `gluOrtho2D` call (with the -0.325 pixel offset), and resets view/world to identity.
- `EndText2D()` — Restores the saved matrices.
- Added `#include "im_renderer.h"`, `"render_states.h"`.

**`NeuronClient\im_renderer.h`:**
- Added `GetViewMatrix()` accessor (was missing; `GetProjectionMatrix()` and `GetWorldMatrix()` already existed).

### 8.3 Remaining OpenGL Matrix Calls (deferred to Phase 6–8)

The OpenGL `glMatrixMode`, `glLoadIdentity`, `glLoadMatrixd`, `gluPerspective`, `gluOrtho2D`, `gluLookAt`, `glGetDoublev` calls are **preserved** alongside the new D3D11 paths. They will be removed in Phase 9 when OpenGL is fully stripped.

The `glPushMatrix`/`glPopMatrix`/`glMultMatrixf`/`glTranslatef`/`glScalef`/`glRotatef` calls in game code (~30 sites) will be migrated to `g_imRenderer->PushMatrix()` etc. as part of the per-file migration in Phases 6–8.

**Verification:** Build passes. Both OpenGL and D3D11 matrix paths coexist. The `ImRenderer` now has correct projection, view, and world matrices set at the same points in the frame as the OpenGL matrix calls.

---

## 9. Phase 6 — Core 3D Rendering Migration ✅ COMPLETED

ImRenderer calls have been added **alongside** existing OpenGL calls in all Phase 6 files. OpenGL still does all visible rendering. The ImRenderer receives the same vertex data and state changes, preparing for the final switchover in Phase 9.

### 9.1 Shape Rendering ✅

**File:** `NeuronClient\shape.cpp`

- **Display lists removed:** `#define USE_DISPLAY_LISTS` commented out. Both `Shape::BuildDisplayList()` and `ShapeFragment::BuildDisplayList()` are now no-ops (D3D11 has no display list equivalent). All rendering goes through `RenderSlow()`.
- **`ShapeFragment::RenderSlow()`** — `glBegin(GL_TRIANGLES)` / `glNormal3fv` / `glColor4ub` / `glVertex3fv` / `glEnd()` duplicated with `g_imRenderer->Begin(PRIM_TRIANGLES)` etc.
- **`ShapeFragment::Render()`** — `glPushMatrix` / `glMultMatrixf` / `glPopMatrix` duplicated with `g_imRenderer->PushMatrix()` / `MultMatrixf()` / `PopMatrix()`. Display list path (`glCallList`) removed.
- **`Shape::Render()`** — Same matrix stack migration. Display list path removed.
- **`ShapeFragment::RenderMarkers()`** — `glDisable(GL_DEPTH_TEST)` / `glEnable(GL_DEPTH_TEST)` duplicated with `g_renderStates->SetDepthState()`. Commented-out debug GL code removed.
- Added `#include "render_device.h"`, `"im_renderer.h"`, `"render_states.h"`.

### 9.2 Sphere Rendering ✅

**File:** `NeuronClient\sphere_renderer.cpp` / `sphere_renderer.h`

- **Display list removed:** `m_displayListId` member removed from `Sphere`. Constructor no longer calls `glGenLists` / `glNewList` / `glEndList`. `Render()` calls `RenderLong()` directly instead of `glCallList`.
- **`Sphere::ConsiderTriangle()`** — `glBegin(GL_TRIANGLES)` / `glVertex3f` / `glEnd()` replaced with `g_imRenderer` calls.
- **`Sphere::Render()`** — `glPushMatrix` / `glTranslatef` / `glScalef` / `glPopMatrix` duplicated with `g_imRenderer->PushMatrix()` / `Translatef()` / `Scalef()` / `PopMatrix()`.
- Added `#include "im_renderer.h"`.

### 9.3 3D Sprite Rendering ✅

**File:** `NeuronClient\3d_sprite.cpp`

- **`Render3DSprite()`** — `glBegin(GL_QUADS)` / `glTexCoord2f` / `glVertex3fv` / `glEnd()` duplicated with `g_imRenderer` calls. Texture binding via `g_imRenderer->BindTexture(id)` / `UnbindTexture()`. Cull state via `g_renderStates`.
- Added `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"`.

### 9.4 Debug Rendering ✅

**File:** `NeuronClient\debug_render.cpp`

All functions migrated with dual GL + ImRenderer paths:
- **`RenderSquare2d()`** — 2D ortho projection via ImRenderer matrix save/restore + `XMMatrixOrthographicOffCenterRH`. Quad via `g_imRenderer->Begin(PRIM_QUADS)`.
- **`RenderCube()`** — Line loops and lines via `g_imRenderer->Begin(PRIM_LINE_LOOP)` / `Begin(PRIM_LINES)`.
- **`RenderSphereRings()`** — Three line loops via `g_imRenderer->Begin(PRIM_LINE_LOOP)`.
- **`RenderSphere()`** — Color set on both ImRenderer and GL.
- **`RenderVerticalCylinder()`** — Base/top/side line loops + line pairs via ImRenderer.
- **`RenderArrow()`** — Lines via `g_imRenderer->Begin(PRIM_LINES)`.
- Added `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"`.

### 9.5 Particles ✅

**File:** `Starstrike\particle_system.cpp`

- **`ParticleSystem::Render()`** — State setup duplicated: texture binding, blend state (`BLEND_ADDITIVE`), depth state (`DEPTH_ENABLED_READONLY`), raster state (`RASTER_CULL_NONE`), sampler (`SAMPLER_NEAREST_CLAMP`).
- **`Particle::Render()`** — Per-particle blend state switching (missile trail uses `BLEND_SUBTRACTIVE_COLOR`, others use `BLEND_ADDITIVE`). Quad rendering via `g_imRenderer->Begin(PRIM_QUADS)`.
- Added `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"`, `"texture_manager.h"`.

### 9.6 Explosions ✅

**File:** `Starstrike\explosion.cpp`

- **`Explosion::Render()`** — Triangle rendering duplicated with `g_imRenderer->Begin(PRIM_TRIANGLES)`.
- **`ExplosionManager::Render()`** — State setup duplicated: texture binding, cull/blend state.
- Added `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"`, `"texture_manager.h"`.

### 9.7 Clouds ✅

**File:** `Starstrike\clouds.cpp`

- **`Clouds::RenderQuad()`** — Quad grid rendering duplicated with `g_imRenderer->Begin(PRIM_QUADS)`.
- **`Clouds::RenderFlat()`** — State setup duplicated: texture binding (`SAMPLER_NEAREST_WRAP`), blend (`BLEND_ADDITIVE`), depth (`DEPTH_DISABLED`). Color set on both paths.
- **`Clouds::RenderBlobby()`** — Same pattern as `RenderFlat` with `SAMPLER_LINEAR_WRAP`.
- **`Clouds::RenderSky()`** — Sky grid quads duplicated with ImRenderer. State: `BLEND_ADDITIVE`, `DEPTH_ENABLED_READONLY`.
- Added `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"`, `"texture_manager.h"`.

### 9.8 Landscape Rendering — Includes Only (deferred)

**File:** `Starstrike\landscape_renderer.cpp`

Added D3D11 includes (`im_renderer.h`, `render_device.h`, `render_states.h`, `texture_manager.h`). The actual vertex array / VBO / display list rendering is **not migrated** in this phase because:
- Landscape uses `glVertexPointer` / `glNormalPointer` / `glColorPointer` + `glDrawArrays(GL_TRIANGLE_STRIP)` — this is NOT a `glBegin`/`glEnd` pattern and cannot be replaced with `ImRenderer::Begin()/End()`.
- The proper D3D11 approach is a static `ID3D11Buffer` vertex buffer, which will be created in Phase 10 (optimization).
- OpenGL vertex array rendering continues to work correctly.

### 9.9 Water Rendering — Includes Only (deferred)

**File:** `Starstrike\water.cpp`

Same situation as landscape — uses vertex arrays with `glVertexPointer` / `glColorPointer` + `glDrawArrays(GL_TRIANGLE_STRIP)`, plus ARB multitexture extensions. Added D3D11 includes only. Full migration deferred to Phase 10.

**Verification:** Build passes. All Phase 6 files have ImRenderer calls alongside GL calls. OpenGL continues to handle all visible rendering.

---

## 10. Phase 7 — 2D / UI Rendering Migration ✅ COMPLETED

ImRenderer calls have been added **alongside** existing OpenGL calls in all Phase 7 files. OpenGL still does all visible rendering. The ImRenderer receives the same vertex data and state changes, preparing for the final switchover in Phase 9.

### 10.1 Text Renderer ✅

**File:** `NeuronClient\text_renderer.cpp`

- `DrawText2DSimple()` — `glBegin(GL_QUADS)` / `glTexCoord2f` / `glVertex2f` / `glEnd()` duplicated with `g_imRenderer->Begin(PRIM_QUADS)` etc.
- `DrawText2DUp()` — Same quad duplication.
- `DrawText2DDown()` — Same quad duplication.
- `DrawText3DSimple()` — Same quad duplication for 3D text.
- `DrawText3D()` (with front/up vectors) — Same quad duplication.
- `BeginText2D()` / `EndText2D()` — Matrix save/restore already done in Phase 5.
- `BuildOpenGlState()` — Texture binding via `g_imRenderer->BindTexture()` added alongside `glBindTexture`.
- All `glColor4f` calls duplicated with `g_imRenderer->Color4f`.

### 10.2 Taskmanager Interface Icons (Heaviest File) ✅

**File:** `Starstrike\taskmanager_interface_icons.cpp` — 649 GL calls → 636 D3D11 calls

Bulk migration via PowerShell scripts:
- All 46 `glBegin`/`glEnd` blocks (quads, triangles, lines, line loops) duplicated with `g_imRenderer->Begin`/`End` equivalents.
- All standalone `glColor4f`/`glColor4ub`/`glColor3ub` calls duplicated with `g_imRenderer->Color*`.
- All `glBlendFunc` calls paired with `g_renderStates->SetBlendState()`.
- All `glBindTexture` calls paired with `g_imRenderer->BindTexture()`.
- All `glDisable(GL_TEXTURE_2D)` calls paired with `g_imRenderer->UnbindTexture()`.
- All `glDepthMask` calls paired with `g_renderStates->SetDepthState()`.
- `SetupRenderMatrices()` — added `g_imRenderer->SetProjectionMatrix(XMMatrixOrthographicOffCenterRH(...))` alongside `gluOrtho2D`.
- `Render()` — Blend/cull state setup via `g_renderStates`.
- `RenderIcon()` — Full dual-path for textured icon rendering with shadow (subtractive color blend) and foreground (additive blend).
- `RenderScreenZones()` — Highlight quad and textured selection brackets.
- 4 broken `if`/`else` patterns (braceless if with two statements) fixed with explicit braces.

### 10.3 All UI Windows (GameLogic) ✅

**Files migrated:**
- `darwinia_window.cpp` — `DarwiniaButton::Render`, `DarwiniaWindow::Render`, `BorderlessButton::Render`, `CloseButton::Render`, `InvertedBox::Render` (consolidated 4 GL_LINES into 1 PRIM_LINES + color change), `LabelButton::Render`.
- `mainmenus.cpp` — `ResetLocationWindow::Render`, `AboutDarwiniaWindow::Render`, `SkipPrologueWindow::Render`, `PlayPrologueWindow::Render`. Textured quads with additive blend.
- `scrollbar.cpp` — Background quad, border line loop, bar quad.
- `drop_down_menu.cpp` — Triangle indicator.
- `input_field.cpp` — Edit area quad, border lines, `ColourWidget::Render`, `ColourWindow::Render`.
- `profilewindow.cpp` — Performance bar quad and color calls.
- `prefs_graphics_window.cpp`, `prefs_screen_window.cpp`, `prefs_sound_window.cpp`, `prefs_other_window.cpp`, `network_window.cpp`, `userprofile_window.cpp` — Color calls only.

### 10.4 Game Cursor ✅

**File:** `Starstrike\gamecursor.cpp`

- Arrow rendering functions, `RenderSelectionArrow`, `MouseCursor::Render`, `MouseCursor::Render3D` — all duplicated with `g_imRenderer` calls.

### 10.5 Start Sequence ✅

**File:** `Starstrike\startsequence.cpp`

- Full `Render()` function duplicated with `g_imRenderer` calls.

### 10.6 Game Menu ✅

**File:** `Starstrike\game_menu.cpp`

- Color calls duplicated. The only `glBegin` block is inside a `/* */` comment (disabled code).

### 10.7 Includes Added

All Phase 7 files have `#include "im_renderer.h"`, `"render_device.h"`, `"render_states.h"` (and `"texture_manager.h"` where sampler state IDs are referenced).

**Verification:** Build passes. All Phase 7 files have ImRenderer calls alongside GL calls. OpenGL continues to handle all visible rendering.

---

## 11. Phase 8 — Entity / GameLogic 3D Rendering Migration ✅ COMPLETED

ImRenderer calls have been added **alongside** existing OpenGL calls in all Phase 8 files using a bulk migration PowerShell script (`migrate_gl_blocks.ps1`). The script handles `glBegin`/`glEnd` blocks, standalone `glColor*`, `glBlendFunc`, `glBindTexture`, `glDisable(GL_TEXTURE_2D)`, `glDepthMask`, `glEnable/glDisable(GL_BLEND/GL_DEPTH_TEST/GL_CULL_FACE)`, and `glPushMatrix`/`glPopMatrix`/`glTranslatef`/`glScalef`/`glRotatef`/`glMultMatrixf` calls.

**Total coverage:** 82 files with 2,514 ImRenderer calls and 666 RenderStates calls.

### 11.1 Entity Rendering Files ✅

All of these files have dual GL + ImRenderer paths:

- `weapons.cpp` (49 insertions), `darwinian.cpp` (37), `officer.cpp` (25), `engineer.cpp` (13), `insertion_squad.cpp` (5)
- `spider.cpp` (5), `souldestroyer.cpp` (23), `tripod.cpp` (1), `virii.cpp` (16), `lasertrooper.cpp` (10), `lander.cpp` (5)
- `building.cpp` (22), `generator.cpp` (37), `laserfence.cpp` (17), `factory.cpp` (1), `bridge.cpp` (5)
- `incubator.cpp` (11), `constructionyard.cpp` (22), `controltower.cpp` (23), `gunturret.cpp` (25)
- `radardish.cpp` (18), `researchitem.cpp` (14), `safearea.cpp` (2), `switch.cpp` (20)
- `teleport.cpp` (12), `trunkport.cpp` (19), `triffid.cpp` (12), `tree.cpp` (17)
- `spirit.cpp` (4), `spiritreceiver.cpp` (41), `spiritstore.cpp` (11), `sporegenerator.cpp` (7)
- `rocket.cpp` (38), `mine.cpp` (22), `egg.cpp` (17), `flag.cpp` (11), `goddish.cpp` (12), `spawnpoint.cpp` (28)
- `snow.cpp` (1), `spam.cpp` (21), `feedingtube.cpp` (18), `entity.cpp` (11), `blueprintstore.cpp` (42)

### 11.2 Location & World Rendering ✅

- `location.cpp` (21 insertions) — Fog/light setup GL calls preserved (will be constant buffer updates in Phase 9).
- `global_world.cpp` (61) — Full `Render()` function with textured quads, line loops, blend/depth state.
- `global_internet.cpp` (28) — Planet/node rendering with textured quads.
- `entity_grid.cpp` (8), `obstruction_grid.cpp` (8) — Debug grid rendering.
- `team.cpp` (21) — Virii batch rendering (merged `Begin`/`End` wrapping shared loop).
- `user_input.cpp` (8), `location_input.cpp` (1), `routing_system.cpp` (2).
- `3d_sierpinski_gasket.cpp` (12) — Fractal rendering.

### 11.3 Additional Files ✅

- `renderer.cpp` (63 insertions) — All `glBegin`/`glEnd` blocks in `RenderFlatTexture`, `DrawQuad`, and screen-space rendering functions.
- `landscape.cpp` (2) — Debug line rendering.
- `render_utils.cpp` (2) — Utility quad rendering.

### 11.4 Manual Fixes Required

Several files needed manual correction after the bulk script:

- **`virii.cpp`** — `glBegin(GL_QUADS)` wraps rendering loops with shared state (`colour.a`, `texYpos`, `prevPos`). Merged into single loops with dual GL + ImRenderer output to avoid double mutation of shared state.
- **`team.cpp`** — `glBegin(GL_QUADS)` wraps the Virii entity loop. Merged `g_imRenderer->Begin`/`glBegin` before the loop and `g_imRenderer->End`/`glEnd` after.
- **`weapons.cpp`** — `GL_TRIANGLE_FAN` block with loop variable `angle`. Merged into single loop.
- **`blueprintstore.cpp`** — Two quad blocks with `float infected` variable declarations inside `glBegin`/`glEnd`. Merged into interleaved dual output.
- **`spiritreceiver.cpp`** — Broken `if`/`else` (braceless if with two statements). Fixed with explicit braces.

### 11.5 Remaining Unmigrated Files

- `game_menu.cpp` — Only `glBegin` is inside a `/* */` comment (dead code). No action needed.
- `water.cpp` — Uses `glVertexPointer`/`glDrawArrays(GL_TRIANGLE_STRIP)` vertex array paths. Deferred to Phase 10 (static D3D11 vertex buffers).

### 11.6 Includes Added

All 50+ files received `#include "im_renderer.h"` and conditionally `#include "render_device.h"` / `#include "render_states.h"` based on usage.

**Verification:** Build passes. All Phase 8 files have ImRenderer calls alongside GL calls. OpenGL continues to handle all visible rendering.

---

## 12. Phase 9 — OGL Extension & Utility Cleanup ✅ COMPLETED

All OpenGL headers, libraries, API calls, extension systems, and GL types have been removed from the entire codebase. The project now compiles and links exclusively against D3D11.

### 12.1 OpenGL Extension System Removed ✅

**Files deleted:**
- `NeuronClient\ogl_extensions.h` — ARB extension typedefs and declarations
- `NeuronClient\ogl_extensions.cpp` — Extension loading via `wglGetProcAddress`

**References removed:**
- `#include "ogl_extensions.h"` from `renderer.cpp`, `landscape_renderer.cpp`, `water.cpp`, `laserfence.cpp`, `feedingtube.cpp`, `radardish.cpp`, `render_utils.cpp` (7 files)
- `InitialiseOGLExtensions()` call removed from `Renderer::Initialise()`

### 12.2 OpenGL Includes Replaced in PCH ✅

**File:** `NeuronClient\NeuronClient.h`

Replaced:
```cpp
#include <GL/gl.h>
#include <GL/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
```
With:
```cpp
#include <d3d11.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
```

### 12.3 OpenGL Context Removed ✅

**File:** `NeuronClient\window_manager.cpp`
- `EnableOpenGL()` — WGL pixel format / context creation removed; function is now an empty stub returning `true`.
- `DisableOpenGL()` — `wglDeleteContext` / `ReleaseDC` removed; function is now an empty stub.
- `DestroyWin()` — `DisableOpenGL()` call removed.
- `Flip()` — `SwapBuffers(m_win32Specific->m_hDC)` replaced with `g_renderDevice->EndFrame()`.

**File:** `NeuronClient\window_manager_win32.h`
- Removed `HDC m_hDC` and `HGLRC m_hRC` members (only `HWND m_hWnd` remains).

### 12.4 Bulk GL Call Removal ✅

Used PowerShell line-filter scripts to strip all remaining `gl*` / `glu*` / `ggl*ARB` calls across ~37 GameLogic and Starstrike files (154+ lines removed). Categories removed:

| Pattern | Sites | Notes |
|---|---|---|
| `glBegin` / `glEnd` | ~200 | Orphaned OpenGL duplicates of ImRenderer paths |
| `glVertex*` / `glNormal*` / `glTexCoord*` / `glColor*` | ~400 | Vertex attribute calls |
| `glBindTexture` / `glEnable(GL_TEXTURE_2D)` / `glDisable(GL_TEXTURE_2D)` | ~130 | Texture state |
| `glBlendFunc` / `glDepthMask` / `glEnable`/`glDisable` (state) | ~80 | Render state |
| `glPushMatrix` / `glPopMatrix` / `glTranslatef` / `glScalef` / `glRotatef` / `glMultMatrixf` | ~30 | Matrix stack |
| `glMatrixMode` / `glLoadIdentity` / `gluPerspective` / `gluOrtho2D` / `gluLookAt` | ~15 | Matrix setup |
| `glLineWidth` / `glPointSize` / `glTexParameteri` | ~40 | Misc state |
| `gglActiveTextureARB` / `gglMultiTexCoord2fARB` | ~20 | ARB multitexture |
| `glMaterialfv` / `glLightfv` / `glFog*` / `glAlphaFunc` / `glClearColor` / `glClear` | ~25 | Fixed-function pipeline |

### 12.5 GL Type Replacements ✅

All OpenGL type aliases replaced with C++ native types:
- `GLfloat` → `float`, `GLint` → `int`, `GLenum` → `int`, `GLdouble` → `double`, `GLubyte` → `unsigned char`
- Files affected: `location.cpp`, `global_world.cpp`

### 12.6 Infrastructure Functions Migrated ✅

**File:** `Starstrike\renderer.cpp`
- `CheckOpenGLState()` — Body replaced with no-op comment (was already `return;` at top with dead GL asserts below).
- `SetOpenGLState()` — All `gl*` calls replaced with `g_renderStates->Set*State()` and `g_imRenderer->UnbindTexture()`.
- `SetObjectLighting()` / `UnsetObjectLighting()` — GL material/light calls removed; bodies are stubs (TODO Phase 10: lighting via constant buffer).
- `BuildOpenGlState()` — GL state calls removed; function retained as stub.
- `GetGLStateInt()` / `GetGLStateFloat()` — GL query calls removed; return `0` / `0.0f`.

**File:** `Starstrike\renderer.h`
- `#define CHECK_OPENGL_STATE()` macro removed.

### 12.7 Remaining Subsystems Updated ✅

**File:** `NeuronClient\resource.cpp`
- `CreateDisplayList()` — `glGenLists` replaced with static counter returning dummy IDs.

**File:** `NeuronClient\text_renderer.cpp`
- `BeginText2D()` — `GLint` variables replaced; viewport dimensions sourced from `g_renderDevice->GetBackBufferWidth/Height()` instead of `glGetIntegerv(GL_VIEWPORT)`.
- Empty `if/else` blocks (orphaned from `glBlendFunc` removal) cleaned up.

**File:** `NeuronClient\debug_render.cpp` / `debug_render.h`
- `PrintMatrix()` — `GLenum` parameter changed to `int`; `GLdouble` matrix query removed; function is now a stub.
- `PrintMatrices()` — Label changed from "OpenGL" to "D3D11".

**File:** `NeuronClient\render_utils.cpp`
- `gglMultiTexCoord2fARB` calls replaced with single-texture `g_imRenderer->TexCoord2f`; orphaned GL duplicate loop removed.

**File:** `NeuronClient\debug_fog_logging.cpp`
- All `glGetFloatv(GL_FOG_*)` calls removed.

**File:** `GameLogic\building.cpp`, `GameLogic\generator.cpp`
- Inline `if(...) glColor4f(...)` patterns replaced with `g_imRenderer->Color4f()`.

**File:** `Starstrike\landscape_renderer.cpp`
- Extra closing brace from removed `switch` block fixed.
- `RenderMainSlow()` / `RenderOverlaySlow()` / `BuildOpenGlState()` are stubs (deferred to Phase 10 static VBs).

### 12.8 Landscape & Water (deferred to Phase 10)

These two systems used OpenGL vertex array paths (`glVertexPointer`/`glDrawArrays`) which cannot be emulated by `ImRenderer::Begin()/End()`. Their GL calls were stripped in this phase, leaving stub render functions. Proper D3D11 static vertex buffer rendering will be implemented in Phase 10.

**Verification:** Clean build passes. Zero `gl*`/`glu*`/`wgl*`/`GL_*`/`GLfloat`/`GLint`/`GLenum`/`HGLRC` references remain in any compiled source file. `opengl32.lib` and `glu32.lib` are no longer linked. `d3d11.lib` and `dxgi.lib` are the only graphics libraries.

---

## 13. Phase 10 — Polish & Optimisation ✅ PARTIALLY COMPLETED

### 13.1 Batch Optimisation (deferred)
The `ImRenderer` immediate-mode emulation works but is inherently slow (one draw call per `Begin`/`End` pair). Future optimisation:

- **Merge batches:** Consecutive `Begin`/`End` pairs with the same state (same texture, same blend mode) can be merged into a single draw call.
- **Static vertex buffers:** For geometry that doesn't change (shapes), create static `ID3D11Buffer` objects uploaded once.

### 13.2 Landscape — Static Vertex Buffer ✅

**File:** `Starstrike\landscape_renderer.h`

- Removed old `RenderMode` enum (`RenderModeVertexArray`, `RenderModeDisplayList`, `RenderModeVertexBufferObject`), `m_renderMode`, `unsigned int m_vertexBuffer`, static offset members (`m_posOffset`, `m_normOffset`, `m_colOffset`, `m_uvOffset`), and `Initialise()`.
- Added `ID3D11Buffer* m_d3dVertexBuffer`, `int m_d3dVertexCount`, `void CreateD3DVertexBuffer()`.

**File:** `Starstrike\landscape_renderer.cpp`

- **`CreateD3DVertexBuffer()`** — Converts `m_verts` (array of `LandVertex`) to ImRenderer-compatible vertex format (48 bytes/vert: `XMFLOAT3 pos`, `XMFLOAT4 color`, `XMFLOAT2 texcoord`, `XMFLOAT3 normal`), creates a `D3D11_USAGE_DEFAULT` static buffer via `CreateBuffer()`.
- **`BuildRenderState()`** — Calls `CreateD3DVertexBuffer()` after building vertex/normal/colour/UV arrays.
- **`RenderMainSlow()`** — Binds the static VB, uploads WVP via ImRenderer's constant buffer, sets ImRenderer's colored pixel shader, and draws each triangle strip via `ctx->Draw()`. Handles `m_negativeRenderer` blend mode.
- **`RenderOverlaySlow()`** — Same VB with additive blend (`BLEND_ADDITIVE`), depth readonly, and `textures/triangleOutline.bmp` texture via ImRenderer's textured pixel shader. Sampler set to `SAMPLER_LINEAR_WRAP`.
- **`Render()`** — Simplified to call `RenderMainSlow()` / `RenderOverlaySlow()` directly (no more display list lookup or render mode switch).
- **`~LandscapeRenderer()`** — Releases `m_d3dVertexBuffer`.
- **`ImRenderer` extended** with accessors for direct rendering: `GetConstantBuffer()`, `GetInputLayout()`, `GetVertexShader()`, `GetColoredPixelShader()`, `GetTexturedPixelShader()`.

### 13.2.1 Water — Flat + Dynamic Rendering ✅

**File:** `Starstrike\water.cpp`

- **`RenderFlatWaterTiles()`** — Renders textured quads via `g_imRenderer->Begin(PRIM_QUADS)` with scrolling UV coordinates (single texture; original multitexture lightmap deferred to future work).
- **`RenderFlatWater()`** — Sets cull/depth/blend states, binds water colour texture with `SAMPLER_LINEAR_WRAP`, calls `RenderFlatWaterTiles()`, restores state.
- **`RenderDynamicWater()`** — Renders animated wave triangle strips per-frame via `g_imRenderer->Begin(PRIM_TRIANGLE_STRIP)` with per-vertex colors and normals from `m_renderVerts`. Each `WaterTriangleStrip` is a separate `Begin`/`End` pair.

### 13.3 Lighting via Constant Buffer ✅

`SetObjectLighting()` / `UnsetObjectLighting()` in `renderer.cpp` now call `g_imRenderer->SetLightingEnabled(true/false)`. `SetupLights()` in `location.cpp` and `global_world.cpp` now call `g_imRenderer->SetLightParams()` to upload light direction, color, and ambient to the expanded `CBPerDraw` constant buffer.

**Shader changes:**
- `im_defaultVS.hlsl` — Added `gWorld` matrix to constant buffer. Outputs world-space position (`posWS`) and world-transformed normal (`normalWS`) for per-pixel lighting and fog.
- `im_coloredPS.hlsl` / `im_texturedPS.hlsl` — Added N·L diffuse lighting: `color.rgb *= ambient + sum(max(dot(N, lightDir), 0) * lightColor)`. Up to 2 directional lights supported. Lighting is gated by `gLightingEnabled` flag.

**`ImRenderer` additions:**
- `SetLightingEnabled(bool)` — toggles lighting in the pixel shader
- `SetLightParams(numLights, dirs[], colors[], ambient)` — stores up to 2 directional light directions/colors and ambient color
- `SetCameraPos(x, y, z)` — stores camera world position for fog distance calculation
- `UpdateConstantBuffer()` — public method that uploads the full `CBPerDraw` (matrices + lighting + fog + alpha clip) for external renderers (landscape, water)

**Constant buffer now also bound to PS stage** via `PSSetConstantBuffers(0, 1, &cb)` in both `Flush()` and landscape/water direct rendering.

### 13.4 Fog via Constant Buffer ✅

`SetupFog()` in `location.cpp` now calls `g_imRenderer->SetFogParams(1000.0f, 4000.0f, r, g, b)` with the background color. `SetupFog()` in `global_world.cpp` calls `g_imRenderer->SetFogParams(0.0f, 19000.0f, 0.1f, 0.1f, 0.1f)`. Fog is enabled/disabled per render section via `g_imRenderer->SetFogEnabled(true/false)` in `Location::RenderBuildings()` and `Location::RenderBuildingAlphas()`, matching the original `glEnable(GL_FOG)` / `glDisable(GL_FOG)` pattern.

**Pixel shader:** Linear fog computed as `saturate((gFogEnd - dist) / (gFogEnd - gFogStart))` where `dist = length(posWS - gCameraPos)`. Result: `lerp(gFogColor, color, fogFactor)`. Gated by `gFogEnabled` flag.

**`ImRenderer` additions:**
- `SetFogEnabled(bool)` — toggles fog in the pixel shader
- `SetFogParams(start, end, r, g, b)` — stores fog range and color

### 13.5 Alpha Test via Pixel Shader ✅

Both pixel shaders now include `if (color.a <= gAlphaClipThreshold) discard;`. Default threshold is `-1.0` (disabled). Call `g_imRenderer->SetAlphaClipThreshold(0.01f)` to mimic `glAlphaFunc(GL_GREATER, 0.01f)`.

**`ImRenderer` addition:** `SetAlphaClipThreshold(float)` — sets the clip threshold in the constant buffer.

### 13.7

**File:** `NeuronClient\system_info.cpp`

Changed `m_directXVersion = 9;` → `m_directXVersion = 11;`.

### 13.8 Remove Legacy Naming ✅

All OpenGL-named functions have been renamed to neutral names:

| Old Name | New Name | Files Affected |
|---|---|---|
| `SetOpenGLState()` | `SetDefaultRenderState()` | `renderer.h`, `renderer.cpp`, `main.cpp` |
| `CheckOpenGLState()` | **removed** (was no-op) | `renderer.h`, `renderer.cpp`, `water.cpp` |
| `BuildOpenGlState()` | `BuildRenderState()` | `renderer.h/cpp`, `text_renderer.h/cpp`, `landscape_renderer.h/cpp`, `landscape.h/cpp`, `water.h/cpp`, `resource.cpp` |
| `FlushOpenGlState()` | `FlushRenderState()` | `resource.h/cpp`, `location.h/cpp`, `cheat_window.cpp`, `mainmenus.cpp`, `prefs_screen_window.cpp` |
| `RegenerateOpenGlState()` | `RegenerateRenderState()` | `resource.h/cpp`, `location.h/cpp`, `cheat_window.cpp`, `mainmenus.cpp`, `prefs_screen_window.cpp` |
| `EnableOpenGL()` | **removed** (was empty stub) | `window_manager.h/cpp` |
| `DisableOpenGL()` | **removed** (was empty stub) | `window_manager.h/cpp` |
| `GetGLStateInt()` | **removed** (no callers) | `renderer.h/cpp` |
| `GetGLStateFloat()` | **removed** (no callers) | `renderer.h/cpp` |

Zero OpenGL-named function references remain in any compiled source file.

### 13.9 `Camera::Get2DScreenPos()` Migration ✅

`Camera::Get2DScreenPos()` was already migrated to use `g_imRenderer` matrices with manual viewport projection (D3D11 convention: Y=0 at top). However, 10 call sites in `camera.cpp` still applied a legacy `screenH - Y` flip (needed when `gluProject` returned Y=0 at bottom). These flips inverted the Y mouse delta, causing inverted camera controls.

**File:** `Starstrike\camera.cpp`
- Removed 10 `oldMouseY = screenH - oldMouseY` / `newMouseY = screenH - newMouseY` lines across 5 camera mode functions: `AdvanceSphereWorldMode()`, `AdvanceSphereWorldIntroMode()`, `AdvanceNormalMode()`/`AdvanceFreeMovement()`, `AdvanceEntityTrackMode()`, `AdvanceTurretAimMode()`.
- All other callers (`gamecursor.cpp`, `location.cpp`, `global_world.cpp`, `taskmanager_interface_icons.cpp`) already used the screen coordinates directly without Y-flipping.

**Bug fix:** `Starstrike\gamecursor.cpp` and `Starstrike\taskmanager_interface_icons.cpp` — Fixed incomplete `LegacyVector3 toCam` declarations (missing `= g_app->m_camera->GetPos() - _pos;` initializer and semicolon).

### 13.10 `ConvertToOpenGLFormat()` Rename ✅

`Matrix33::ConvertToOpenGLFormat()` and `Matrix34::ConvertToOpenGLFormat()` renamed to `ConvertToColumnMajor()`. The static member `m_openGLFormat[16]` renamed to `m_columnMajor[16]` in both classes. All 3 call sites updated (`shape.cpp` ×2, `tree.cpp` ×1).

---

## 14. File-Level Change Summary

### New Files (NeuronClient)
| File | Purpose |
|---|---|
| `render_device.h` | D3D11 device wrapper |
| `render_device.cpp` | D3D11 device init, swap chain, present |
| `im_renderer.h` | Immediate-mode emulation API + matrix stack |
| `im_renderer.cpp` | Vertex batching, pre-compiled shader loading, draw, matrix stack |
| `Shaders\im_defaultVS.hlsl` | Default vertex shader (FxCompile → `CompiledShaders\im_defaultVS.h`) |
| `Shaders\im_coloredPS.hlsl` | Colored pixel shader (FxCompile → `CompiledShaders\im_coloredPS.h`) |
| `Shaders\im_texturedPS.hlsl` | Textured pixel shader (FxCompile → `CompiledShaders\im_texturedPS.h`) |
| `render_states.h` | Pre-built blend/depth/raster state cache |
| `render_states.cpp` | State object creation |
| `texture_manager.h` | D3D11 texture registry + sampler states |
| `texture_manager.cpp` | Texture creation, SRV management, sampler creation |

### Deleted Files
| File | Phase | Reason |
|---|---|---|
| `NeuronClient\opengl_directx_internals.h` | 0 ✅ | Empty / unused |
| `NeuronClient\ogl_extensions.h` | 9 ✅ | OpenGL extension definitions |
| `NeuronClient\ogl_extensions.cpp` | 9 ✅ | OpenGL extension loading |

### Modified Files (Infrastructure — Phase 1-5)
| File | Changes |
|---|---|
| `NeuronClient\NeuronClient.h` | Replace GL includes with D3D11 includes |
| `NeuronClient\window_manager.h` | Remove `EnableOpenGL`/`DisableOpenGL` |
| `NeuronClient\window_manager.cpp` | Replace WGL context with `RenderDevice` calls |
| `NeuronClient\bitmap.cpp` | Replace `ConvertToTexture()` with D3D11 texture creation |
| `NeuronClient\resource.cpp` | Remove display list code, update texture handling |
| `NeuronClient\resource.h` | Remove `m_displayLists` |
| `NeuronClient\text_renderer.cpp` | Replace all GL calls with `ImRenderer` |
| `NeuronClient\shape.cpp` | Replace GL immediate mode, remove display lists |
| `NeuronClient\sphere_renderer.cpp` | Replace GL calls |
| `NeuronClient\debug_render.cpp` | Replace GL calls |
| `NeuronClient\debug_render.h` | Remove `GLenum` parameter |
| `NeuronClient\debug_fog_logging.cpp` | Replace GL fog queries |
| `NeuronClient\render_utils.cpp` | Replace GL calls |
| `NeuronClient\3d_sprite.cpp` | Replace GL calls |
| `NeuronClient\shader.h` | D3D9 class removed (Phase 0 ✅); add D3D11 shader class |
| `NeuronClient\shader.cpp` | D3D9 code removed (Phase 0 ✅); rewrite for D3D11 |
| `NeuronClient\texture.h` | D3D9 class removed (Phase 0 ✅); add D3D11 texture class |
| `NeuronClient\system_info.cpp` | Update `m_directXVersion` |
| `Starstrike\renderer.h` | Remove GL macros and GL helper methods |
| `Starstrike\renderer.cpp` | Complete GL → D3D11 migration |
| `Starstrike\camera.cpp` | Replace `gluLookAt` with `XMMatrixLookAtRH` |
| `Starstrike\landscape_renderer.h` | D3D9 modes removed (Phase 0 ✅); remove remaining GL modes |
| `Starstrike\landscape_renderer.cpp` | D3D9 blocks removed (Phase 0 ✅); replace GL with D3D11 static VB |

### Modified Files (Game Content — Phase 6-8)
All ~65 GameLogic `.cpp` files and ~15 Starstrike `.cpp` files listed in Sections 10 and 11. Each requires mechanical replacement of `gl*` calls with `g_imRenderer->*` calls and render state calls with `g_renderDevice->*` / `g_renderStates->*` calls.

---

## 15. Migration Order Checklist

- [x] **Phase 0:** Remove dead D3D9 code, audit includes
- [x] **Phase 1:** Create `RenderDevice`, init D3D11, swap chain, integrate into `WindowManager`
- [x] **Phase 2:** Create `ImRenderer` with built-in shaders
- [x] **Phase 3:** Create `RenderStates`, pre-build blend/depth/raster state objects
- [x] **Phase 4:** Migrate texture system (`ConvertToTexture`, `TextureManager`, sampler states)
- [x] **Phase 5:** Migrate matrix system (projection, modelview, `gluPerspective`, `gluLookAt`)
- [x] **Phase 6:** Migrate core 3D rendering (shapes, landscape, spheres, particles, water, clouds)
- [x] **Phase 7:** Migrate 2D/UI rendering (text, HUD, menus, cursors, taskmanager icons)
- [x] **Phase 8:** Migrate entity/GameLogic rendering (~65 files)
- [x] **Phase 9:** Remove all OpenGL code, headers, libs
- [x] **Phase 10:** Landscape static VB, water rendering, legacy naming cleanup, `m_directXVersion = 11`, `Get2DScreenPos` Y-flip fix
- [ ] **Phase 10 remaining:** Batch optimisation

---

## 16. Build & Link Configuration

### NuGet / Libraries
No NuGet packages needed — D3D11 ships with the Windows SDK already referenced by the project.

### Linker Additions
In `NeuronClient.h` (replacing the GL pragmas):
```cpp
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
```

> **Note:** `d3dcompiler.lib` is NOT needed — shaders are pre-compiled at build time via FXCompile.

### Linker Removals
```cpp
// Remove:
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
```

### Project File Changes
- Add new `.cpp` / `.h` files to `NeuronClient.vcxproj` and `.filters`.
- Add `.hlsl` files with `FXCompile` build action (see below).
- Remove `ogl_extensions.cpp`, `ogl_extensions.h`, `opengl_directx_internals.h` from project.

### FxCompile Configuration for HLSL Shaders

The three `.hlsl` files are added to `NeuronClient.vcxproj` as `FxCompile` items with **no per-configuration conditions** (properties apply to all configurations/platforms):

| File | Shader Type | Shader Model | Entry Point | Header File Output | Variable Name |
|---|---|---|---|---|---|
| `Shaders\im_defaultVS.hlsl` | Vertex | 5.0 | `main` | `$(ProjectDir)CompiledShaders\%(Filename).h` | `g_pim_defaultVS` |
| `Shaders\im_coloredPS.hlsl` | Pixel | 5.0 | `main` | `$(ProjectDir)CompiledShaders\%(Filename).h` | `g_pim_coloredPS` |
| `Shaders\im_texturedPS.hlsl` | Pixel | 5.0 | `main` | `$(ProjectDir)CompiledShaders\%(Filename).h` | `g_pim_texturedPS` |

The generated `.h` files (e.g. `const BYTE g_pim_defaultVS[] = { ... };`) are included by `im_renderer.cpp` via `#include "CompiledShaders/im_defaultVS.h"`.

> **Important:** FxCompile properties must **not** be conditioned on a single configuration — they must apply to both Debug and Release. The `.vcxproj` uses unconditional `<VariableName>`, `<HeaderFileOutput>`, `<ShaderType>`, and `<ShaderModel>` elements.

---

## 17. Risk Register

| Risk | Severity | Mitigation |
|---|---|---|
| Coordinate handedness mismatch (RH→LH) | ✅ Resolved | `XMMatrix*RH` variants used throughout — existing vertex data and winding preserved |
| Texture coordinate origin difference (GL bottom-left vs D3D top-left) | 🟠 Moderate | Flip V coordinate in `ConvertToTexture()` or in HLSL |
| `GL_QUADS` has no D3D primitive | ✅ Resolved | `ImRenderer` converts quads to triangle pairs |
| `glAlphaFunc` / alpha test has no D3D11 fixed function | ✅ Resolved | `if (color.a <= gAlphaClipThreshold) discard;` in both pixel shaders; `-1.0` default disables |
| Display lists have no D3D11 equivalent | ✅ Resolved | Shape and sphere display lists removed; `BuildDisplayList()` no-op; always uses `RenderSlow()` path |
| Performance regression from immediate-mode emulation | 🟠 Moderate | Landscape uses static VB; water dynamic uses `ImRenderer`; shapes and batch merge deferred |
| GL state queries (`glGet*`) used in debug asserts | ✅ Resolved | `CheckOpenGLState()` removed; `GetGLStateInt/Float()` removed; viewport from `g_renderDevice->GetBackBufferWidth/Height()` |
| `gluBuild2DMipmaps` has no D3D11 equivalent | ✅ Resolved | `GenerateMips()` via `TextureManager::CreateTexture()` |
| Large number of files to touch (~95) | ✅ Resolved | PowerShell bulk scripts stripped 154+ GL lines across 37 files in Phase 9 |
| `glLineWidth` > 1.0 unsupported in D3D11 (36 sites) | ✅ Resolved | 1px lines accepted; thick-line emulation not needed |
| Save/restore state RAII pattern in `text_renderer.cpp` | ✅ Resolved | RAII guards removed; explicit save/restore via `ImRenderer` matrix accessors and `RenderStates::GetCurrent*()` |
| Vertex array paths (landscape, water) | ✅ Resolved | Landscape uses static `ID3D11Buffer`; water uses `ImRenderer::Begin(PRIM_TRIANGLE_STRIP)` per frame |
| `Matrix34::ConvertToOpenGLFormat()` column-major vs `XMMATRIX` row-major | ✅ Resolved | `ImRenderer::MultMatrixf()` transposes on load; function rename deferred (cosmetic) |
| `GL_CLAMP` vs `D3D11_TEXTURE_ADDRESS_CLAMP` (border pixel bleed) | ✅ Resolved | 4 sampler states pre-created; accept minor difference |
| `Camera::SetupModelviewMatrix()` scales vectors by `magOfPos` before `gluLookAt` | ✅ Resolved | `XMMatrixLookAtRH` receives same eye/target/up — normalises cross products internally |
| Missing modelview matrix stack in `ImRenderer` | ✅ Resolved | `PushMatrix`/`PopMatrix`/`MultMatrixf`/`Translatef`/`Rotatef`/`Scalef` added |
| Legacy OpenGL naming in function names | ✅ Resolved | All `*OpenGL*` / `*OpenGl*` function names renamed or removed (Phase 10) |
| `Camera::Get2DScreenPos()` Y-axis convention | ✅ Resolved | Function returns D3D11 convention (Y=0 top); 10 legacy `screenH - Y` flips removed from `camera.cpp` |
| Incomplete `toCam` declarations | ✅ Resolved | Missing initializers fixed in `gamecursor.cpp` and `taskmanager_interface_icons.cpp` |
| Water multitexture (lightmap overlay) | 🟡 Deferred | Single-texture rendering only; `GL_TEXTURE1_ARB` lightmap layer not yet reimplemented |
