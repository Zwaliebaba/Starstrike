# DirectX 12 Rendering Migration Strategy

## Migration: `D3D12Backend` → `Neuron::Graphics::Core`

### Executive Summary

This document defines a phased, incremental migration plan to retire the legacy
`OpenGLD3D::D3D12Backend` (a self-contained DX12 backend with its own device,
swap chain, command infrastructure, descriptor heaps, PSO cache, upload ring
buffers, and root signature) and replace it with the modern
`Neuron::Graphics::Core` subsystem (which already owns the device, swap chain,
command list, fence, descriptor allocator, and resource-state tracker).

**Both systems currently coexist at runtime:** `Graphics::Core` is started in
`ClientEngine::Startup()` before `Direct3DInit()` creates the legacy backend's
*second* device and swap chain. The legacy backend drives all actual rendering
through the OpenGL-to-DX12 translation layer (`opengl_directx.cpp`,
`opengl_directx_dlist.cpp`, `ogl_extensions_directx.cpp`). The new `Core` owns
the window and provides device resources but nothing renders through it yet.

The goal is to make the OpenGL translation layer render through `Core`'s device
and command infrastructure, then delete `D3D12Backend` entirely.

---

## 1. Architectural Overview

### 1.1 Current Dual-Device Layout

```
┌───────────────────────────────────────────────────────────────┐
│ ClientEngine::Startup()                                       │
│   Graphics::Core::Startup()          ← creates Device A       │
│   Direct3DInit(hWnd, ...)            ← creates Device B       │
│     └─ g_backend.Init(...)              (D3D12Backend)        │
│                                                               │
│ Frame Loop (WindowManager::Flip)                              │
│   └─ Direct3DSwapBuffers()                                    │
│       └─ g_backend.EndFrame()        ← presents on Device B   │
│                                                               │
│ ClientEngine::Shutdown()                                      │
│   Direct3DShutdown()                 ← destroys Device B      │
│   Graphics::Core::Shutdown()         ← destroys Device A      │
└───────────────────────────────────────────────────────────────┘
```

### 1.2 Target Single-Device Layout

```
┌───────────────────────────────────────────────────────────────┐
│ ClientEngine::Startup()                                       │
│   Graphics::Core::Startup()          ← creates Device         │
│   Graphics::Core::SetWindow(hWnd)    ← creates swap chain     │
│   LegacyBridge::Init()              ← creates upload bufs,    │
│                                        root sig, PSO cache,   │
│                                        samplers on Core's     │
│                                        device                 │
│                                                               │
│ Frame Loop                                                    │
│   Graphics::Core::Prepare()          ← reset allocator,       │
│   ... gl* calls ...                     set descriptor heaps  │
│   Graphics::Core::Present()          ← execute, present,      │
│                                        move to next frame     │
│                                                               │
│ ClientEngine::Shutdown()                                      │
│   LegacyBridge::Shutdown()           ← destroys bridge objects│
│   Graphics::Core::Shutdown()         ← destroys Device        │
└───────────────────────────────────────────────────────────────┘
```

### 1.3 Files Affected

| File | Role | Impact |
|------|------|--------|
| `d3d12_backend.h/.cpp` | Legacy self-contained DX12 backend | **Delete at end** |
| `opengl_directx.cpp` | OpenGL→DX12 translation (~1485 lines) | Redirect `g_backend` calls |
| `opengl_directx_dlist.cpp` | Display list recording/replay | Redirect `g_backend` calls |
| `ogl_extensions_directx.cpp` | VBO extension emulation | Redirect `g_backend` calls |
| `NeuronClient.cpp` | Engine startup/shutdown | Remove `Direct3DInit`/`Direct3DShutdown` |
| `window_manager_directx.cpp` | Frame flip | Replace `Direct3DSwapBuffers` |
| `GraphicsCore.h/.cpp` | New Core subsystem | Minor additions (if needed) |
| `DescriptorHeap.h/.cpp` | Core's descriptor allocator | Already functional |
| `ResourceStateTracker.h/.cpp` | Core's barrier tracker | Already functional |
| `GraphicsCommon.h/.cpp` | Common rasterizer/blend/depth states | Already functional |
| `PipelineState.h/.cpp` | Core's modern PSO wrapper | Consumed in later phases |
| `LegacyBridge.h/.cpp` | Transitional bridge (Phase 0+) | **New — Phase 0** |

