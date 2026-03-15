# Decouple `TreeRenderer` from `d3d12_backend.h` and `opengl_directx.h`

## Goal

Make `tree_renderer.cpp` / `tree_renderer.h` independent of the OpenGL
translation layer (`opengl_directx.h`, `d3d12_backend.h`).  After this work
the only engine headers TreeRenderer includes are its own types,
`GraphicsCore.h`, `UploadRingBuffer.h`, `PipelineState.h`, and a new
lightweight `DrawConstants.h`.

---

## Dependency Audit

Current usages of the two headers inside `tree_renderer.cpp`:

| # | Symbol | Source Header | Used In |
|---|--------|---------------|---------|
| 1 | `OpenGLD3D::g_glState.GetRootSignature()` | `d3d12_backend.h` | `Init()` |
| 2 | `OpenGLD3D::g_glState.GetSamplerBaseGPUHandle()` | `d3d12_backend.h` | `DrawTree()` |
| 3 | `OpenGLD3D::DrawConstants` (struct) | `d3d12_backend.h` | `DrawTree()` |
| 4 | `OpenGLD3D::GetModelViewStack().GetTopXM()` | `opengl_directx.h` | `DrawTree()` |
| 5 | `OpenGLD3D::GetProjectionStack().GetTopXM()` | `opengl_directx.h` | `DrawTree()` |
| 6 | `OpenGLD3D::GetFogState()` | `opengl_directx.h` | `DrawTree()` |
| 7 | `OpenGLD3D::GetTextureSRVGPUHandle()` | `opengl_directx.h` | `DrawTree()` |
| 8 | `OpenGLD3D::EnsureSceneConstantsUploaded()` | `opengl_directx.h` | `DrawTree()` |
| 9 | `OpenGLD3D::GetSceneConstantsGPUAddr()` | `opengl_directx.h` | `DrawTree()` |

Dependencies on `Graphics::Core` (device, command list, upload buffer, formats)
are **kept** — that is engine infrastructure, not the OpenGL shim.

---

## Step 1 — Extract `DrawConstants` to a standalone header

Create **`NeuronClient/DrawConstants.h`** containing `LightConstants` and
`DrawConstants` (currently defined inside `d3d12_backend.h`).  These are pure
data-layout structs that match HLSL constant buffers and have no logical
relationship with the OpenGL translation layer.

- `d3d12_backend.h` replaces the inline definitions with
  `#include "DrawConstants.h"` (no API break for existing consumers).
- `tree_renderer.cpp` includes `DrawConstants.h` directly.

---

## Step 2 — Move the root signature to `Graphics::Core`

There is only one root signature for the entire 3D world pipeline.  It
currently lives inside `OpenGLTranslationState` (`g_glState`), but it is
shared engine infrastructure — not an OpenGL concern.

1. Add a `const RootSignature& GetRootSignature()` accessor to
   `Graphics::Core`.
2. `OpenGLTranslationState::Init()` creates the root signature as before,
   then registers it with `Graphics::Core` (e.g. a
   `SetRootSignature(const RootSignature&)` setter, or Core owns creation
   directly).
3. All consumers — including `TreeRenderer::Init()` — query
   `Graphics::Core::Get().GetRootSignature()` instead of touching
   `g_glState`.

With this in place, `TreeRenderer::Init()` stays **parameterless**.  It
already reads back-buffer and depth formats from `Graphics::Core`; now the
root signature comes from the same place:

```cpp
void TreeRenderer::Init()
{
  // ...
  m_pso.SetRootSignature(Graphics::Core::Get().GetRootSignature());
  m_pso.SetRenderTargetFormat(Graphics::Core::Get().GetBackBufferFormat(),
                              Graphics::Core::Get().GetDepthBufferFormat());
  // ...
}
```

---

## Step 3 — Introduce `TreeDrawParams` for per-frame draw state

Add a new struct (in `tree_renderer.h`):

```cpp
struct TreeDrawParams
{
    DirectX::XMMATRIX                viewMatrix;
    DirectX::XMMATRIX                projMatrix;
    bool                             fogEnabled;
    D3D12_GPU_DESCRIPTOR_HANDLE      textureSRV;
    D3D12_GPU_DESCRIPTOR_HANDLE      samplerBaseHandle;
    D3D12_GPU_VIRTUAL_ADDRESS        sceneConstantsGPUAddr;
};
```

---

## Step 4 — Refactor `DrawTree()` to accept `TreeDrawParams`

Change the signature from:

