# D3D12 Architecture Refactor Plan

## Current State

### Ownership Map

| Resource | Owner | Notes |
|---|---|---|
| Device (`ID3D12Device10`) | `Graphics::Core` | Singleton |
| Command Queue | `Graphics::Core` | |
| Command Allocators (per frame) | `Graphics::Core` | |
| Command List | `Graphics::Core` | |
| Fence + Event | `Graphics::Core` | |
| Swap Chain | `Graphics::Core` | |
| Render Targets (back buffers) | `Graphics::Core` | `GpuResource` wrappers |
| Depth Stencil | `Graphics::Core` | |
| RTV / DSV descriptors | `DescriptorAllocator` | CPU-only heaps |
| SRV/CBV descriptor heap (shader-visible) | `DescriptorAllocator` | 256 slots — **unused**, overridden every frame |
| Sampler descriptor heap (shader-visible) | `DescriptorAllocator` | 256 slots — **unused**, overridden every frame |
| SRV/CBV descriptor heap (shader-visible) | `D3D12Backend` | 1024 slots — **actually bound** |
| Sampler descriptor heap (shader-visible) | `D3D12Backend` | 24 slots — **actually bound** |
| Root Signature | `D3D12Backend` | 5 params: 2 CBV + 2 SRV tables + 1 sampler table |
| PSO Cache | `D3D12Backend` | Keyed by `PSOKey` (rasterizer, depth, blend, topology) |
| Upload Ring Buffers (per frame) | `D3D12Backend` | 64 MB each |
| Default White Texture | `D3D12Backend` | 1×1 RGBA |
| Static Samplers (8 combos) | `D3D12Backend` | In sampler heap |
| Common state objects | `GraphicsCommon` | Rasterizer, blend, depth descs; sampler descs |

### Init Sequence

```
ClientEngine::Startup()
  ├─ Graphics::Core::Startup()           → device, queue, allocators, fence
  │    └─ DescriptorAllocator::Create()  → SRV(256), Sampler(256), RTV, DSV heaps
  ├─ Graphics::Core::SetWindow()         → swap chain, render targets, depth stencil
  ├─ Graphics::Core::Prepare()           → opens command list, sets DescriptorAllocator heaps
  └─ Direct3DInit()  (opengl_directx.cpp)
       └─ g_backend.Init()
            ├─ CreateDescriptorHeaps()   → SECOND SRV(1024) + sampler(24) heaps
            ├─ CreateRootSignature()     → shared 5-param root sig
            ├─ CreateUploadBuffer()      → per-frame ring buffers
            ├─ CreateDefaultTexture()    → 1×1 white
            ├─ CreateSamplers()          → 8 static combos
            └─ BeginFrame()              → OVERRIDES heaps, sets root sig, render targets
```

### Per-Frame Sequence

```
Renderer::Render()
  └─ WindowManager::Flip()
       └─ Direct3DSwapBuffers()
            └─ g_backend.EndFrame()
                 ├─ Transition RT → PRESENT
                 ├─ Core::Present()           → close cmd list, execute, present, move to next frame
                 ├─ Core::Prepare()           → reset allocator/cmd list, set DescriptorAllocator heaps ← wasted
                 ├─ Reset upload ring buffer
                 └─ BeginFrame()              → OVERRIDE heaps again, set root sig, render targets
```

### Shutdown Sequence

```
ClientEngine::Shutdown()
  ├─ GameMain::Shutdown()
  ├─ Direct3DShutdown()
  │    ├─ g_backend.WaitForGpu()
  │    ├─ Release textures, display lists, VBOs, vertices
  │    ├─ TreeRenderer::Shutdown()
  │    └─ g_backend.Shutdown()              → PSO cache, root sig, heaps, upload buffers
  └─ Graphics::Core::Shutdown()             → fence, swap chain, device
```

### Consumer Map (`g_backend` references)

| File | Count | What it uses |
|---|---|---|
| `opengl_directx.cpp` | ~35 | Upload buffer, PSO cache, cmd list, SRV/sampler heaps, root sig, default texture, BeginFrame/EndFrame |
| `tree_renderer.cpp` | ~10 | Upload buffer, PSO cache, cmd list, SRV heaps, root sig |
| `ogl_extensions_directx.cpp` | ~4 | SRV heap (texture creation), upload buffer |
| `opengl_directx_dlist.cpp` | ~6 | Upload buffer, cmd list |
| `Starstrike/renderer.cpp` | **0** | Pure `gl*` calls — no D3D12 awareness |

---

## Problem

`D3D12Backend` mixes two concerns:

1. **Engine infrastructure** — descriptor heaps, upload buffers, frame orchestration — things any renderer needs.
2. **OpenGL translation specifics** — PSO cache keyed by GL state, sampler index mapping, default texture for GL's "texturing disabled" concept.

