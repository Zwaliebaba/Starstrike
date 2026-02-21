# OpenGL 1.x → DirectX 11 Migration Plan

## 1. Executive Summary

The Starstrike codebase currently uses **OpenGL 1.x immediate-mode** rendering (plus a handful of ARB extensions for multitexture and VBOs). The OpenGL headers and libraries are pulled in globally via `NeuronClient\NeuronClient.h` (lines 118–122), meaning every `.cpp` file in **Starstrike**, **NeuronClient**, and **GameLogic** can (and does) call `gl*` / `glu*` functions directly.

A dormant `USE_DIRECT3D` compile-time path that targeted **Direct3D 9** existed across many files. **This has been fully removed in Phase 0.** The remaining migration replaces OpenGL with **DirectX 11**.

**Total OpenGL call-sites:** ~95 source files across three projects, with the heaviest consumers being:

| File | Approx. GL calls | Category |
|---|---|---|
| `Starstrike\taskmanager_interface_icons.cpp` | 633 | 2D UI quads |
| `Starstrike\renderer.cpp` | 254 | State management, frame rendering |
| `GameLogic\darwinia_window.cpp` | 172 | 2D UI |
| `Starstrike\global_world.cpp` | 152 | 3D world rendering |
| `NeuronClient\text_renderer.cpp` | 142 | Bitmap font rendering |
| `GameLogic\weapons.cpp` | 124 | 3D entity rendering |
| `Starstrike\gamecursor.cpp` | 114 | 2D cursor |
| ~85 other files | 10–110 each | Misc rendering |

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

## 7. Phase 4 — Texture System Migration

### 7.1 Replace `BitmapRGBA::ConvertToTexture()`

**File:** `NeuronClient\bitmap.cpp` — `ConvertToTexture()` (line 706+)

Currently calls `glGenTextures`, `glBindTexture`, `glTexImage2D`, `gluBuild2DMipmaps`.

Replace with:
1. Create `ID3D11Texture2D` from pixel data (`D3D11_SUBRESOURCE_DATA`).
2. Create `ID3D11ShaderResourceView`.
3. If `_mipmapping`, either use `D3D11_RESOURCE_MISC_GENERATE_MIPS` + `GenerateMips()` or pre-generate mip chain on CPU.
4. Return a handle/index into a texture registry (keep returning `int` for API compatibility).

### 7.2 Texture Registry

Create a global texture table in `RenderDevice` (or a new `TextureManager`):
```cpp
struct TextureEntry
{
  ID3D11Texture2D*          texture;
  ID3D11ShaderResourceView* srv;
  ID3D11SamplerState*       sampler;
};
```
The `int` texture IDs returned by `ConvertToTexture()` and used by `Resource::GetTexture()` become indices into this table.

### 7.3 Replace `glBindTexture` / `glTexParameteri`

Every call site that does:
```cpp
glBindTexture(GL_TEXTURE_2D, textureId);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```
becomes:
```cpp
g_imRenderer->BindTexture(textureId);
```
The sampler state (linear, nearest, mipmap, clamp/wrap) should be set once per texture in the registry, or via a small set of pre-created `ID3D11SamplerState` objects.

**Sampler variety found in codebase:**
- Filter: `GL_LINEAR` (89 sites), `GL_NEAREST` (9 sites), `GL_LINEAR_MIPMAP_LINEAR` (in `ConvertToTexture`)
- Wrap: `GL_CLAMP` (36 sites), `GL_REPEAT` (42 sites)

Pre-create 4 sampler states:
| Sampler | Filter | Address Mode |
|---|---|---|
| `LinearClamp` | `MIN_MAG_MIP_LINEAR` | `CLAMP` |
| `LinearWrap` | `MIN_MAG_MIP_LINEAR` | `WRAP` |
| `NearestClamp` | `MIN_MAG_MIP_POINT` | `CLAMP` |
| `NearestWrap` | `MIN_MAG_MIP_POINT` | `WRAP` |

> **Note:** OpenGL's `GL_CLAMP` clamps to [0,1] with border bleed; D3D11's `D3D11_TEXTURE_ADDRESS_CLAMP` behaves like `GL_CLAMP_TO_EDGE` (no border bleed). This may cause subtle 1-pixel differences on UI texture edges. Accept this minor visual difference.