---

## 2. Subsystem-by-Subsystem Mapping

This section maps every responsibility of `D3D12Backend` to its `Core`
equivalent, identifying gaps that require bridge code.

| D3D12Backend Responsibility | Core Equivalent | Gap / Bridge Needed |
|---|---|---|
| **Device creation** (`CreateDevice`) | `Core::CreateDeviceResources` — creates `ID3D12Device10` | None. Core already creates a more capable device. |
| **Command queue** (`CreateCommandQueue`) | `Core::CreateDeviceResources` — `m_commandQueue` | None. |
| **Swap chain** (`CreateSwapChain`) | `Core::CreateWindowSizeDependentResources` | Core needs `SetWindow()` called; format differs (B8G8R8A8 vs R8G8B8A8). Configurable via `Startup()` params. |
| **RTV / DSV heaps** (`CreateRTVAndDSV`) | `Core::CreateWindowSizeDependentResources` — uses `DescriptorAllocator` | None. Core uses `GpuResource` wrappers. |
| **SRV/CBV descriptor heap** (shader-visible, 1024 slots) | `DescriptorAllocator` (CPU-side heaps, 256/type) | **Gap**: Core's allocator is CPU-side, not shader-visible. The OpenGL layer needs shader-visible SRV and sampler heaps. **Bridge must own these.** |
| **Sampler heap** (shader-visible, 24 slots) | `SamplerManager` / `GraphicsCommon` samplers | **Gap**: Core samplers are CPU descriptors. Bridge must create a shader-visible sampler heap or use `DescriptorAllocator::SetDescriptorHeaps`. |
| **Root signature** (`CreateRootSignature`) | `RootSignature` class exists | **Bridge**: Must create the same root signature layout (CBV b0, SRV t0, SRV t1, Sampler table) using the `RootSignature` helper. |
| **PSO cache** (`GetOrCreatePSO`) | `GraphicsPSO` class exists | **Bridge**: Must reimplement the `PSOKey`→PSO caching using `GraphicsPSO`. |
| **Upload ring buffer** (16 MB/frame) | No equivalent in Core | **Bridge must keep**: `UploadRingBuffer` is independent of the device — just needs to be initialized on Core's device. |
| **Constant buffer** (`UploadConstants`) | No equivalent in Core | **Bridge must keep**: Ring-buffer allocation replaces the fixed constant buffer. |
| **Default 1×1 white texture** (`CreateDefaultTexture`) | No equivalent | **Bridge must keep**. |
| **Frame management** (`BeginFrame`/`EndFrame`/`MoveToNextFrame`) | `Core::Prepare()` / `Core::Present()` / `Core::MoveToNextFrame()` | Direct replacement. Barrier management handled by `ResourceStateTracker`. |
| **Fence / WaitForGpu** | `Core::WaitForGpu()` | Direct replacement. |
| **Command allocator reset** | `Core::ResetCommandAllocatorAndCommandlist()` | Direct replacement. |
| **Command list close + execute** | `Core::ExecuteCommandList()` | Direct replacement. |

---

## 3. Phased Migration Plan

### Phase 0: Pre-Migration Preparation (Risk Reduction) ✅ COMPLETE

**Goal:** Establish baseline correctness and create the bridge abstraction layer
without changing any runtime behavior.

**Status:** Completed. `LegacyBridge.h/.cpp` created with stub implementations,
added to `NeuronClient.vcxproj` and filters under `Graphics\Legacy`. Build
verified clean. Zero references to `LegacyBridge` outside its own files — no
runtime behavior changed. 39 existing `g_backend` call sites confirmed untouched.

#### Steps

1. **Create a visual regression baseline.**
   - Capture reference screenshots at key rendering moments (menu, in-game, fog,
     transparency, display lists) using the current dual-device setup.
   - Store as `.png` files in a `test/baselines/` directory.
   - ⚠️ Manual step — to be done before starting Phase 1.