This creates issues:

- **Dual frame orchestration.** `BeginFrame`/`EndFrame` duplicate frame lifecycle that `Core` already owns. `Core::Prepare()` sets descriptor heaps via `DescriptorAllocator::SetDescriptorHeaps()`, then `BeginFrame()` immediately overrides them — a wasted GPU command every frame.
- **Dual shader-visible heaps.** `DescriptorAllocator::Create()` allocates shader-visible SRV/CBV (256 slots) and Sampler (256 slots) heaps that are never actually used for rendering. `D3D12Backend` creates a second set (1024 SRV + 24 sampler) and binds those instead. Two heaps exist for each type; only the backend's are functional.
- **SRV slot leak.** `AllocateSRVSlot` is a monotonic bump pointer (`m_srvNextFreeSlot++`). When textures are destroyed via `glDeleteTextures`, their SRV slots are never reclaimed. A long session will exhaust the 1024-slot heap.
- **Modern renderer coexistence.** New renderers (`SpriteBatch`, future systems) would need to bypass or coexist with the backend's heaps and root signature. The backend should not be the gatekeeper for shared GPU infrastructure.
- **Device-lost gap.** `ClientEngine::OnDeviceLost()` and `OnDeviceRestored()` are empty stubs (NeuronClient.cpp:304–308). Backend resources (PSO cache, default texture, upload buffers) are not recreated after device lost.
- **Device-lost destroys all heaps.** `ReleaseWindowSizeDependentResources()` calls `DescriptorAllocator::DestroyAll()`, which destroys all descriptor heaps including shader-visible ones. This method is only called from `Core::Shutdown()` and `Core::HandleDeviceLost()` — not from a normal window resize. After the refactor unifies the heaps, a device-lost event will invalidate every texture SRV index. The `IDeviceNotify` path must recreate them.
- **RTV/DSV descriptor exhaustion on resize (pre-existing).** `CreateWindowSizeDependentResources()` bump-allocates new RTV/DSV descriptors via `AllocateDescriptor()` on every call, but the RTV heap has only `backBufferCount` slots (2–3). A second window resize overflows the heap because old descriptors are never freed. This is a pre-existing bug that becomes more visible after heap unification.

---

## Plan

### Phase 1 — Absorb Frame Orchestration into `Core`

**Goal:** `Core` owns the full frame lifecycle. Remove `BeginFrame`/`EndFrame` from `D3D12Backend`.

**Design principle:** `Core` must not contain rendering policy (root signatures, render target binding). Instead, introduce a lightweight listener so subsystems can inject per-frame setup without `Core` knowing about them.

**Changes:**

#### 1a. Remove the wasted `SetDescriptorHeaps` call

`Core::Prepare()` currently calls `DescriptorAllocator::SetDescriptorHeaps()` which is immediately overridden by `BeginFrame()`. Remove that line. The listener (below) will set the correct heaps.

**Files touched:** `GraphicsCore.cpp`.

#### 1b. Add `IFrameListener` callback to `Core`

Define `IFrameListener` in its own header to avoid widening `GraphicsCore.h` (which is in the PCH chain via `pch.h` → `NeuronClient.h` → `GraphicsCore.h` — any change triggers a full NeuronClient rebuild):

```cpp
// FrameListener.h
#pragma once
struct ID3D12GraphicsCommandList;

namespace Neuron::Graphics
{
    class IFrameListener
    {
    public:
        virtual void OnFrameBegin(ID3D12GraphicsCommandList* cmdList) = 0;
        virtual ~IFrameListener() = default;
    };
}
```

In `GraphicsCore.h`, forward-declare `IFrameListener` and add registration methods. A forward declaration is sufficient for `std::vector<IFrameListener*>`:

```cpp
// GraphicsCore.h
namespace Neuron::Graphics { class IFrameListener; }  // forward decl

class Core
{
public:
    // ...existing code...
    void RegisterFrameListener(IFrameListener* listener);
    void UnregisterFrameListener(IFrameListener* listener);
    // ...existing code...
private:
    // ...existing code...
    std::vector<IFrameListener*> m_frameListeners;
};
```

`GraphicsCore.cpp` includes `FrameListener.h` and calls `OnFrameBegin` on each listener at the end of `Prepare()`, after the allocator reset and RT transition are complete. Registration order defines call order.

`UnregisterFrameListener` is required for shutdown safety — without it, `Core::Prepare()` would call through a dangling pointer if a listener is destroyed before `Core`.

**Why not bake heap/root-sig binding into `Core::Prepare()`:** The root signature and render target binding are rendering policy specific to the OpenGL translation layer. If `Core` sets them directly, we recreate the coupling in a different file. `SpriteBatch` and future renderers need different root signatures. The listener pattern keeps `Core` as pure infrastructure.