### 7.4 Remove `glEnable(GL_TEXTURE_2D)` / `glDisable(GL_TEXTURE_2D)`

In D3D11, texturing is enabled simply by binding an SRV. Replace these calls with `g_imRenderer->BindTexture(id)` / `g_imRenderer->UnbindTexture()`.

**Verification:** Build and run. Textured quads should now render correctly.

---

## 8. Phase 5 — Matrix System Migration

### 8.1 Replace OpenGL Matrix Stack

OpenGL calls used:
- `glMatrixMode(GL_PROJECTION)` / `glMatrixMode(GL_MODELVIEW)`
- `glLoadIdentity()`
- `glLoadMatrixd()`
- `glPushMatrix()` / `glPopMatrix()`
- `gluPerspective()`
- `gluOrtho2D()`
- `gluLookAt()`
- `glGetDoublev(GL_MODELVIEW_MATRIX, ...)`
- `glGetDoublev(GL_PROJECTION_MATRIX, ...)`

Replace with **DirectXMath** (`DirectX::XMMATRIX`):

| OpenGL | DirectXMath Replacement |
|---|---|
| `gluPerspective(fov, aspect, near, far)` | `XMMatrixPerspectiveFovLH(fovY, aspect, near, far)` — NOTE: DX uses LH by default; verify winding. |
| `gluOrtho2D(l, r, b, t)` | `XMMatrixOrthographicOffCenterLH(l, r, b, t, -1, 1)` |
| `gluLookAt(...)` | `XMMatrixLookAtLH(eye, target, up)` |
| `glLoadIdentity()` | `XMMatrixIdentity()` |
| `glLoadMatrixd(m)` | Construct `XMMATRIX` from the 16 doubles |

### 8.2 Files to Migrate

- **`Starstrike\renderer.cpp`** — `SetupProjMatrixFor3D()`, `SetupMatricesFor2D()`, `UpdateTotalMatrix()`, `Get2DScreenPos()`
- **`Starstrike\camera.cpp`** — `SetupModelviewMatrix()` (line 1659-1673)
- **`NeuronClient\text_renderer.cpp`** — `BeginText2D()`, `EndText2D()`
- Various `glPushMatrix` / `glPopMatrix` in GameLogic files

### 8.3 Coordinate System Difference

OpenGL uses **right-handed** coordinates; DirectX 11 conventionally uses **left-handed**. Two options:
- **Option A (recommended):** Use `XMMatrix*RH` variants (`XMMatrixLookAtRH`, `XMMatrixPerspectiveFovRH`) to preserve existing coordinate conventions and winding order. This avoids touching vertex data.
- **Option B:** Convert to LH and flip winding order (`FrontCounterClockwise = FALSE` in rasteriser state).

**Recommendation:** Use Option A to minimise changes.

### 8.4 `Matrix34::ConvertToOpenGLFormat()` — Column-Major Transpose

`Matrix34::ConvertToOpenGLFormat()` outputs a **column-major** `float[16]` for `glMultMatrixf`. DirectXMath `XMMATRIX` is **row-major** in memory. The `ImRenderer::MultMatrixf()` method already handles this transpose (swaps rows/columns on load). When migrating call sites:

```cpp
// OpenGL:
glMultMatrixf(mat.ConvertToOpenGLFormat());
// D3D11:
g_imRenderer->MultMatrixf(mat.ConvertToOpenGLFormat());  // transpose handled internally
```

No changes needed to `Matrix34` itself. The transpose is encapsulated in `ImRenderer::MultMatrixf()`.

### 8.5 `Camera::SetupModelviewMatrix()` — `gluLookAt` Scaling Quirk

In `Camera::SetupModelviewMatrix()`, the `front` and `up` vectors are **scaled by `magOfPos`** before being passed to `gluLookAt`. The `gluLookAt` function normalizes the forward/up vectors internally, so this doesn't affect the result. However, `XMMatrixLookAtRH` does NOT normalize — it expects a target POINT, not a direction. The current code computes `forwards = m_pos + front * magOfPos`, which is fine as a target point. But the `up` vector is also scaled — this works because `XMMatrixLookAtRH` normalises the cross products internally. Verify this during implementation.