2. **Audit `g_backend` call sites.** ✅
   All call sites are in exactly 4 files (39 total references):
   - `opengl_directx.cpp` (27 call sites)
   - `opengl_directx_dlist.cpp` (6 call sites)
   - `ogl_extensions_directx.cpp` (4 call sites)
   - `d3d12_backend.cpp` (definition + internal use)

3. **Classify each call site by subsystem:** ✅
   - **Device access**: `GetDevice()` → 3 sites
   - **Command list access**: `GetCommandList()` → 8 sites
   - **Upload buffer access**: `GetUploadBuffer()` → 6 sites
   - **Descriptor heap access**: `GetSRVCBVHeap()`, `AllocateSRVSlot()`,
     `GetSRVGPUHandle()`, `GetSRVDescriptorSize()` → 6 sites
   - **Sampler access**: `GetSamplerHeap()`, `GetSamplerDescriptorSize()` → 2 sites
   - **PSO cache**: `GetOrCreatePSO()` → 1 site
   - **Root signature**: `GetRootSignature()` → implicit (set in `BeginFrame`)
   - **RTV/DSV**: `GetCurrentRTVHandle()`, `GetCurrentDSVHandle()` → 2 sites
   - **Frame lifecycle**: `Init()`, `Shutdown()`, `EndFrame()`, `WaitForGpu()` → 4 sites
   - **Default texture**: `GetDefaultTextureSRVIndex()` → 2 sites

4. **Create `LegacyBridge.h/.cpp`** ✅

   Created `NeuronClient/LegacyBridge.h` — static class with the full bridge
   interface including `BeginFrame()` and `ResetUploadBuffer()` (identified as
   needed during Phase 5 analysis). Temporarily includes `d3d12_backend.h` for
   `UploadRingBuffer` and `PSOKey` type definitions.

   Created `NeuronClient/LegacyBridge.cpp` — stub implementations that call
   `Fatal()` if invoked before their respective migration phase. `Init()` and
   `Shutdown()` are no-ops (safe to call).

   Added both files to `NeuronClient.vcxproj` and
   `NeuronClient.vcxproj.filters` under the `Graphics\Legacy` filter.

   **Files are NOT wired into the rendering path.** All rendering still goes
   through `g_backend`.

5. **Verify build succeeds.** ✅ Build clean, no errors.

#### Validation
- Build succeeds.
- Game launches and renders identically to baseline.
- No `D3D12 Debug Layer` errors.

#### Risks
- None. This is purely additive.

---

### Phase 1: Unify the Device (Eliminate Dual-Device) ✅ COMPLETE

**Goal:** Make `D3D12Backend` use `Core`'s device, swap chain, command queue,
and fence instead of creating its own. Both systems share one `ID3D12Device`.

**Status:** Completed. Backend no longer creates or owns device, swap chain,
command queue, command list, command allocators, fence, render targets, or
depth stencil. All Core-owned `ComPtr` members removed from `D3D12Backend`.
Build verified clean. External API surface (`g_backend.*`) unchanged (39 refs).

#### What was done

1. **`d3d12_backend.h`:** ✅
   - `Init()` signature simplified to `bool Init()` (no params — device info from Core).
   - Removed 17 `ComPtr` members and 5 scalar members for Core-owned resources.
   - Removed `CreateDevice`, `CreateCommandQueue`, `CreateSwapChain`,
     `CreateRTVAndDSV`, `CreateConstantBuffer`, `MoveToNextFrame`.
   - Removed dead accessors: `GetWidth`, `GetHeight`, `UploadConstants`,
     `GetConstantBufferGPUAddress`.
   - `GetDevice()` / `GetCommandList()` now delegate to Core.
   - `GetCurrentRTVHandle()` / `GetCurrentDSVHandle()` now delegate to Core.
   - `GetUploadBuffer()` indexes by `Core::GetCurrentFrameIndex()`.