**Files touched:** new `FrameListener.h`, `GraphicsCore.h`, `GraphicsCore.cpp`.

#### 1c. Inline `EndFrame`/`BeginFrame` into `Direct3DSwapBuffers`, register as listener, remove trivial wrappers

`D3D12Backend` implements `IFrameListener`. Its `OnFrameBegin` does what `BeginFrame()` currently does: set descriptor heaps, root signature, and render targets. `Direct3DSwapBuffers()` calls `Core` directly for present/prepare:

```cpp
void Direct3DSwapBuffers()
{
    auto& core = Graphics::Core::Get();

    // Transition RT → PRESENT
    core.GetGpuResourceStateTracker()->TransitionResource(core.GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, true);

    core.Present();   // close, execute, present, move to next frame
    core.Prepare();   // reset allocator/cmdlist, transition RT, invoke listeners

    // Reset upload buffer for new frame (interim — Phase 3a absorbs into Core::Prepare())
    g_backend.GetUploadBuffer().Reset();
}
```

Remove `BeginFrame()` and `EndFrame()` from `D3D12Backend`.

Also remove the trivial one-liner wrappers that just delegate to `Core` — there is no reason to keep them now that `Direct3DSwapBuffers` calls `Core` directly:
- `D3D12Backend::WaitForGpu()` → callers use `Graphics::Core::Get().WaitForGpu()`
- `D3D12Backend::GetCurrentRTVHandle()` → callers use `Graphics::Core::Get().GetRenderTargetView()`
- `D3D12Backend::GetCurrentDSVHandle()` → callers use `Graphics::Core::Get().GetDepthStencilView()`

Update `glClear()` (uses `GetCurrentRTVHandle`/`GetCurrentDSVHandle`) and `Direct3DShutdown()` (uses `WaitForGpu`) to call `Core` directly.

**Files touched:** `d3d12_backend.h`, `d3d12_backend.cpp`, `opengl_directx.cpp`.

#### 1d. Update `D3D12Backend::Init()` to register as listener and call `OnFrameBegin` once

Replace the `BeginFrame()` call in `Init()` with listener registration and an initial `OnFrameBegin` call.

`D3D12Backend::Shutdown()` must call `Core::Get().UnregisterFrameListener(this)` before releasing resources. This ensures `Core::Prepare()` never dispatches to a destroyed listener.

**Files touched:** `d3d12_backend.cpp`.

**Verification:** Build and run. Frame should look identical — same heaps, same root sig, same render targets, just driven by the callback instead of the wrapper.

---

### Phase 2 — Unify Shader-Visible Descriptor Heaps

**Goal:** Single source of truth for shader-visible descriptor heaps. Eliminate the duplicate heaps.

**Design principle:** Don't create a new parallel API. Resize the existing `DescriptorAllocator` heaps to match the backend's needs and route all allocations through them.

**Changes:**

#### 2a. Resize `DescriptorAllocator` heaps and delete backend's duplicates

In `DescriptorAllocator::Create()`, change the SRV/CBV heap from 256 → 1024 and the Sampler heap from 256 → 24. These match the backend's current sizes. Delete `D3D12Backend::CreateDescriptorHeaps()` and all of `m_srvCbvHeap`, `m_samplerHeap`, `m_srvDescriptorSize`, `m_samplerDescriptorSize` from the backend.

```cpp
// DescriptorHeap.cpp — DescriptorAllocator::Create()
static constexpr uint32_t MAX_SRV_DESCRIPTORS = 1024;
static constexpr uint32_t MAX_SAMPLER_DESCRIPTORS = 24;

sm_descriptorHeapPool[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Create(
    L"CBV_SRV_UAV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAX_SRV_DESCRIPTORS);
sm_descriptorHeapPool[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Create(
    L"Sampler Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, MAX_SAMPLER_DESCRIPTORS);
```

**Files touched:** `DescriptorHeap.h`, `DescriptorHeap.cpp`, `d3d12_backend.h`, `d3d12_backend.cpp`.

#### 2b. Redirect `AllocateSRVSlot` / `GetSRVGPUHandle` and sampler allocation through `DescriptorAllocator`

`D3D12Backend::AllocateSRVSlot` and `GetSRVGPUHandle` become thin wrappers over `DescriptorAllocator::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)`. The `DescriptorHandle` returned already carries both CPU and GPU handles, so `GetSRVGPUHandle` is just a cast.

`D3D12Backend::OnFrameBegin` now calls `DescriptorAllocator::SetDescriptorHeaps()` (which binds the unified heaps) instead of binding its own.