```cpp
void DrawTree(Tree* _tree, float _predictionTime,
              unsigned int _textureGLId, bool _skipLeaves);
```

to:

```cpp
void DrawTree(Tree* _tree, float _predictionTime,
              const TreeDrawParams& _params, bool _skipLeaves = false);
```

Inside `DrawTree()`:

- Replace `OpenGLD3D::GetModelViewStack().GetTopXM()` with `_params.viewMatrix`.
- Replace `OpenGLD3D::GetProjectionStack().GetTopXM()` with `_params.projMatrix`.
- Replace `OpenGLD3D::GetFogState().enabled` with `_params.fogEnabled`.
- Replace `OpenGLD3D::GetTextureSRVGPUHandle()` with `_params.textureSRV`.
- Replace `OpenGLD3D::g_glState.GetSamplerBaseGPUHandle()` with
  `_params.samplerBaseHandle`.
- Replace `OpenGLD3D::EnsureSceneConstantsUploaded()` /
  `GetSceneConstantsGPUAddr()` with `_params.sceneConstantsGPUAddr`
  (caller is responsible for ensuring upload).
- Remove the `m_textureLoaded` / `m_treeTextureSRV` lazy-cache fields from
  the class — the caller now owns texture resolution.

---

## Step 5 — Update the call site (`Starstrike/location.cpp`)

The single call site at `location.cpp:1087` becomes:

```cpp
TreeDrawParams params;
params.viewMatrix            = OpenGLD3D::GetModelViewStack().GetTopXM();
params.projMatrix            = OpenGLD3D::GetProjectionStack().GetTopXM();
params.fogEnabled            = OpenGLD3D::GetFogState().enabled;
params.textureSRV            = OpenGLD3D::GetTextureSRVGPUHandle(s_treeTextureId);
params.samplerBaseHandle     = OpenGLD3D::g_glState.GetSamplerBaseGPUHandle();
OpenGLD3D::EnsureSceneConstantsUploaded();
params.sceneConstantsGPUAddr = OpenGLD3D::GetSceneConstantsGPUAddr();

TreeRenderer::Get().DrawTree(tree, predTime, params, skipLeaves);
```

`location.cpp` already includes `opengl_directx.h` and `d3d12_backend.h`, so
no new includes are introduced there.

If all trees in the loop share the same params (they do — view/proj/fog/texture
don't change between trees), hoist `TreeDrawParams` construction above the
loop for a minor perf win.

---

## Step 6 — Update the `Init()` call site

No signature change needed — `Init()` is still parameterless.  Just make sure
it is called **after** the root signature has been registered with
`Graphics::Core` (i.e. after `OpenGLTranslationState::Init()` runs).
The existing call order already satisfies this.

---

## Step 7 — Remove includes from `tree_renderer.cpp`

```diff
- #include "d3d12_backend.h"
- #include "opengl_directx.h"
+ #include "DrawConstants.h"
```

---

## Step 8 — Clean up `tree_renderer.h`

Remove fields that are no longer needed:

```diff
- D3D12_GPU_DESCRIPTOR_HANDLE m_treeTextureSRV{};
- bool m_textureLoaded = false;
```

Update `Shutdown()` to remove the `m_textureLoaded = false` line.

---

## File Change Summary

| File | Action |
|------|--------|
| `NeuronClient/DrawConstants.h` | **New** — `LightConstants` + `DrawConstants` structs |
| `NeuronClient/d3d12_backend.h` | Replace inline struct defs with `#include "DrawConstants.h"` |
| `NeuronClient/GraphicsCore.h` | Add `SetRootSignature()` / `GetRootSignature()` accessor |
| `NeuronClient/opengl_directx.cpp` | Register root signature with `Graphics::Core` after creation |
| `NeuronClient/tree_renderer.h` | Add `TreeDrawParams`; update `DrawTree()` signature; remove cached texture fields |
| `NeuronClient/tree_renderer.cpp` | Swap includes; use params instead of global queries |
| `Starstrike/location.cpp` | Build `TreeDrawParams` at the call site |

---

## What Stays Coupled (intentionally)

- **`Graphics::Core`** — engine-level D3D12 infrastructure (device, command
  list, upload buffer).  TreeRenderer *is* a D3D12 renderer; this coupling is
  appropriate.
- **`PipelineState.h` / `RootSignature.h`** — engine-level PSO and root
  signature wrappers, not part of the OpenGL translation layer.
- **`UploadRingBuffer.h`** — engine upload infrastructure.