2. **`d3d12_backend.cpp`:** ✅
   - `Init()` reduced to: `CreateDescriptorHeaps`, `CreateRootSignature`,
     `CreateUploadBuffer`, `CreateDefaultTexture`, `CreateSamplers`, `BeginFrame`.
     No device/queue/swap chain/fence creation. Uses Core's command list for
     init-time GPU work (default texture upload).
   - `Shutdown()` is now empty (ComPtr destructors handle bridge resources).
   - `BeginFrame()` sets shader-visible descriptor heaps, root signature, and
     render targets from `Core::GetRenderTargetView()` / `GetDepthStencilView()`.
     No manual barriers (Core::Prepare does them).
   - `EndFrame()` transitions via `ResourceStateTracker`, delegates to
     `Core::Present()` + `Core::Prepare()`, resets upload buffer, calls
     `BeginFrame()` for next frame.
   - `WaitForGpu()` delegates to `Core::WaitForGpu()`.
   - All `m_device->` calls replaced with `GetDevice()->` (delegates to Core).
   - PSO `RTVFormats[0]` and `DSVFormat` now read from
     `Core::GetBackBufferFormat()` / `Core::GetDepthBufferFormat()`.
   - Deleted 6 methods and ~200 lines of Core-duplicated infrastructure.

3. **`NeuronClient.cpp`:** ✅
   - `Core::Startup(DXGI_FORMAT_R8G8B8A8_UNORM)` — aligns back buffer format.
   - Added `Core::SetWindow(m_hwnd, w, h)` to create swap chain.
   - Added `Core::Prepare()` to open command list before `Direct3DInit()`.
   - `Direct3DInit()` call simplified (no params).

4. **`opengl_directx.cpp`:** ✅
   - `Direct3DInit()` signature simplified to `bool Direct3DInit()`.
   - Viewport dimensions read from `Core::GetOutputSize()` instead of params.

5. **No changes needed** to `window_manager_directx.cpp` —
   `Direct3DSwapBuffers()` still calls `g_backend.EndFrame()` which now
   internally delegates to Core. This preserves the FPS counter and existing
   call chain.

#### Validation
- ✅ Build succeeds (0 errors, 0 warnings in backend changes).
- Only one device visible in GPU debugger (PIX / RenderDoc). ← Manual test needed.
- ✅ D3D12 Debug Layer clean (live object leak fixed — see Phase 1 bugfix below).
- Visual output matches Phase 0 baseline screenshots. ← Manual test needed.
- Fence values advance correctly (no GPU hangs or CPU stalls). ← Manual test needed.
- `Alt+Tab`, minimize/restore, and resize work without crashes. ← Manual test needed.

#### Phase 1 Bugfix: Live Object Leak on Shutdown

**Problem:** D3D12 debug layer reported ~75 live objects at process termination.
`D3D12Backend::Shutdown()` was empty, so ComPtr members (descriptor heaps, root
signature, PSO cache, upload buffers, default texture) were only released at
static destruction time — after `Core::Shutdown()` had already destroyed the
device. Additionally, `Direct3DShutdown()` did not release OpenGL layer GPU
resources (textures, VBOs, display lists).

**Fix:**
- Added `UploadRingBuffer::Shutdown()` — unmaps and releases the upload buffer resource.
- `D3D12Backend::Shutdown()` now calls `WaitForGpu()` then explicitly releases all
  owned resources (`m_psoCache`, `m_rootSignature`, `m_srvCbvHeap`, `m_samplerHeap`,
  `m_defaultTexture`, upload buffers).
- `Direct3DShutdown()` now clears `s_textureResources` (OpenGL textures),
  deletes all display lists, calls `ShutdownOGLExtensions()` (VBOs), and frees
  the vertex scratch buffer before calling `g_backend.Shutdown()`.
- Added `ShutdownOGLExtensions()` to `ogl_extensions_directx.cpp` / `ogl_extensions.h`
  to clear `s_vertexBuffers`.

**Files changed:** `d3d12_backend.h`, `d3d12_backend.cpp`, `opengl_directx.cpp`,
`ogl_extensions_directx.cpp`, `ogl_extensions.h`.

#### Risks (Mitigated)
| Risk | Status |
|------|--------|
| Format mismatch (BGRA vs RGBA) | ✅ Mitigated: `Core::Startup(DXGI_FORMAT_R8G8B8A8_UNORM)` aligns format. PSO reads `Core::GetBackBufferFormat()`. |
| Command list state leak | ✅ Mitigated: `BeginFrame()` sets descriptor heaps and root sig after `Prepare()`. |
| Fence double-signal | ✅ Mitigated: Backend fence fully removed; only Core signals. |
| RTV descriptor mismatch | ✅ Mitigated: `GetCurrentRTVHandle()` delegates to `Core::GetRenderTargetView()`. |
| DSV format mismatch (D24 → D32) | ✅ Mitigated: PSO reads `Core::GetDepthBufferFormat()` (D32_FLOAT). |
| Live object leak at shutdown | ✅ Fixed: explicit resource release in `Shutdown()` + `Direct3DShutdown()`. |