**Sampler migration:** `CreateSamplers()` currently computes handles manually from `m_samplerHeap->GetCPUDescriptorHandleForHeapStart()` + `m_samplerDescriptorSize`. After 2a deletes those members, this code must migrate to `DescriptorAllocator::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, NUM_STATIC_SAMPLERS)`. Store the returned `DescriptorHandle` as `m_samplerBaseHandle` for GPU-side lookups.

**Sampler GPU handle accessor replacement:** `bindTexturesAndSamplers()` (opengl_directx.cpp) and `TreeRenderer::DrawTree()` (tree_renderer.cpp) currently compute sampler GPU handles via:

```cpp
D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = g_backend.GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart();
samplerHandle.ptr += samplerIdx * g_backend.GetSamplerDescriptorSize();
```

After 2a deletes `GetSamplerHeap()` and `GetSamplerDescriptorSize()`, replace with:

```cpp
D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = g_glState.GetSamplerBaseHandle();
samplerHandle.ptr += samplerIdx * DescriptorAllocator::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
```

This requires a new `DescriptorAllocator::GetDescriptorSize(Type)` static accessor that delegates to `sm_descriptorHeapPool[Type].GetDescriptorSize()` (`DescriptorHeap` already has `GetDescriptorSize()`).

**Files touched:** `d3d12_backend.h`, `d3d12_backend.cpp`, `DescriptorHeap.h`, `opengl_directx.cpp`, `tree_renderer.cpp`.

#### 2c. Add descriptor free-list to `DescriptorHeap` (required)

The current allocator is a monotonic bump pointer — slots are never reclaimed when textures are destroyed. `glDeleteTextures()` releases the `ID3D12Resource` and marks the slot invalid but never returns the SRV index. With a 1024-slot heap, a long session that loads/unloads map textures will exhaust it. This is a functional bug, not a nice-to-have.

Add a free-list to `DescriptorHeap` using the existing `GetOffsetOfHandle()` method to recover the slot index:

```cpp
class DescriptorHeap
{
public:
    // ...existing code...
    DescriptorHandle Alloc(uint32_t Count = 1);  // checks free-list before bumping
    void Free(DescriptorHandle handle);           // returns slot to free-list

private:
    // ...existing code...
    std::vector<uint32_t> m_FreeList;   // Indices of freed slots
    uint32_t m_BumpRemaining = 0;       // Contiguous slots left at bump frontier
};

// DescriptorHeap.cpp  —  Create() must initialize m_BumpRemaining = MaxCount.

void DescriptorHeap::Free(DescriptorHandle handle)
{
    DEBUG_ASSERT(handle.IsShaderVisible() && "Free() only valid for shader-visible heaps");
    DEBUG_ASSERT(ValidateHandle(handle));
    uint32_t index = GetOffsetOfHandle(handle);
    m_FreeList.push_back(index);
    m_NumFreeDescriptors++;
}

DescriptorHandle DescriptorHeap::Alloc(uint32_t Count)
{
    // Single-slot requests can be satisfied from the free-list
    if (Count == 1 && !m_FreeList.empty())
    {
        uint32_t index = m_FreeList.back();
        m_FreeList.pop_back();
        m_NumFreeDescriptors--;
        return (*this)[index];
    }
    // Multi-slot or empty free-list: bump-allocate.
    // m_BumpRemaining tracks contiguous space at the frontier;
    // m_NumFreeDescriptors may include scattered free-list slots.
    DEBUG_ASSERT_TEXT(m_BumpRemaining >= Count, "Descriptor Heap out of contiguous space.");
    DescriptorHandle ret = m_NextFreeHandle;
    m_NextFreeHandle += Count * m_DescriptorSize;
    m_BumpRemaining -= Count;
    m_NumFreeDescriptors -= Count;
    return ret;
}
```

**Why `m_BumpRemaining`:** The original `HasAvailableSpace(Count)` checks `Count <= m_NumFreeDescriptors`, but after adding the free-list, `m_NumFreeDescriptors` conflates scattered free-list slots with contiguous bump-frontier slots. After the heap is fully bump-allocated and some slots are freed, `HasAvailableSpace(8)` would return true even though the bump pointer has zero contiguous space. Phase 2b calls `Allocate(SAMPLER, NUM_STATIC_SAMPLERS)` — a batch of 8 — so this is a real concern. `m_BumpRemaining` tracks only contiguous space at the frontier and is decremented only by the bump path.

**`DescriptorAllocator::Free` plumbing:** Add a static `DescriptorAllocator::Free(D3D12_DESCRIPTOR_HEAP_TYPE Type, DescriptorHandle handle)` that delegates to `sm_descriptorHeapPool[Type].Free(handle)`. This mirrors the existing `Allocate` path.