### 8.6 Store Matrices in `ImRenderer`

```cpp
DirectX::XMMATRIX m_projMatrix;
DirectX::XMMATRIX m_viewMatrix;
DirectX::XMMATRIX m_worldMatrix;
```
On `End()`, compute `WVP = m_worldMatrix * m_viewMatrix * m_projMatrix` and upload to constant buffer.

**Verification:** 3D perspective rendering should now show correct geometry.

---

## 9. Phase 6 — Core 3D Rendering Migration

### 9.1 Shape Rendering

**Files:** `NeuronClient\shape.cpp` (especially `ShapeFragment::RenderSlow()` lines 843-888)

Replace:
```cpp
glBegin(GL_TRIANGLES);
  glNormal3fv(...);
  glColor4ub(...);
  glVertex3fv(...);
glEnd();
```
with:
```cpp
g_imRenderer->Begin(PRIM_TRIANGLES);
  g_imRenderer->Normal3fv(...);
  g_imRenderer->Color4ub(...);
  g_imRenderer->Vertex3fv(...);
g_imRenderer->End();
```

Also migrate `Shape::BuildDisplayList()` — OpenGL display lists (`glNewList`/`glEndList`/`glCallList`) have no D3D11 equivalent. Replace with:
- Pre-built vertex buffers (preferred for static shapes), or
- Simply remove display list caching and always use the immediate-mode emulation layer (simpler, less optimal).

### 9.2 Landscape Rendering

**File:** `Starstrike\landscape_renderer.cpp`

This file has three render modes: vertex arrays, display lists, and VBOs. Migrate to:
- Build a static `ID3D11Buffer` (vertex buffer) from `m_verts` array.
- Use `DrawIndexed` or `Draw` with triangle strips.
- Remove `glVertexPointer`, `glNormalPointer`, `glColorPointer`, `glTexCoordPointer`, `glEnableClientState`, `glDrawArrays` calls.
- Remove display list code.
- Remove `gglBindBufferARB` / `gglBufferDataARB` VBO code.

### 9.3 Sphere Rendering

**File:** `NeuronClient\sphere_renderer.cpp`

Replace `glBegin(GL_TRIANGLES)` + recursive subdivision with `g_imRenderer->Begin(PRIM_TRIANGLES)`. Remove `glNewList`/`glCallList`.

### 9.4 3D Sprite Rendering

**File:** `NeuronClient\3d_sprite.cpp`

Replace `glBegin(GL_QUADS)` billboarded sprite rendering with `g_imRenderer` calls.

### 9.5 Debug Rendering

**File:** `NeuronClient\debug_render.cpp`

Replace all `glBegin`/`glEnd` blocks (`RenderCube`, `RenderSphere`, `RenderArrow`, etc.) with `g_imRenderer` calls.

**File:** `NeuronClient\debug_render.h` — Remove `PrintMatrix` (uses `GLenum`).

### 9.6 Water, Clouds, Particles, Explosions

**Files:**
- `Starstrike\water.cpp`
- `Starstrike\clouds.cpp`
- `Starstrike\particle_system.cpp`
- `Starstrike\explosion.cpp`

**Water** uses a vertex array path (`glVertexPointer`/`glColorPointer` + `glDrawArrays(GL_TRIANGLE_STRIP, ...)`) with a dynamic `WaterVertex` array rebuilt every frame. This is NOT a `glBegin`/`glEnd` pattern and **cannot be mechanically replaced** with `g_imRenderer->Begin()/End()`. Options:
- Create a dynamic `ID3D11Buffer` in `Water` and upload `WaterVertex` data directly (preferred — avoids converting to `ImVertex`).
- Or add a `DrawArrays()` method to `ImRenderer` that accepts raw interleaved vertex data.

Clouds, particles, and explosions use immediate-mode quads/triangles and can migrate to `g_imRenderer` calls.

> **Note:** `Starstrike\water_reflection.cpp` and `Starstrike\deform.cpp` were D3D9-only and have been emptied in Phase 0. Water reflection will need to be re-implemented from scratch for D3D11 if desired (render-to-texture pass using a secondary `ID3D11RenderTargetView`).