---

### Phase 2: Migrate Descriptor Heaps to Bridge

**Goal:** Move the shader-visible SRV/CBV and Sampler heaps out of
`D3D12Backend` into `LegacyBridge`, initializing them on `Core`'s device.

#### Steps

1. **Implement `LegacyBridge::Init()`.** Create the two shader-visible heaps on
   `Core::GetD3DDevice()`:
   - SRV/CBV heap: `MAX_SRV_DESCRIPTORS = 1024`, shader-visible.
   - Sampler heap: `NUM_STATIC_SAMPLERS + 16`, shader-visible.
   Store the heap pointers, descriptor sizes, and next-free-slot counter as
   `LegacyBridge` static members.

2. **Move sampler creation** from `D3D12Backend::CreateSamplers()` into
   `LegacyBridge::Init()`. Same sampler configs, same heap layout.

3. **Move default texture creation** from `D3D12Backend::CreateDefaultTexture()`
   into `LegacyBridge::Init()`. Uses `Core::GetD3DDevice()` and
   `Core::GetCommandList()`.

4. **Move SRV allocation** (`AllocateSRVSlot`, `GetSRVGPUHandle`) into
   `LegacyBridge`.

5. **Redirect consumer call sites** (mechanical find-and-replace):
   | Old call | New call |
   |----------|----------|
   | `g_backend.GetSRVCBVHeap()` | `LegacyBridge::GetSRVCBVHeap()` |
   | `g_backend.AllocateSRVSlot(idx)` | `LegacyBridge::AllocateSRVSlot(idx)` |
   | `g_backend.GetSRVGPUHandle(idx)` | `LegacyBridge::GetSRVGPUHandle(idx)` |
   | `g_backend.GetSRVDescriptorSize()` | `LegacyBridge::GetSRVDescriptorSize()` |
   | `g_backend.GetSamplerHeap()` | `LegacyBridge::GetSamplerHeap()` |
   | `g_backend.GetSamplerDescriptorSize()` | `LegacyBridge::GetSamplerDescriptorSize()` |
   | `g_backend.GetDefaultTextureSRVIndex()` | `LegacyBridge::GetDefaultTextureSRVIndex()` |

6. **Remove these members and methods from `D3D12Backend`.**

7. **Update `D3D12Backend::BeginFrame()` `SetDescriptorHeaps` call** to use
   bridge heaps.

#### Validation
- Build succeeds.
- Textured geometry renders correctly (check SRV binding).
- Sampler filtering matches baseline (compare mipmap quality, wrap vs clamp).
- Default (white) untextured quads still render.
- No descriptor heap corruption in D3D12 Debug Layer.

#### Risks
| Risk | Mitigation |
|------|------------|
| SRV indices shift if allocation order changes | Keep exact same allocation order: default texture first, then user textures. |
| `SetDescriptorHeaps` with bridge heaps must happen after `Core::Prepare()` | Call `DescriptorAllocator::SetDescriptorHeaps` first (for Core's heaps), then override with bridge shader-visible heaps. Or skip Core's non-shader-visible heaps and set bridge heaps directly. |

---

### Phase 3: Migrate Upload Buffers and Root Signature to Bridge

**Goal:** Move the upload ring buffers and root signature out of
`D3D12Backend` into `LegacyBridge`.

#### Steps

1. **Move `UploadRingBuffer` instances** into `LegacyBridge`.
   - Allocate `Core::GetBackBufferCount()` ring buffers on `Core::GetD3DDevice()`.
   - `GetUploadBuffer()` returns `m_uploadBuffers[Core::GetCurrentFrameIndex()]`.
   - Reset the current frame's buffer at the start of each frame (after
     `Core::Prepare()`).