**`TextureResource` migration — store `DescriptorHandle` instead of `UINT srvIndex`:** Currently `TextureResource` (opengl_directx.cpp) stores `UINT srvIndex`. Converting a UINT back to a `DescriptorHandle` for `Free()` requires accessing the underlying heap's `operator[]`, which `DescriptorAllocator` does not expose. The cleanest fix: change `TextureResource.srvIndex` from `UINT` to `DescriptorHandle`. Benefits:

- `glDeleteTextures` calls `DescriptorAllocator::Free(CBV_SRV_UAV, tex.srvHandle)` directly — no index→handle roundtrip.
- `bindTexturesAndSamplers` and `GetTextureSRVGPUHandle` use the stored handle's built-in `operator D3D12_GPU_DESCRIPTOR_HANDLE()` instead of calling `GetSRVGPUHandle(index)`, which can be removed.
- `AllocateSRVSlot` returns a `DescriptorHandle` (already planned in 2b); callers store it directly.

```cpp
// opengl_directx.cpp — TextureResource (updated)
struct TextureResource
{
    com_ptr<ID3D12Resource> resource;
    DescriptorHandle srvHandle;  // was: UINT srvIndex
    UINT width = 0, height = 0;
    bool valid = false;
};

// opengl_directx.cpp — glDeleteTextures()
if (texIdx < s_textureResources.size() && s_textureResources[texIdx].valid)
{
    // Reclaim the SRV slot
    DescriptorAllocator::Free(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        s_textureResources[texIdx].srvHandle);
    s_textureResources[texIdx].resource = nullptr;
    s_textureResources[texIdx].valid = false;
}
```

**Files touched:** `DescriptorHeap.h`, `DescriptorHeap.cpp`, `opengl_directx.cpp`.

#### 2d. Fix `DestroyAll` scope and RTV/DSV descriptor reuse

**Clarification:** `ReleaseWindowSizeDependentResources()` is only called from `Core::Shutdown()` and `Core::HandleDeviceLost()` — **not** from a normal window resize. `WindowSizeChanged()` calls `CreateWindowSizeDependentResources()` directly without releasing first. So `DestroyAll` is not the resize problem.

Two issues must be fixed:

**Issue 1 — `DestroyAll` scope for device-lost.** `HandleDeviceLost()` calls `ReleaseWindowSizeDependentResources()` → `DestroyAll()`, then recreates everything. After heap unification, `DestroyAll` destroys the SRV/CBV heap containing all texture descriptors. This is correct for device-lost (the device is gone, all GPU resources are invalid) but `ReleaseWindowSizeDependentResources` should not destroy shader-visible heaps during a normal shutdown-then-recreate cycle.

**Fix:** Split `DestroyAll` into `DestroyWindowSizeDependent()` (RTV/DSV only) and `DestroyAll()` (everything, for shutdown/device-lost). `ReleaseWindowSizeDependentResources` calls `DestroyWindowSizeDependent()`. `DestroyAll()` is called at the start of `ReleaseDeviceResources()` (alongside the existing `DestroyCommonState()`), since device-level resources include the SRV/Sampler heaps.

Exact call sequences after the split:

```
Shutdown():
  WaitForGpu()
  ReleaseWindowSizeDependentResources()  → DestroyWindowSizeDependent() (RTV/DSV only)
  ReleaseDeviceResources()               → DestroyAll() + DestroyCommonState() + release fence/device

HandleDeviceLost():
  OnDeviceLost()
  ReleaseWindowSizeDependentResources()  → DestroyWindowSizeDependent()
  ReleaseDeviceResources()               → DestroyAll() + release fence/device
  CreateDeviceResources()                → DescriptorAllocator::Create() (fresh heaps)
  CreateWindowSizeDependentResources()   → allocate new RTV/DSV descriptors
  OnDeviceRestored()
```

```cpp
// DescriptorHeap.h
static void DestroyWindowSizeDependent();  // RTV + DSV only
static void DestroyAll();                  // Everything (shutdown / device-lost)
```

**Issue 2 — RTV/DSV descriptor exhaustion on resize (pre-existing).** `CreateWindowSizeDependentResources()` bump-allocates new RTV descriptors via `AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)` each time it runs, but the RTV heap has only `backBufferCount` slots. A second resize overflows the heap.

**Fix:** `Core::m_rtvDescriptors` is `std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 3>` initialized to `{}` (all zeros). `m_dsvDescriptor` is likewise zero-initialized. Guard allocation with an already-allocated check:

```cpp
// CreateWindowSizeDependentResources — RTV loop
if (m_rtvDescriptors[n].ptr == 0)
    m_rtvDescriptors[n] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].GetResource(), &rtvDesc, m_rtvDescriptors[n]);

// CreateWindowSizeDependentResources — DSV
if (m_dsvDescriptor.ptr == 0)
    m_dsvDescriptor = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
m_d3dDevice->CreateDepthStencilView(m_depthStencil.GetResource(), &dsvDesc, m_dsvDescriptor);
```