**Verification:** All 3D geometry renders correctly.

---

## 10. Phase 7 — 2D / UI Rendering Migration

### 10.1 Text Renderer

**File:** `NeuronClient\text_renderer.cpp`

- `BeginText2D()` — Replace `glMatrixMode`/`glLoadIdentity`/`gluOrtho2D` with `g_imRenderer->SetProjectionMatrix(orthoMatrix)` and `g_imRenderer->SetViewMatrix(identity)`.
- `DrawText2DSimple()`, `DrawText2DUp()`, `DrawText2DDown()`, all `DrawText2D*` variants — Replace `glBegin(GL_QUADS)`/`glTexCoord2f`/`glVertex2f`/`glEnd` with `g_imRenderer` equivalents.
- `EndText2D()` — Restore previous matrices.
- `BuildOpenGlState()` — Replace `ConvertToTexture()` result handling (should work once Phase 4 is complete).

### 10.2 Taskmanager Interface Icons (Heaviest File)

**File:** `Starstrike\taskmanager_interface_icons.cpp` — 633 GL calls

This file is mostly 2D quads with textures and colours. Bulk search-and-replace:
- `glBegin(GL_QUADS)` → `g_imRenderer->Begin(PRIM_QUADS)`
- `glEnd()` → `g_imRenderer->End()`
- `glVertex2f(x, y)` → `g_imRenderer->Vertex2f(x, y)`
- `glTexCoord2f(u, v)` → `g_imRenderer->TexCoord2f(u, v)`
- `glColor4f(r,g,b,a)` → `g_imRenderer->Color4f(r,g,b,a)`
- `glEnable(GL_TEXTURE_2D)` → `g_imRenderer->BindTexture(...)`
- `glDisable(GL_TEXTURE_2D)` → `g_imRenderer->UnbindTexture()`
- `glEnable(GL_BLEND)` / `glBlendFunc(...)` → `g_renderDevice->SetBlendState(...)`

### 10.3 All UI Windows (GameLogic)

**Files (partial list, all in GameLogic):**
- `darwinia_window.cpp`, `mainmenus.cpp`, `scrollbar.cpp`, `drop_down_menu.cpp`, `input_field.cpp`, `profilewindow.cpp`, `prefs_graphics_window.cpp`, `prefs_screen_window.cpp`, `prefs_sound_window.cpp`, `prefs_other_window.cpp`, `network_window.cpp`, `userprofile_window.cpp`

All follow the same pattern of `glBegin(GL_QUADS)` for UI elements. Apply the same mechanical replacement.

### 10.4 Game Cursor

**File:** `Starstrike\gamecursor.cpp`

Replace immediate-mode 2D drawing with `g_imRenderer`.

### 10.5 Start Sequence

**File:** `Starstrike\startsequence.cpp`

Replace `glBegin`/`glEnd` blocks.

### 10.6 Game Menu

**File:** `Starstrike\game_menu.cpp`

Replace `glBegin`/`glEnd` blocks.

**Verification:** All 2D UI renders correctly.

---

## 11. Phase 8 — Entity / GameLogic 3D Rendering Migration

### 11.1 Entity Rendering Files

All of these files contain `Render()` methods with `glBegin`/`glEnd` blocks, `glColor`, `glVertex`, `glEnable(GL_BLEND)`, etc.:

- `weapons.cpp`, `darwinian.cpp`, `officer.cpp`, `engineer.cpp`, `insertion_squad.cpp`
- `armour.cpp`, `armyant.cpp`, `centipede.cpp`, `spider.cpp`, `souldestroyer.cpp`, `tripod.cpp`
- `virii.cpp`, `lasertrooper.cpp`, `lander.cpp`
- `building.cpp`, `generator.cpp`, `laserfence.cpp`, `factory.cpp`, `bridge.cpp`
- `incubator.cpp`, `constructionyard.cpp`, `controltower.cpp`, `gunturret.cpp`
- `radardish.cpp`, `researchitem.cpp`, `safearea.cpp`, `switch.cpp`
- `teleport.cpp`, `trunkport.cpp`, `triffid.cpp`, `tree.cpp`
- `spirit.cpp`, `spiritreceiver.cpp`, `spiritstore.cpp`, `sporegenerator.cpp`
- `rocket.cpp`, `airstrike.cpp`, `anthill.cpp`, `mine.cpp`
- `egg.cpp`, `flag.cpp`, `goddish.cpp`, `spawnpoint.cpp`
- `snow.cpp`, `spam.cpp`, `feedingtube.cpp`
- `blueprintstore.cpp`, `ai.cpp`