2. **Move root signature creation** into `LegacyBridge::Init()`.
   - Use the `RootSignature` helper class or replicate the manual creation.
   - Same layout: CBV at b0, SRV table at t0, SRV table at t1, Sampler table.
   - Store as `LegacyBridge::GetRootSignature()`.

3. **Redirect consumer call sites:**
   | Old call | New call |
   |----------|----------|
   | `g_backend.GetUploadBuffer()` | `LegacyBridge::GetUploadBuffer()` |
   | `g_backend.GetRootSignature()` | `LegacyBridge::GetRootSignature()` |

4. **Move constant buffer upload** into bridge.
   The current `UploadConstants` writes to a fixed buffer. The OpenGL layer
   already uses the ring buffer for per-draw constant data
   (`uploadAndBindConstants`). The fixed CB in `D3D12Backend` is vestigial.
   Verify it's unused and remove it, or move it to the bridge.

5. **Remove these members from `D3D12Backend`.**

#### Validation
- Build succeeds.
- Constant buffer data reaches GPU correctly (check lighting, fog, material
  colors).
- No ring buffer corruption (watch for wraparound bugs when frame index
  sourcing changes from backend's `m_frameIndex` to `Core::GetCurrentFrameIndex()`).
- Multi-frame stability (run for 60+ seconds with no visual artifacts).

#### Risks
| Risk | Mitigation |
|------|------------|
| Frame index mismatch (backend's 0–1 vs Core's 0–N) | `Core::GetBackBufferCount()` controls ring buffer array size. Both default to 2, but verify. |
| Ring buffer reset timing | Must reset after `Core::Prepare()` resets the command allocator, before any draw calls. |

---

### Phase 4: Migrate PSO Cache to Bridge

**Goal:** Move the PSO cache out of `D3D12Backend` into `LegacyBridge`.

#### Steps

1. **Move `PSOKey`, `PSOKeyHash`, and the `unordered_map` cache** into
   `LegacyBridge`.

2. **Move `GetOrCreatePSO()`** into `LegacyBridge::GetOrCreatePSO()`.
   - Uses `Core::GetD3DDevice()` to create PSOs.
   - References `LegacyBridge::GetRootSignature()`.
   - References compiled shader headers (`VertexShader.h`, `PixelShader.h`).
   - Reads DSV format from `Core::GetDepthBufferFormat()`.
   - Reads RTV format from `Core::GetBackBufferFormat()`.

3. **Redirect the single consumer call site** in `opengl_directx.cpp`
   (`PrepareDrawState`):
   ```cpp
   auto* pso = LegacyBridge::GetOrCreatePSO(key);
   ```

4. **Remove PSO cache from `D3D12Backend`.**

5. **Consider future migration to `GraphicsPSO`.**
   This phase preserves the `PSOKey` approach for behavioral equivalence.
   A future optimization phase can replace it with named `GraphicsPSO` instances
   built from `GraphicsCommon` state descriptors.

#### Validation
- Build succeeds.
- All primitive types render correctly (points, lines, triangles, quads).
- Wireframe mode works.
- Blend state transitions (transparent objects) render correctly.
- Depth test enable/disable transitions work.
- No PSO creation failures in Debug Layer.

#### Risks
| Risk | Mitigation |
|------|------------|
| DSV format mismatch | Backend used `D24_UNORM_S8_UINT`; Core uses `D32_FLOAT`. Update PSO `DSVFormat`. |
| Compiled shader incompatibility with Core's device | Same device, same feature level — should be fine. Verify shader compilation target. |

---

### Phase 5: Migrate Frame Lifecycle and Eliminate g_backend

**Goal:** Replace all remaining `g_backend` calls with `Core` or
`LegacyBridge` equivalents. Delete `D3D12Backend`.

#### Steps

1. **Replace device/command list access in consumers:**
   | Old call | New call |
   |----------|----------|
   | `g_backend.GetDevice()` | `Graphics::Core::GetD3DDevice()` |
   | `g_backend.GetCommandList()` | `Graphics::Core::GetCommandList()` |
   | `g_backend.GetCurrentRTVHandle()` | `Graphics::Core::GetRenderTargetView()` |
   | `g_backend.GetCurrentDSVHandle()` | `Graphics::Core::GetDepthStencilView()` |
   | `g_backend.WaitForGpu()` | `Graphics::Core::WaitForGpu()` |

2. **Replace `Direct3DInit()`.** Inline into `ClientEngine::Startup()`:
   ```cpp
   Graphics::Core::Startup(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
   Graphics::Core::SetWindow(m_hwnd, width, height);
   LegacyBridge::Init();
   ```

3. **Replace `Direct3DShutdown()`.** Inline into `ClientEngine::Shutdown()`:
   ```cpp
   LegacyBridge::Shutdown();
   Graphics::Core::Shutdown();
   ```

4. **Replace `Direct3DSwapBuffers()`.** In `window_manager_directx.cpp`:
   ```cpp
   void WindowManager::Flip()
   {
     Graphics::Core::Present();
     Graphics::Core::Prepare();
     LegacyBridge::ResetUploadBuffer();
   }
   ```

5. **Replace `glFinish()`.** Change from `g_backend.WaitForGpu()` to
   `Graphics::Core::WaitForGpu()`.

6. **Remove `D3D12Backend::BeginFrame()` logic.**
   - Setting descriptor heaps → done by `LegacyBridge` after `Core::Prepare()`.
   - Setting root signature → done by `LegacyBridge` after `Core::Prepare()`.
   - Setting render targets → done in `PrepareDrawState` or a new
     `LegacyBridge::BeginFrame()` called after `Core::Prepare()`.

7. **Verify zero references to `g_backend` remain.**
   ```
   Select-String -Path NeuronClient\*.cpp,NeuronClient\*.h -Pattern "g_backend"
   ```

8. **Delete `d3d12_backend.h` and `d3d12_backend.cpp`.**
   Remove from the `NeuronClient.vcxproj`.

9. **Remove `#include "d3d12_backend.h"` from all files.**

#### Validation
- Build succeeds with `d3d12_backend.*` deleted.
- Full game loop runs (menu → game → back to menu).
- Visual output matches Phase 0 baseline screenshots.
- D3D12 Debug Layer clean.
- PIX capture shows single device, correct resource states.
- Memory usage reduced (one fewer device, swap chain, heap set).
- No leaked COM objects on shutdown (check `IDXGIDebug1::ReportLiveObjects`).

#### Risks
| Risk | Mitigation |
|------|------------|
| Missing call site causes linker error | Grep for `g_backend` and `D3D12Backend` before deleting. |
| Core's command list type mismatch (`ID3D12GraphicsCommandList7` vs `ID3D12GraphicsCommandList`) | The OpenGL layer only uses base `ID3D12GraphicsCommandList` methods. `7` inherits from the base. Cast or template as needed. |
| `Core::Prepare()` sets Core's descriptor heaps, then bridge overrides with shader-visible ones | Ensure bridge's `SetDescriptorHeaps` comes last before draws. |

---

### Phase 6: Post-Migration Cleanup and Optimization (Optional)

**Goal:** Leverage `Core`'s modern infrastructure to improve the legacy bridge.

#### Steps (Priority Order)

1. **Replace manual barriers** in OpenGL layer with
   `ResourceStateTracker::TransitionResource()`.
   - `opengl_directx.cpp`: lines 1217-1223 (texture upload barrier).
   - `opengl_directx_dlist.cpp`: lines 44-58 (VBO upload barriers).
   - `ogl_extensions_directx.cpp`: lines 109-122 (VBO upload barriers).

2. **Replace `PSOKey` cache with named `GraphicsPSO` instances.**
   - Pre-create the ~20 most common state combinations using
     `GraphicsCommon` descriptors.
   - Fall back to dynamic creation for rare combinations.

3. **Replace manual root signature with `RootSignature` helper.**
   - Use `RootParameter` helpers for cleaner code.

4. **Migrate shader-visible heaps to `DescriptorAllocator`.**
   - Requires extending `DescriptorAllocator` to support shader-visible heaps
     or using a `DynamicDescriptorHeap` pattern.

5. **Consider replacing `UploadRingBuffer` with a proper `LinearAllocator`**
   from Core infrastructure (if one exists or is added).

6. **Migrate `CustomVertex` to `VertexTypes.h` pattern.**
   - Define a `VertexPositionNormalColorTexture` in `VertexTypes.h` with
     `constexpr D3D12_INPUT_ELEMENT_DESC` and `D3D12_INPUT_LAYOUT_DESC`.

#### Validation
- Each sub-step validated independently.
- Full regression test against baseline screenshots.

---

## 4. Dependency Graph (Phase Ordering)

```
Phase 0  (Prep)
   │
   ▼
Phase 1  (Unify Device)
   │
   ├──────────────┐
   ▼              ▼
Phase 2        Phase 3
(Descriptors)  (Upload + RootSig)
   │              │
   └──────┬───────┘
          ▼
       Phase 4  (PSO Cache)
          │
          ▼
       Phase 5  (Delete Backend)
          │
          ▼
       Phase 6  (Cleanup)
```

**Phases 2 and 3 are independent and can be done in parallel or either order.**
All other phases are sequential.

---

## 5. Validation Strategy Summary

| Phase | Validation Method |
|-------|-------------------|
| 0 | Build. Baseline screenshots. |
| 1 | Build. Single device in PIX. Debug Layer clean. Visual match. |
| 2 | Build. Texture rendering correct. Sampler filtering correct. |
| 3 | Build. Lighting/fog/materials correct. 60s stability. |
| 4 | Build. All topology types. Blend/depth transitions. |
| 5 | Build (backend deleted). Full game loop. Visual match. No leaks. |
| 6 | Build. Per-step regression. |

**Automated validation** (future): Add a `--screenshot` command-line flag that
renders one frame, saves it, and exits. Compare against baselines with a
pixel-difference threshold.

---

## 6. Risk Register

| # | Risk | Likelihood | Impact | Mitigation |
|---|------|-----------|--------|------------|
| 1 | Back-buffer format mismatch | Medium | High (wrong colors) | Align format in `Core::Startup()` call |
| 2 | Dual fence signal causes GPU hang | Medium | High | Remove backend fence in Phase 1 |
| 3 | Descriptor heap binding order | Medium | Medium | Bridge sets heaps after `Core::Prepare()` |
| 4 | DSV format mismatch in PSO | Medium | High (validation error) | Read from `Core::GetDepthBufferFormat()` |
| 5 | Ring buffer frame index mismatch | Low | High (data corruption) | Use `Core::GetCurrentFrameIndex()` consistently |
| 6 | Command list type cast (`7` → base) | Low | Low | Implicit upcast; legacy methods are on base interface |
| 7 | Regression in display list replay | Low | Medium | Test with scenes heavy on display lists |
| 8 | VBO extension buffer barriers | Low | Medium | Use `ResourceStateTracker` in Phase 6 |

---

## 7. Estimated Effort

| Phase | Lines Changed (est.) | Risk Level | Duration (est.) |
|-------|---------------------|------------|-----------------|
| 0 | ~100 (new file, no behavior change) | None | 0.5 day |
| 1 | ~200 (backend gutting + wiring) | High | 1–2 days |
| 2 | ~150 (heap + sampler + texture move) | Medium | 1 day |
| 3 | ~100 (upload buf + root sig move) | Medium | 0.5–1 day |
| 4 | ~80 (PSO cache move) | Low | 0.5 day |
| 5 | ~120 (redirect + delete) | Medium | 1 day |
| 6 | ~200 (cleanup, optional) | Low | 1–2 days |
| **Total** | **~950** | | **5–8 days** |

---

## 8. Key Design Decisions

1. **Bridge pattern over inline migration.** Extracting backend responsibilities
   into `LegacyBridge` allows each phase to be a contained, testable unit
   rather than a "big bang" rewrite across 4 files.

2. **Preserve `PSOKey` caching initially.** The existing hash-based PSO cache
   works and matches the OpenGL state machine model. Migrating to named PSOs
   is a separate optimization concern.

3. **Core owns the frame lifecycle.** `Prepare()` / `Present()` is the
   canonical frame bracket. The bridge hooks into it, not the other way around.

4. **Single `#include` change strategy.** Consumer files switch from
   `#include "d3d12_backend.h"` to `#include "LegacyBridge.h"` (which itself
   includes `GraphicsCore.h`). This minimizes header churn.

5. **Format alignment at startup.** Changing `Core::Startup()` parameters is
   simpler and safer than changing 500+ lines of format assumptions in the
   OpenGL layer.