After `HandleDeviceLost()` destroys all heaps via `DestroyAll()`, these handles become stale. `ReleaseDeviceResources()` (or `HandleDeviceLost` before `CreateWindowSizeDependentResources`) must reset them to `{}` so the guard re-allocates:

```cpp
// ReleaseDeviceResources() — after DestroyAll()
m_rtvDescriptors.fill({});
m_dsvDescriptor = {};
```

**Files touched:** `DescriptorHeap.h`, `DescriptorHeap.cpp`, `GraphicsCore.cpp`.

**Verification:** Build and run. Textures render correctly. Resize window — textures survive, no RTV heap overflow. Create and destroy textures in a loop — SRV heap doesn't exhaust.

---

### Phase 3 — Extract OpenGL-Specific State from `D3D12Backend`

**Goal:** `D3D12Backend` becomes a thin OpenGL translation helper (`OpenGLTranslationState`) with no engine infrastructure ownership.

**Changes:**

#### 3a. Move `UploadRingBuffer` to its own header and `Core` ownership

`UploadRingBuffer` is general-purpose infrastructure used by 4 files. Move it out of `d3d12_backend.h` into `UploadRingBuffer.h` to avoid widening `GraphicsCore.h` (build cascade mitigation — `GraphicsCore.h` is pulled in widely via `pch.h`).

`Core` owns the per-frame upload buffers. Expose a free function that encapsulates frame-index logic:

```cpp
// UploadRingBuffer.h
namespace Neuron::Graphics
{
    class UploadRingBuffer { /* ...existing code... */ };

    // Returns the upload buffer for the current frame. Hides frame-index bookkeeping.
    UploadRingBuffer& GetCurrentUploadBuffer();
}
```

All consumers change from `g_backend.GetUploadBuffer()` to `Graphics::GetCurrentUploadBuffer()`.

**Init sequence:** Create the upload buffers at the end of `Core::CreateDeviceResources()`, after the command list and fence are created but before any consumer runs. This is necessary because `D3D12Backend::Init()` → `CreateDefaultTexture()` uses the upload buffer to copy the white pixel to the GPU. The buffers must exist before `Direct3DInit()` is called.

`Core::Prepare()` resets the current frame's upload buffer after `MoveToNextFrame` (removing this responsibility from `Direct3DSwapBuffers`).

**Note:** `DescriptorAllocator::Create()` also creates a CPU-only SRV heap at pool index `D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES` for `_forceCpu=true` allocations. This heap is unaffected by the Phase 2 changes. Give each heap its own size constant in Phase 2a instead of sharing `NUM_DESCRIPTORS_PER_HEAP`.

**Files touched:** new `UploadRingBuffer.h`, `d3d12_backend.h`, `d3d12_backend.cpp`, `GraphicsCore.h`, `GraphicsCore.cpp`, `opengl_directx.cpp`, `tree_renderer.cpp`, `ogl_extensions_directx.cpp`, `opengl_directx_dlist.cpp`.

#### 3b. Rename `D3D12Backend` → `OpenGLTranslationState`

Rename the class, file, and global:

| Before | After |
|---|---|
| `D3D12Backend` | `OpenGLTranslationState` |
| `d3d12_backend.h` / `.cpp` | `OpenGLTranslationState.h` / `.cpp` |
| `g_backend` | `g_glState` |

What remains in `OpenGLTranslationState`:
- Root signature (specific to the uber-shader)
- PSO cache (keyed by GL state)
- Default white texture (GL "texturing disabled" concept)
- Static samplers (GL→sampler-index mapping)
- `IFrameListener` implementation (sets root sig + render targets)

What has been removed:
- Descriptor heaps → `DescriptorAllocator`
- Upload ring buffers → `Core` / `UploadRingBuffer.h`
- `BeginFrame` / `EndFrame` → inlined into callback + `Direct3DSwapBuffers`
- `GetDevice` / `GetCommandList` / `WaitForGpu` / `GetCurrentRTVHandle` / `GetCurrentDSVHandle` → consumers call `Core` directly (wrappers removed in Phase 1c; call sites migrated in 3b)

**Explicit call-site migration (sub-step of 3b):** Replace all remaining `g_backend.GetDevice()` → `Graphics::Core::Get().GetD3DDevice()` and `g_backend.GetCommandList()` → `Graphics::Core::Get().GetCommandList()` across consumer files. Approximate call sites:

| File | `GetDevice()` | `GetCommandList()` |
|---|---|---|
| `opengl_directx.cpp` | ~2 | ~10+ |
| `tree_renderer.cpp` | 1 | 1 |
| `ogl_extensions_directx.cpp` | 1 | 1 |
| `opengl_directx_dlist.cpp` | 0 | ~4 |