Apply the same mechanical `gl*` → `g_imRenderer->*` replacement.

### 11.2 Location Rendering

**Files:**
- `Starstrike\location.cpp` — `SetupFog()`, `SetupLights()`, `Render()`
- `Starstrike\global_world.cpp` — `SetupFog()`, `SetupLights()`, `Render()`
- `Starstrike\entity_grid.cpp`
- `Starstrike\obstruction_grid.cpp`
- `Starstrike\routing_system.cpp`
- `Starstrike\team.cpp`
- `Starstrike\user_input.cpp`
- `Starstrike\location_input.cpp`
- `Starstrike\global_internet.cpp`
- `Starstrike\3d_sierpinski_gasket.cpp`

Replace `glFog*` calls with constant buffer updates. Replace `glLight*` calls with light parameter updates. Replace `glBegin`/`glEnd` blocks with `g_imRenderer`.

**Verification:** Full game renders with D3D11.

---

## 12. Phase 9 — OGL Extension & Utility Cleanup

### 12.1 Remove OpenGL Extension System

**Delete files:**
- `NeuronClient\ogl_extensions.h`
- `NeuronClient\ogl_extensions.cpp`

**Remove references:**
- `#include "ogl_extensions.h"` from `Starstrike\renderer.cpp`, `Starstrike\landscape_renderer.cpp`, and any other files.
- Remove `InitialiseOGLExtensions()` call.

### 12.2 Remove OpenGL Utility Files

**File:** `NeuronClient\debug_fog_logging.cpp` — Remove all `glGetFloatv(GL_FOG_*)` calls.

**File:** `NeuronClient\render_utils.cpp` — Audit and replace any remaining GL calls.

### 12.3 Remove `CHECK_OPENGL_STATE()` Macro

**File:** `Starstrike\renderer.h` — Remove `#define CHECK_OPENGL_STATE()`.

Remove all `CHECK_OPENGL_STATE()` call sites in `renderer.cpp` and anywhere else.

### 12.4 Remove OpenGL from Display List System

**File:** `NeuronClient\resource.cpp` — `HashTable<int> m_displayLists` and any `glNewList`/`glCallList`/`glDeleteLists` calls.

### 12.5 Remove OpenGL from PCH

**File:** `NeuronClient\NeuronClient.h` — Remove lines 118-122:
```cpp
#include <GL/gl.h>
#include <GL/glu.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
```

Replace with:
```cpp
#include <d3d11.h>
#include <DirectXMath.h>
```

**Verification:** Full clean build with zero OpenGL references. `opengl32.lib` and `glu32.lib` are no longer linked.

---

## 13. Phase 10 — Polish & Optimisation

### 13.1 Batch Optimisation
The `ImRenderer` immediate-mode emulation works but is inherently slow (one draw call per `Begin`/`End` pair). After correctness is verified:

- **Merge batches:** Consecutive `Begin`/`End` pairs with the same state (same texture, same blend mode) can be merged into a single draw call.
- **Static vertex buffers:** For geometry that doesn't change (landscape, shapes with display lists), create static `ID3D11Buffer` objects uploaded once.

### 13.2 Replace `Renderer::UpdateTotalMatrix()`

**File:** `Starstrike\renderer.cpp` (lines 821-863)

Currently reads back matrices from OpenGL via `glGetDoublev`. Replace with direct computation from the stored `XMMATRIX` values:
```cpp
XMMATRIX totalMatrix = m_worldMatrix * m_viewMatrix * m_projMatrix;
XMStoreFloat4x4(&m_totalMatrix, totalMatrix);
```

### 13.3 Remove `Renderer::GetGLStateInt()` / `GetGLStateFloat()`

**File:** `Starstrike\renderer.h` / `renderer.cpp`

Remove these methods and all callers.

### 13.4 Update `SystemInfo::GetDirectXVersion()`