#### 3c. Decide `TreeRenderer` root signature coupling

`TreeRenderer` currently uses the backend's shared root signature (`g_backend.GetRootSignature()`). It shares the same 5-param layout (2 CBV + 2 SRV tables + 1 sampler table) because its shaders match.

**Decision:** Keep shared for now. `TreeRenderer` accesses the root signature via `g_glState.GetRootSignature()`. If tree shaders diverge later, give `TreeRenderer` its own root signature at that point. Document the coupling:

```cpp
// tree_renderer.cpp — Init()
// NOTE: shares root signature with OpenGL translation layer.
// If tree shaders diverge (new params, different SRV layout),
// create a dedicated root sig here instead.
m_pso.SetRootSignature(OpenGLD3D::g_glState.GetRootSignature());
```

**Files touched:** `d3d12_backend.h` → `OpenGLTranslationState.h`, `d3d12_backend.cpp` → `OpenGLTranslationState.cpp`, `opengl_directx.cpp`, `tree_renderer.cpp`, `ogl_extensions_directx.cpp`, `opengl_directx_dlist.cpp`.

**Verification:** Build all configurations. Run game. All rendering identical.

---

## Dependency Graph (After Refactor)

```
Starstrike/renderer.cpp
    │  (gl* calls)
    ▼
opengl_directx.cpp  ──►  OpenGLTranslationState (PSO cache, root sig, default tex, samplers)
    │                          │  implements IFrameListener
    │                          ▼
    └─────────────────►  Graphics::Core
                              ├─ Device, cmd list, swap chain, fence
                              ├─ Frame lifecycle (Prepare → listeners → Present)
                              ├─ DescriptorAllocator (unified SRV/CBV + Sampler heaps)
                              └─ UploadRingBuffer (per-frame, via GetCurrentUploadBuffer())
```

- `Renderer` stays untouched — pure game-layer rendering via `gl*` calls.
- `SpriteBatch` and future renderers talk directly to `Core` and register their own `IFrameListener`.
- `OpenGLTranslationState` holds only GL→D3D12 mapping concerns.

---

## Ownership Map (After Refactor)

| Resource | Owner | Notes |
|---|---|---|
| Device, Queue, Fence, Swap Chain | `Core` | Unchanged |
| Command Allocators / List | `Core` | Unchanged |
| Render Targets / Depth Stencil | `Core` | Unchanged |
| RTV / DSV descriptors | `DescriptorAllocator` | Unchanged |
| SRV/CBV heap (shader-visible, 1024) | `DescriptorAllocator` | **Moved from backend** |
| Sampler heap (shader-visible, 24) | `DescriptorAllocator` | **Moved from backend** |
| Upload Ring Buffers (per frame) | `Core` | **Moved from backend** |
| Frame listener dispatch | `Core` | **New** |
| Root Signature | `OpenGLTranslationState` | Renamed, same owner |
| PSO Cache | `OpenGLTranslationState` | Renamed, same owner |
| Default White Texture | `OpenGLTranslationState` | Renamed, same owner |
| Static Samplers (8 combos) | `OpenGLTranslationState` | Renamed, same owner |

---

## Known Risks & Gaps

### Device-Lost Recovery

`ClientEngine::OnDeviceLost()` and `OnDeviceRestored()` are empty stubs. After this refactor, resources are split across `Core` (upload buffers, descriptor heaps) and `OpenGLTranslationState` (PSO cache, default texture, root signature). Neither set is recreated on device lost.

**Phase 1 escalates the risk.** Before this refactor, device-lost is a silent leak — stale resources are never freed. After Phase 1, `Core::Prepare()` automatically invokes frame listeners. If `HandleDeviceLost()` recreates Core resources (device, heaps, command list) and the caller then calls `Prepare()`, listeners fire — but the listener's resources (root signature, PSO cache, samplers, default texture) haven't been recreated because `OnDeviceRestored` is empty. This turns a silent leak into a **crash on the next frame** (null root signature → D3D12 validation error / access violation).

**Mitigation:** Out of scope for this refactor, but the escalation means device-lost recovery should be prioritized as a follow-up. Options:
1. Implement `OnDeviceRestored` to call `OpenGLTranslationState::Init()` (full re-init), or
2. Have the listener guard `OnFrameBegin` with a validity check (e.g. skip if root signature is null) as a temporary safety net.

When device-lost recovery is implemented, `Core` must recreate infrastructure resources, then notify all `IFrameListener` / `IDeviceNotify` implementations to recreate their own.

### `DestroyAll` Scope and Resize

Addressed in Phase 2d. `DestroyAll` is only called from `Shutdown()` and `HandleDeviceLost()` — not from a normal window resize. The real resize risk is RTV/DSV descriptor heap exhaustion (pre-existing bug). Phase 2d fixes both the `DestroyAll` scope and the RTV reuse. Phase 2d must ship with 2a/2b — it is not optional.

### Listener Lifetime

If a listener is destroyed without unregistering, `Core::Prepare()` dispatches to a dangling pointer. `D3D12Backend::Shutdown()` (and later `OpenGLTranslationState::Shutdown()`) must call `Core::Get().UnregisterFrameListener(this)` before releasing any resources. Phase 1d adds this requirement.

### Thread Safety

`DescriptorAllocator` declares `sm_allocationMutex` but never locks it in `Allocate()`. `D3D12Backend::AllocateSRVSlot` has no locking either. All current callers run on the main thread.

**Decision:** Document that descriptor allocation is main-thread-only. Add `DEBUG_ASSERT` for thread ID verification. If multithreaded allocation is needed later, lock the mutex in `Allocate()`.

### Build Cascade

`GraphicsCore.h` is included widely via `pch.h` → `NeuronClient.h` → `GraphicsCore.h`. Any modification triggers a full NeuronClient rebuild.

**Mitigation:** `UploadRingBuffer` gets its own header (Phase 3a). `IFrameListener` is defined in its own `FrameListener.h` (Phase 1b) — `GraphicsCore.h` only forward-declares `class IFrameListener;` and adds the registration methods. If `IFrameListener` evolves later (e.g. adding `OnFrameEnd`), only the `.cpp` files that implement or call it need recompilation — not the entire project.

---

## Step Ordering

Each step is independently compilable and testable. Steps within a phase can often ship in one commit.

| Step | What | Risk | Est. Lines |
|---|---|---|---|
| **1a** | Remove wasted `DescriptorAllocator::SetDescriptorHeaps()` from `Core::Prepare()` | Low | ~2 |
| **1b** | Add `IFrameListener` (own header) + `Register`/`UnregisterFrameListener` to `Core` | Low | ~30 |
| **1c** | Inline `EndFrame`/`BeginFrame` into `Direct3DSwapBuffers` + listener; remove trivial wrappers (`WaitForGpu`, RTV/DSV handle getters) | Low | ~40 |
| **1d** | Update `D3D12Backend::Init()` for listener registration | Low | ~10 |
| **2a** | Resize `DescriptorAllocator` SRV→1024, Sampler→24; delete backend heaps | Medium | ~50 |
| **2b** | Redirect `AllocateSRVSlot`/`GetSRVGPUHandle` and sampler allocation through `DescriptorAllocator` | Medium | ~50 |
| **2c** | Add descriptor free-list to `DescriptorHeap`; wire `glDeleteTextures` to reclaim SRV slots | Medium | ~70 |
| **2d** | Split `DestroyAll` scope; fix RTV/DSV descriptor reuse on resize | Medium | ~30 |
| **3a** | Move `UploadRingBuffer` to own header + `Core` ownership | Low | ~50 |
| **3b** | Rename `D3D12Backend` → `OpenGLTranslationState`, `g_backend` → `g_glState` | Low (mechanical) | ~60 |
| **3c** | Update `TreeRenderer` root sig reference + add coupling comment | Low | ~5 |

**Constraints:**
- Steps 1a–1d should ship together (one commit).
- Steps 2a + 2b + 2c + 2d must ship together — 2a without 2d breaks device-lost recovery; 2c fixes a functional SRV leak that is required for correctness.
- Steps 3a and 3b are independent of each other.
- Step 3c ships with 3b.

---

## Migration Notes

- Each phase is independently shippable and testable.
- Phase 1 is the highest-value change: eliminates the dual frame orchestration and wasted `SetDescriptorHeaps` call.
- Phase 2 eliminates the dual heap problem and adds SRV reclamation. Steps 2a+2b+2c+2d are atomic.
- Phase 3 is mostly mechanical renaming and file moves.
- All ~59 `g_backend` call sites need updating, but the API surface stays similar:
  - Infrastructure: `g_backend.GetUploadBuffer()` → `Graphics::GetCurrentUploadBuffer()`
  - Infrastructure: `g_backend.GetDevice()` / `GetCommandList()` → `Core::Get().GetD3DDevice()` / `GetCommandList()`
  - GL-specific: `g_backend.GetOrCreatePSO()` → `g_glState.GetOrCreatePSO()` (name change only)
  - GL-specific: `g_backend.GetRootSignature()` → `g_glState.GetRootSignature()` (name change only)
- `tree_renderer.cpp` references follow the same pattern as `opengl_directx.cpp` and migrate identically.
- After all phases, `OpenGLTranslationState` contains ~200 lines (root sig, PSO cache, default texture, samplers, listener). `Core` gains ~80 lines (upload buffers, listener dispatch). Net complexity is lower.