**File:** `NeuronClient\system_info.cpp`

Change `m_directXVersion = 9;` → `m_directXVersion = 11;` (or query feature level from `g_renderDevice`).

### 13.5 Remove `GLenum` References in Headers

**File:** `NeuronClient\debug_render.h` — Remove `PrintMatrix(const char*, GLenum)` declaration.

Grep the entire codebase for `GLenum`, `GLint`, `GLfloat`, `GLuint`, `GLboolean`, `GLubyte`, `GLvoid`, `GLsizei`, `GLsizeiptrARB`, `GLintptrARB` and remove/replace all occurrences.

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

### Deleted Files
| File | Phase | Reason |
|---|---|---|
| `NeuronClient\opengl_directx_internals.h` | 0 ✅ | Empty / unused |
| `NeuronClient\ogl_extensions.h` | 9 | OpenGL extension definitions |
| `NeuronClient\ogl_extensions.cpp` | 9 | OpenGL extension loading |

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
- [ ] **Phase 4:** Migrate texture system (`ConvertToTexture`, `Resource`, `glBindTexture`)
- [ ] **Phase 5:** Migrate matrix system (projection, modelview, `gluPerspective`, `gluLookAt`)
- [ ] **Phase 6:** Migrate core 3D rendering (shapes, landscape, spheres, particles, water, clouds)
- [ ] **Phase 7:** Migrate 2D/UI rendering (text, HUD, menus, cursors, taskmanager icons)
- [ ] **Phase 8:** Migrate entity/GameLogic rendering (~65 files)
- [ ] **Phase 9:** Remove all OpenGL code, headers, libs
- [ ] **Phase 10:** Polish, optimise batching, static VBs, final cleanup

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
| Coordinate handedness mismatch (RH→LH) | 🟠 Moderate | Use `XMMatrix*RH` variants to preserve existing vertex data and winding |
| Texture coordinate origin difference (GL bottom-left vs D3D top-left) | 🟠 Moderate | Flip V coordinate in `ConvertToTexture()` or in HLSL |
| `GL_QUADS` has no D3D primitive | ✅ Resolved | `ImRenderer` converts quads to triangle pairs |
| `glAlphaFunc` / alpha test has no D3D11 fixed function | 🟡 Important | Implement via `clip()` in pixel shader |
| Display lists have no D3D11 equivalent | 🟡 Important | Replace with static vertex buffers or remove caching |
| Performance regression from immediate-mode emulation | 🟠 Moderate | Acceptable initially; optimise with static VBs in Phase 10 |
| GL state queries (`glGet*`) used in debug asserts | 🟡 Important | Remove or replace with cached state tracking |
| `gluBuild2DMipmaps` has no D3D11 equivalent | 🟠 Moderate | Use `GenerateMips()` or CPU-side mip generation |
| Large number of files to touch (~95) | 🟠 Moderate | Mechanical replacement; use scripts/regex where possible |
| `glLineWidth` > 1.0 unsupported in D3D11 (36 sites) | 🔴 Critical | Accept 1px initially; implement thick-line quad expansion in Phase 10 |
| Save/restore state RAII pattern in `text_renderer.cpp` | 🟡 Important | Replace with explicit save/restore via `ImRenderer` accessors |
| Vertex array paths (landscape, water) not coverable by `ImRenderer::Begin/End` | 🟡 Important | Create direct `ID3D11Buffer` for these systems |
| `Matrix34::ConvertToOpenGLFormat()` column-major vs `XMMATRIX` row-major | ✅ Resolved | `ImRenderer::MultMatrixf()` transposes on load |
| `GL_CLAMP` vs `D3D11_TEXTURE_ADDRESS_CLAMP` (border pixel bleed) | 🟢 Minor | Accept subtle 1-pixel difference at texture edges |
| `Camera::SetupModelviewMatrix()` scales vectors by `magOfPos` before `gluLookAt` | 🟢 Minor | Verify `XMMatrixLookAtRH` produces same result (it normalises internally) |
| Missing modelview matrix stack in `ImRenderer` | ✅ Resolved | `PushMatrix`/`PopMatrix`/`MultMatrixf`/`Translatef`/`Rotatef`/`Scalef` added |
