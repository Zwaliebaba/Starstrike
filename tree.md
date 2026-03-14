# Tree.cpp — OpenGL-to-Native-DX12 Conversion Plan

## 1. Current State Analysis

### 1.1 What Tree.cpp Does Today

`Tree` is a `Building` subclass that generates and renders procedural L-system trees.
All rendering lives in two methods:

| Method | Purpose |
|---|---|
| `Generate()` | Recursively walks the L-system, emitting quads into two **OpenGL display lists** (branches, leaves). Also computes `m_hitcheckCentre` / `m_hitcheckRadius`. |
| `RenderAlphas()` | Called per-frame from `Location::RenderBuildingAlphas()`. Sets blend/texture state, pushes modelview matrix, calls `glCallList` for each display list. |

`Render()` is empty — all visuals go through the alpha pipeline because the tree is drawn additively with depth-write disabled.

### 1.2 OpenGL API Surface Used

```
glGenLists / glNewList / glEndList / glDeleteLists / glCallList   — display lists
glBegin(GL_QUADS) / glEnd                                        — immediate mode
glVertex3fv / glTexCoord2f / glColor4ubv                         — per-vertex data
glEnable/glDisable(GL_TEXTURE_2D, GL_BLEND, GL_CULL_FACE)       — fixed-function state
glBlendFunc(GL_SRC_ALPHA, GL_ONE)                                — additive blending
glDepthMask(false/true)                                          — depth write toggle
glBindTexture / glTexParameteri                                  — texture binding
OpenGLD3D::GetModelViewStack().Push/Multiply/Scale/Pop           — matrix stack
```

### 1.3 How It Works Today Under the Hood

The codebase already has an **OpenGL-to-DX12 translation layer** (`opengl_directx*.h/cpp`):

1. `glNewList` → creates a `DisplayListRecorder` that records `DLCommand` objects.
2. `glBegin(GL_QUADS)` / `glVertex3fv` / `glEnd` → records a `DLCommandDraw` with `CustomVertex` data (`pos, normal, color, uv0, uv1`).
3. `glEndList` → compiles recorded data into a `DisplayList` holding a **D3D12 committed vertex buffer** and a command list.
4. `glCallList` → replays commands: calls `PrepareDrawState` (uploads CBV, binds PSO, textures, samplers) then issues `DrawInstanced` per draw command.
5. All draw calls go through the **uber vertex/pixel shader** (`VertexShader.hlsl` / `PixelShader.hlsl`) that emulates OpenGL fixed-function pipeline state via `PerFrameConstants`.

**Translation overhead per tree per frame:**
- `PrepareDrawState` re-uploads the full `PerFrameConstants` CBV (~2 KB) per draw call.
- PSO is looked up from a hash-map cache keyed by `PSOKey` (blend, depth, cull, topology).
- Each display list replay issues at least **one** `DrawInstanced` + **one** CBV upload + descriptor table sets.
- Two display lists = **two** full state-setup passes per tree.

### 1.4 Vertex Format (current, via translation layer)

```cpp
struct CustomVertex {    // 48 bytes — OpenGLD3D::CustomVertex
    float x, y, z;       // POSITION
    float nx, ny, nz;    // NORMAL (unused by tree — always 0,0,0)
    UINT32 diffuse;      // COLOR (BGRA packed)
    float u, v;          // TEXCOORD0
    float u2, v2;        // TEXCOORD1 (unused by tree — always 0,0)
};
```

Tree only needs **position (12 B) + texcoord (8 B) = 20 bytes per vertex**. Normal and second UV are wasted bandwidth (28 bytes/vertex ×2 quads/segment × all segments × 2 lists).

### 1.5 Geometry Profile

Each branch segment emits **2 cross-quads = 8 vertices, 4 quads** (= 16 triangles when converted from GL_QUADS).

A typical tree with `m_iterations = 6` and `numBranches = 4` per split produces roughly:
- **~1,365 branch segments** (4^0 + 4^1 + ... + 4^5)
- **~10,920 GL-side vertices** per display list (4 verts/quad × 2,730 quads)
- The translation layer expands GL_QUADS inline to triangles (6 verts/quad), so **~16,380 stored vertices** per list
- **×2 display lists** = ~32,760 stored vertices total
- At 48 B/vertex (current) = **~1.5 MB** in committed GPU memory per tree

After optimization to 20 B/vertex (non-indexed, 6 verts/quad) = ~**655 KB** per tree.
With indexing (4 unique verts × 20 B + 6 indices × 2 B per quad) = ~**440 KB** per tree.

---

## 2. Goals

| # | Goal | Rationale |
|---|---|---|
| G1 | **Eliminate display lists and GL calls** from `Tree` entirely | Remove dependency on OpenGL translation layer |
| G2 | **Dedicated PSO** for tree rendering | Avoid uber-shader overhead; simplify state |
| G3 | **Compact vertex format** (pos + uv only, color as push constant) | Reduce GPU memory and bandwidth per tree |
| G4 | **Single instanced draw call** per tree (or per tree batch) | Minimize CPU overhead and draw call count |
| G5 | **Preserve all existing behavior** | Fire/regrow height scaling, Christmas mod, depth-sorted alpha blend, hit-testing — all unchanged |
| G6 | **Keep `Generate()` on CPU** | L-system with random seeds must remain deterministic; GPU compute is overkill |
| G7 | **Respect GameLogic project boundary** | `tree.h/cpp` live in GameLogic (no DX12 headers). Rendering code goes in NeuronClient or Starstrike. |

---

## 3. Architecture

### 3.1 Separation of Concerns

```
GameLogic/tree.h/cpp        — Owns tree data, L-system generation, gameplay logic
                               Produces a CPU-side vertex buffer (TreeMeshData)
                               No GL or DX12 headers

NeuronClient/tree_renderer.h/cpp  — Owns DX12 resources (VB, IB, PSO, root sig, shaders)
                                     Receives TreeMeshData, uploads to GPU
                                     Exposes DrawTree() called from rendering code

Starstrike/location.cpp     — Calls TreeRenderer::DrawTree() instead of tree->RenderAlphas()
```

### 3.2 Data Flow

```
Tree::Generate()
  ├─ Runs recursive L-system (unchanged logic)
  ├─ Emits vertices into TreeMeshData (CPU vector)
  │    struct TreeVertex { float x,y,z; float u,v; };  // 20 bytes
  ├─ Computes m_hitcheckCentre / m_hitcheckRadius (unchanged)
  └─ Flags m_meshDirty = true

TreeRenderer::EnsureUploaded(Tree*)
  ├─ If both meshes are empty, calls tree->Generate() (lazy-gen safety net)
  ├─ If m_meshDirty, uploads TreeMeshData to GPU default heap via staging buffer
  ├─ Creates D3D12 vertex buffer + vertex buffer view
  └─ Clears m_meshDirty

TreeRenderer::DrawTree(Tree*, float predictionTime)
  ├─ Calls EnsureUploaded
  ├─ Computes world transform (translation + uniform scale by actualHeight)
  ├─ Sets root CBV: WorldView + Proj matrices, branchColor or leafColor, fog params
  ├─ Binds tree PSO, VB, texture SRV
  ├─ DrawInstanced for branch vertices
  └─ Updates root constant color, DrawInstanced for leaf vertices
```

### 3.3 Module Diagram

```
┌─────────────────────────────────────────────────┐
│ GameLogic (static lib, no graphics headers)      │
│                                                   │
│  Tree                                             │
│   ├─ m_branchMesh : TreeMeshData                  │
│   ├─ m_leafMesh   : TreeMeshData                  │
│   ├─ m_meshDirty  : bool                          │
│   ├─ Generate()   → fills m_branchMesh/m_leafMesh │
│   └─ Advance() / Damage() / Read() / Write()     │
│                                                   │
│  TreeMeshData (POD)                               │
│   ├─ vertices : std::vector<TreeVertex>           │
│   └─ TreeVertex { float x,y,z,u,v; }             │
└──────────────────┬──────────────────────────────┘
                   │ (link-time dependency)
┌──────────────────▼──────────────────────────────┐
│ NeuronClient (static lib, has DX12 headers)      │
│                                                   │
│  TreeRenderer (singleton or per-location)         │
│   ├─ m_pso        : GraphicsPSO                   │
│   ├─ m_rootSig    : RootSignature                 │
│   ├─ m_gpuBuffers : std::unordered_map<int,       │
│   │     TreeGPUData>  (keyed by GetUniqueId)       │
│   ├─ Init()       → create PSO, root sig, shaders │
│   ├─ Shutdown()   → release all GPU resources      │
│   ├─ EnsureUploaded(Tree*)                         │
│   └─ DrawTree(Tree*, float predictionTime)         │
│                                                   │
│  TreeGPUData (per-tree GPU resources)             │
│   ├─ vertexBuffer : com_ptr<ID3D12Resource>       │
│   ├─ vbView       : D3D12_VERTEX_BUFFER_VIEW      │
│   ├─ branchVertexStart / branchVertexCount         │
│   └─ leafVertexStart   / leafVertexCount           │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│ Starstrike (executable)                          │
│                                                   │
│  Location::RenderBuildingAlphas()                 │
│   └─ For TypeTree buildings:                      │
│        g_treeRenderer->DrawTree(tree, predTime)   │
└─────────────────────────────────────────────────┘
```

---

## 4. Detailed Design

### 4.1 TreeVertex (GameLogic — no graphics dependency)

```cpp
// tree_mesh_data.h (GameLogic)
#pragma once
#include <vector>

struct TreeVertex
{
    float x, y, z;
    float u, v;
};

struct TreeMeshData
{
    std::vector<TreeVertex> vertices;
    void Clear() { vertices.clear(); }
    void Reserve(size_t n) { vertices.reserve(n); }
};
```

**20 bytes per vertex** vs. current 48 bytes — **58% bandwidth reduction**.

### 4.2 Tree Changes (GameLogic)

#### 4.2.1 New Members

```cpp
// tree.h — replace display list IDs with mesh data
TreeMeshData m_branchMesh;
TreeMeshData m_leafMesh;
bool m_meshDirty = true;
```

Remove:
- `int m_branchDisplayListId`
- `int m_leafDisplayListId`
- `void DeleteDisplayLists()`

#### 4.2.2 Generate() Refactor

Replace the `glNewList` / `glBegin` / `glEnd` / `glEndList` blocks with:

```cpp
void Tree::Generate()
{
    float timeNow = GetHighResTime();

    m_hitcheckCentre.Zero();
    m_hitcheckRadius = 0.0f;
    m_numLeafs = 0;

    intToArray(m_branchColour, m_branchColourArray);
    intToArray(m_leafColour, m_leafColourArray);

    // Leaf alpha adjustment for tree detail
    int treeDetail = g_prefsManager->GetInt("RenderTreeDetail", 1);
    if (treeDetail > 1)
    {
        int alpha = m_leafColourArray[3];
        alpha *= pow(1.3f, treeDetail);
        alpha = min(alpha, 255);
        m_leafColourArray[3] = alpha;
    }

    // Generate branch mesh
    m_branchMesh.Clear();
    darwiniaSeedRandom(m_seed);
    GenerateBranch(g_zeroVector, g_upVector, m_iterations,
                   false, true, false, m_branchMesh);

    // Generate leaf mesh
    m_leafMesh.Clear();
    darwiniaSeedRandom(m_seed);
    GenerateBranch(g_zeroVector, g_upVector, m_iterations,
                   false, false, true, m_leafMesh);

    // Compute hit-check centre from accumulated leaf positions
    m_hitcheckCentre /= static_cast<float>(m_numLeafs);
    // Compute hit-check radius
    RenderBranch(g_zeroVector, g_upVector, m_iterations,
                 true, false, false);  // radius-only pass (no mesh output)
    m_hitcheckRadius *= 0.8f;

    m_meshDirty = true;

    float totalTime = GetHighResTime() - timeNow;
    DebugTrace("Tree generated in {}ms\n", static_cast<int>(totalTime * 1000.0f));
}
```

#### 4.2.3 GenerateBranch() — New Mesh-Filling Overload

The existing `RenderBranch` is refactored to output to a `TreeMeshData*` instead of calling GL:

```cpp
void Tree::GenerateBranch(LegacyVector3 _from, LegacyVector3 _to, int _iterations,
                          bool _calcRadius, bool _renderBranch, bool _renderLeaf,
                          TreeMeshData& _mesh)
{
    // Same recursive logic as RenderBranch, but instead of:
    //   glTexCoord2f(...); glVertex3fv(...);
    // emit:
    //   _mesh.vertices.push_back({ x,y,z, u,v });
    //
    // CRITICAL: When _renderLeaf is true and _iterations reaches 0, this
    // function must also accumulate hit-check data exactly as RenderBranch does:
    //   m_hitcheckCentre += _to;
    //   m_numLeafs++;
    // Without this, the subsequent radius-calculation pass will produce garbage
    // because m_hitcheckCentre will be zero.
    //
    // Two cross-quads per segment = 8 vertices → 12 vertices (two tris per quad)
    // OR keep 8 vertices and use an index buffer (see §4.3)
}
```

Each GL_QUAD (4 vertices: A,B,C,D) becomes two triangles: (A,B,C) and (A,C,D).
This conversion happens during generation, not at draw time.

#### 4.2.4 RenderAlphas() — Gutted

```cpp
void Tree::RenderAlphas(float _predictionTime)
{
    // No-op: rendering is now handled by TreeRenderer::DrawTree()
}
```

Or better: remove the override entirely so the base-class no-op is used, and call `TreeRenderer::DrawTree()` from `Location::RenderBuildingAlphas()` directly.

### 4.3 TreeRenderer (NeuronClient)

#### 4.3.1 Shaders

Create a minimal **tree-specific shader pair** that does only what trees need.

Both shaders share a single cbuffer definition via an `#include`-d header to avoid layout mismatches (the VS and PS both bind `b0` — DX12 sends the same memory to both stages, so the struct layout **must** be identical).

**TreeConstants.hlsli** (shared by VS and PS)
```hlsl
// Shared constant buffer layout — must match C++ TreeConstants struct exactly.
cbuffer TreeConstants : register(b0)
{
    row_major float4x4 WorldView;   // Model × View (eye-space transform)
    row_major float4x4 Projection;  // Projection matrix
    float4 Color;                   // Branch or leaf color (RGBA normalized)
    float4 FogColor;
    float  FogStart;
    float  FogEnd;
    uint   FogEnabled;
    float  _pad;                    // Align to 16-byte boundary
};
// Total: 2×64 + 16 + 16 + 4 + 4 + 4 + 4 = 176 bytes = 44 DWORDs
```

**TreeVertexShader.hlsl**
```hlsl
#include "TreeConstants.hlsli"

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord  : TEXCOORD0;
    float  FogDist   : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 eyePos    = mul(float4(input.Position, 1.0f), WorldView);
    output.Position  = mul(eyePos, Projection);
    output.Color     = Color;
    output.TexCoord  = input.TexCoord;
    output.FogDist   = length(eyePos.xyz);  // eye-space distance, matches uber shader
    return output;
}
```

**TreePixelShader.hlsl**
```hlsl
#include "TreeConstants.hlsli"

Texture2D    TreeTexture : register(t0);
SamplerState TreeSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float  FogDist  : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = TreeTexture.Sample(TreeSampler, input.TexCoord);
    float4 color = texColor * input.Color;  // MODULATE

    if (FogEnabled)
    {
        float fogFactor = saturate((FogEnd - input.FogDist) / (FogEnd - FogStart));
        color.rgb = lerp(FogColor.rgb, color.rgb, fogFactor);
    }

    return color;
}
```

**Why dedicated shaders?** The uber shader carries ~50 uniforms for 8 lights, material properties, alpha test, color material mode, multi-texturing, etc. Trees use **none** of these. A dedicated CBV is cheaper to upload (~176 bytes vs. ~2 KB per draw).

**Why split WorldView + Projection?** A fused WorldViewProj matrix loses the eye-space position needed for fog distance. The uber shader computes `FogDist = length(worldPos.xyz)` where `worldPos` is the eye-space position (after modelview). Splitting the matrices lets the VS compute eye-space distance identically, ensuring fog matches the existing rendering.

#### 4.3.2 Root Signature

```
Param 0: Root CBV (b0) — TreeConstants (176 bytes / 44 DWORDs, via upload ring buffer)
Param 1: Descriptor table — 1 SRV (t0) for tree texture
Static sampler: s0 — linear filter, wrap mode, trilinear mip
```

> **Note**: 44 DWORDs is technically within root-constants limits (max 64 DWORDs total)
> but uses most of the budget. A root CBV descriptor pointing to the upload ring
> buffer is preferred — it costs only 2 DWORDs in the root signature and the 176-byte
> payload lives in the ring buffer.

#### 4.3.3 PSO

```
Input layout:   POSITION (R32G32B32_FLOAT), TEXCOORD0 (R32G32_FLOAT)
Topology:       Triangle list
Blend:          SrcAlpha / One (additive)
Depth:          Test enabled, write disabled
Cull:           None (double-sided quads)
RTV format:     B8G8R8A8_UNORM (matches GraphicsCore)
DSV format:     D32_FLOAT (matches GraphicsCore)
VS/PS:          Compiled TreeVertexShader / TreePixelShader
```

#### 4.3.4 Per-Tree GPU Data

```cpp
struct TreeGPUData
{
    com_ptr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView;
    UINT branchVertexStart;
    UINT branchVertexCount;
    UINT leafVertexStart;
    UINT leafVertexCount;
};
```

Branch and leaf vertices are packed into a **single vertex buffer** (branches first, then leaves). Two `DrawInstanced` calls with different `StartVertexLocation` and different root-constant color.

#### 4.3.5 Upload Strategy

- **On Generate**: Tree flags `m_meshDirty = true`.
- **On first DrawTree call** (or explicit `EnsureUploaded`):
  0. If both `m_branchMesh` and `m_leafMesh` are empty, call `tree->Generate()` as a lazy-generation safety net (mirrors the existing `RenderAlphas` fallback at tree.cpp:267-268).
  1. Compute total vertex count = `branchMesh.size() + leafMesh.size()`.
  2. Create a **dedicated one-shot upload resource** (committed, `D3D12_HEAP_TYPE_UPLOAD`). **Do not use** the per-frame `UploadRingBuffer` — it is reset every frame and the `CopyBufferRegion` must complete (GPU-side) before the staging memory is reclaimed. A one-shot resource with deferred release (tied to the frame-end fence) avoids this race.
  3. `memcpy` both meshes contiguously into the upload resource.
  4. Create a **default-heap** vertex buffer.
  5. `CopyBufferRegion` from upload to default on the command list.
  6. Enqueue the upload resource for deferred release after the GPU fence signals.
  7. Store `TreeGPUData` keyed by `tree->m_id.GetUniqueId()` (guaranteed unique even after building-slot reuse).
- **On Tree destruction**: Release `TreeGPUData` entry.

Since trees are generated once and the geometry is static until `SetDetail` is called, this is a one-time upload cost.

#### 4.3.6 DrawTree() Pseudocode

```cpp
void TreeRenderer::DrawTree(Tree* tree, float predictionTime)
{
    EnsureUploaded(tree);
    TreeGPUData& gpu = m_gpuBuffers[tree->m_id.GetUniqueId()];

    // NOTE: GetActualHeight() is currently protected in Tree.
    // Either make it public, or add a public accessor:
    //   float Tree::GetRenderHeight(float predictionTime) const
    float actualHeight = tree->GetRenderHeight(predictionTime);

    // Build world × view matrix (eye-space) — needed for correct fog distance.
    // BuildTreeWorldMatrix must replicate the existing Matrix34 construction:
    //   Matrix34 mat(m_front, g_upVector, m_pos);   // ctor(f, u, pos): r = u^f, u = f^r
    //   mv.Multiply(mat);
    //   mv.Scale(actualHeight, actualHeight, actualHeight);
    // The parameter order here (pos, front, height) differs from Matrix34's ctor
    // — the implementation must internally construct with (front, g_upVector, pos)
    // then apply uniform scale.
    XMMATRIX world = BuildTreeWorldMatrix(tree->m_pos, tree->m_front, actualHeight);
    XMMATRIX view = GetCurrentViewMatrix();       // from camera
    XMMATRIX proj = GetCurrentProjectionMatrix();  // from camera
    XMMATRIX worldView = world * view;

    auto* cmdList = Graphics::Core::Get().GetCommandList();

    // Set PSO and root signature
    cmdList->SetPipelineState(m_pso.GetPipelineStateObject());
    cmdList->SetGraphicsRootSignature(m_rootSig.GetSignature());

    // Bind texture (laser.bmp SRV — see §4.3.7 for how this handle is acquired)
    cmdList->SetGraphicsRootDescriptorTable(1, m_treeTextureSRV);

    // Bind vertex buffer
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &gpu.vbView);

    // --- Prepare CBV ---
    TreeConstants cb;
    // IMPORTANT: Verify transpose convention against the uber shader's CBV upload
    // path in d3d12_backend.cpp before implementation.  With `row_major` in HLSL
    // and XMMATRIX (row-major in memory), storing WITHOUT transpose is likely
    // correct.  If the uber shader transposes before upload, match that convention.
    XMStoreFloat4x4(&cb.WorldView, worldView);
    XMStoreFloat4x4(&cb.Projection, proj);
    PopulateFogParams(&cb);  // reads from same source as uber-shader fog state

    // --- Draw branches ---
    ApplyBranchColor(tree, &cb.Color);  // handles Christmas mod (== 1 check)
    auto cbAlloc = UploadConstants(&cb, sizeof(cb));
    cmdList->SetGraphicsRootConstantBufferView(0, cbAlloc.gpuAddr);
    cmdList->DrawInstanced(gpu.branchVertexCount, 1, gpu.branchVertexStart, 0);

    // --- Draw leaves ---
    // NOTE: Original code uses `ChristmasModEnabled() != 1`, NOT `!ChristmasModEnabled()`.
    // The function may return values other than 0/1, so use the exact comparison.
    if (Location::ChristmasModEnabled() != 1)
    {
        ApplyLeafColor(tree, &cb.Color);
        auto cbAlloc2 = UploadConstants(&cb, sizeof(cb));
        cmdList->SetGraphicsRootConstantBufferView(0, cbAlloc2.gpuAddr);
        cmdList->DrawInstanced(gpu.leafVertexCount, 1, gpu.leafVertexStart, 0);
    }
}
```

**Draw calls per tree: 2** (branches + leaves) — same count as before, but each draw now skips the `PrepareDrawState` overhead (no PSO hash lookup, no 2 KB uber-CBV upload, no descriptor table rebind). The win is **cost per draw call**, not draw call count.

#### 4.3.6a Color Byte Ordering

`ApplyBranchColor` / `ApplyLeafColor` must convert `m_branchColourArray[4]` (populated by `intToArray`) to a normalized `float4`.

`intToArray` extracts bytes in little-endian order from the color `int`:
```cpp
a[0] = x & 0xFF;  // least significant byte
a[1] = (x >> 8) & 0xFF;
a[2] = (x >> 16) & 0xFF;
a[3] = (x >> 24) & 0xFF;  // most significant byte
```

`glColor4ubv` in OpenGL interprets the 4-byte array as **(R, G, B, A)** in order. The translation layer's `glColor4ubv` implementation converts these to `CustomVertex::diffuse` (BGRA packed, D3D convention), internally swapping R↔B.

Since the new path bypasses the translation layer, `ApplyBranchColor` must perform the conversion directly:
```cpp
cb.Color = {
    array[0] / 255.f,  // R (= byte 0 of intToArray output)
    array[1] / 255.f,  // G
    array[2] / 255.f,  // B
    array[3] / 255.f   // A
};
```

**Verification required at Phase D3**: if the translation layer's `glColor4ubv` does NOT follow standard OpenGL RGBA ordering (i.e., it treats the bytes as BGRA), the R and B channels above must be swapped. Compare a non-white tree's branch color in a side-by-side screenshot to confirm.

#### 4.3.7 Texture SRV Acquisition

The tree texture (`textures/laser.bmp`) is loaded by `Resource::GetTexture()` which returns a GL texture ID. Under the hood, `glBindTexture` maps this to a D3D12 SRV via the translation layer's internal texture table (see `opengl_directx.cpp` — each texture gets a `srvIndex` stored alongside its `com_ptr<ID3D12Resource>`).

**Options:**
1. **Query the translation layer**: Add a public accessor `OpenGLD3D::GetTextureSRVGPUHandle(GLuint texId)` that returns the `D3D12_GPU_DESCRIPTOR_HANDLE` for an existing GL texture. Minimal code, reuses existing texture loading.
2. **Independent load**: Load `laser.bmp` directly via `BitmapRGBA` → create D3D12 texture + SRV independently. Cleaner but duplicates the texture in GPU memory.

**Recommended**: Option 1 — add a thin accessor to the translation layer. The texture is already loaded and its SRV already allocated. This is a one-time lookup at `TreeRenderer::Init()` time.

---

## 5. Integration Points

### 5.1 Location::RenderBuildingAlphas() (Starstrike)

In the depth-sorted building render loop, detect `TypeTree` and dispatch to `TreeRenderer`:

```cpp
if (building->m_type == Building::TypeTree)
{
    auto tree = static_cast<Tree*>(building);
    g_treeRenderer->DrawTree(tree, timeSinceAdvance);
}
else
{
    building->RenderAlphas(timeSinceAdvance);
}
```

This also applies to the non-sorted render path earlier in the same function.

### 5.2 Renderer::Initialise() / Shutdown()

`TreeRenderer::Init()` called during startup; `TreeRenderer::Shutdown()` during teardown. Follows existing pattern of subsystem init in `Renderer` or `App`.

### 5.3 SetDetail() — Regeneration

`Tree::SetDetail()` calls `Generate()` which sets `m_meshDirty`. Next `DrawTree()` call triggers GPU re-upload. No change to call site needed.

### 5.4 Tree Destruction

When a `Tree` is destroyed, `TreeRenderer` must release the corresponding `TreeGPUData`. Options:
- **Observer pattern**: TreeRenderer subscribes to building-destroyed event.
- **Explicit call**: Tree destructor calls `g_treeRenderer->ReleaseTree(m_id)` (requires forward-declared interface in GameLogic).
- **Lazy cleanup**: TreeRenderer checks for stale entries periodically.

Recommended: **Explicit call via an abstract interface** defined in GameLogic, implemented in NeuronClient. This avoids GameLogic depending on DX12 headers.

```cpp
// GameLogic/tree_render_interface.h
class ITreeRenderBackend
{
public:
    virtual ~ITreeRenderBackend() = default;
    virtual void ReleaseTree(int uniqueId) = 0;  // pass WorldObjectId::GetUniqueId()
};
extern ITreeRenderBackend* g_treeRenderBackend;  // set by NeuronClient at init
```

> **Why `int uniqueId` and not `WorldObjectId`?** `GetIndex()` is the building-slot
> index, which can be **reused** after a building is destroyed and a new one created.
> `GetUniqueId()` is a monotonically increasing ID that is never reused, making it
> safe as a map key for GPU resource lifetime tracking.

---

## 6. New Files

| File | Project | Purpose |
|---|---|---|
| `GameLogic/tree_mesh_data.h` | GameLogic | `TreeVertex`, `TreeMeshData` structs |
| `GameLogic/tree_render_interface.h` | GameLogic | `ITreeRenderBackend` abstract interface |
| `NeuronClient/tree_renderer.h` | NeuronClient | `TreeRenderer` class declaration |
| `NeuronClient/tree_renderer.cpp` | NeuronClient | `TreeRenderer` implementation |
| `NeuronClient/Shaders/TreeConstants.hlsli` | NeuronClient | Shared cbuffer layout (included by VS and PS) |
| `NeuronClient/Shaders/TreeVertexShader.hlsl` | NeuronClient | Tree-specific vertex shader |
| `NeuronClient/Shaders/TreePixelShader.hlsl` | NeuronClient | Tree-specific pixel shader |
| `NeuronClient/CompiledShaders/TreeVertexShader.h` | NeuronClient | Compiled shader bytecode (via FXC/DXC) |
| `NeuronClient/CompiledShaders/TreePixelShader.h` | NeuronClient | Compiled shader bytecode (via FXC/DXC) |

**Build system changes** (not new files, but required modifications):
- `GameLogic/GameLogic.vcxproj` — add `tree_mesh_data.h` and `tree_render_interface.h` to ClInclude.
- `NeuronClient/NeuronClient.vcxproj` — add `tree_renderer.h` to ClInclude, `tree_renderer.cpp` to ClCompile, HLSL files with FXC custom build rules (check existing `VertexShader.hlsl` / `PixelShader.hlsl` entries as a template for the build step configuration).

---

## 7. Migration Steps (Ordered)

### Phase A — Mesh Generation Refactor (GameLogic only, no rendering changes)

1. **A1**: Create `GameLogic/tree_mesh_data.h` with `TreeVertex` and `TreeMeshData`.
2. **A2**: Add `m_branchMesh`, `m_leafMesh`, `m_meshDirty` members to `Tree`.
3. **A3**: Create `GenerateBranch()` overload that writes to `TreeMeshData&` instead of calling GL. Convert GL_QUADS (4-vert) to triangle pairs (6-vert) during emission. **Critical**: the leaf pass (`_renderLeaf=true`) must also accumulate `m_hitcheckCentre += _to` and `m_numLeafs++` at `_iterations == 0`, exactly as the original `RenderBranch` does — the radius-calculation pass depends on this data.
4. **A4**: Refactor `Generate()` to call `GenerateBranch()` for mesh output. Keep existing `RenderBranch()` for the hit-check radius pass (no GL calls in that pass anyway). Make `GetActualHeight()` public (or add a public `GetRenderHeight()` wrapper) so `TreeRenderer` can access it.
5. **A5**: Verify build. Mesh data is generated but not yet used — old display-list path still active. Both paths produce identical geometry for validation. **Recommended**: add a debug-only validation step that dumps `TreeMeshData` vertex positions and compares against the display list's `CustomVertex` positions (stripping the unused normal/uv1 fields) to confirm geometric equivalence before proceeding to Phase B.

### Phase B — DX12 Renderer (NeuronClient, no integration yet)

6. **B1**: Write `TreeConstants.hlsli`, `TreeVertexShader.hlsl`, and `TreePixelShader.hlsl`. Add FXC custom build rules to `NeuronClient.vcxproj` (mirror existing `VertexShader.hlsl` / `PixelShader.hlsl` entries). Compile to bytecode headers.
7. **B2**: Add `OpenGLD3D::GetTextureSRVGPUHandle(GLuint texId)` accessor to the translation layer so `TreeRenderer` can obtain the SRV for `textures/laser.bmp` without duplicating the texture.
8. **B3**: Create `TreeRenderer` class: Init (PSO, root sig, static sampler, texture SRV lookup), Shutdown, EnsureUploaded, DrawTree, ReleaseTree. Add `tree_renderer.cpp` to `NeuronClient.vcxproj`.
9. **B4**: Unit-test upload path: generate a tree, upload, verify vertex buffer contents.

### Phase C — Integration (Starstrike)

10. **C1**: Create `ITreeRenderBackend` interface in GameLogic. Wire `g_treeRenderBackend` assignment during NeuronClient init.
11. **C2**: Modify `Location::RenderBuildingAlphas()` to dispatch `TypeTree` to `TreeRenderer::DrawTree()`. Note: trees always return `true` from `PerformDepthSort()`, so only the depth-sorted render path needs the dispatch (the non-sorted path never reaches trees).
12. **C3**: Gut `Tree::RenderAlphas()` (empty body or remove override).
13. **C4**: Remove display-list members and `DeleteDisplayLists()` from Tree.
14. **C5**: Wire `Tree::~Tree()` to call `g_treeRenderBackend->ReleaseTree(m_id.GetUniqueId())`.
15. **C6**: Init/shutdown `TreeRenderer` in app lifecycle (alongside `D3D12Backend`).

### Phase D — Cleanup

16. **D1**: Remove all `#include` of GL-related headers from `tree.cpp` that are no longer needed.
17. **D2**: Verify all build configurations (Debug/Release × x64).
18. **D3**: Visual regression test: compare screenshots with old rendering. Pay special attention to fog blending at distance.
19. **D4**: Profile frame time with many trees (100+). Verify improvement.

---

## 8. Trade-Off Analysis

### 8.1 Dedicated PSO vs. Reusing Uber Shader

| | Dedicated PSO | Uber Shader |
|---|---|---|
| **Pros** | Minimal CBV (~176 B vs. ~2 KB), no dead uniforms, simpler shader = faster compile, cleaner code | No new shaders to maintain, reuses existing infrastructure |
| **Cons** | New shader files, new PSO init code | Wasted bandwidth, all uber-shader branching evaluated, harder to optimize later |
| **Decision** | **Dedicated PSO** — trees are rendered frequently enough that the per-draw savings justify the one-time setup cost |

### 8.2 Single VB (branches+leaves) vs. Two VBs

| | Single VB | Two VBs |
|---|---|---|
| **Pros** | One `IASetVertexBuffers` call, simpler upload, less resource tracking | Cleaner separation |
| **Cons** | Need to track offsets | Two VB binds per tree, two resources per tree |
| **Decision** | **Single VB** — pack branches first, then leaves. Use offset in `DrawInstanced`. |

### 8.3 Triangle List vs. Indexed Triangle List

| | Non-indexed | Indexed |
|---|---|---|
| **Pros** | Simpler upload (no IB), simpler draw call | Saves ~33% vertex data (4 unique verts + 6 indices per quad vs. 6 verts) |
| **Cons** | 6 verts/quad vs. 4+6 indices | Index buffer overhead, more complex upload |
| **Decision** | **Start non-indexed** (Phase A simplicity). Add index buffer as a follow-up optimization if vertex buffer sizes become a concern. |

### 8.4 Per-Tree VB vs. Shared/Batched VB

| | Per-tree VB | Shared large VB |
|---|---|---|
| **Pros** | Simple lifetime management, independent upload | Fewer VB binds, potential for instancing |
| **Cons** | Many small buffers, one VB bind per tree | Complex sub-allocation, fragmentation |
| **Decision** | **Per-tree VB** initially. If profiling shows VB bind cost is significant (unlikely with <100 trees), migrate to sub-allocation from a shared buffer. |

### 8.5 Instancing

Trees with the same `m_seed` and `m_iterations` produce identical geometry. In theory, multiple trees could share one VB and use instanced drawing with per-instance transforms. However:
- Trees are depth-sorted individually for correct alpha blending.
- Different trees have different colors, heights, and fire states.
- The number of unique tree types is small but not trivially groupable.

**Decision**: Defer instancing. Per-tree draws with root-constant transforms are simple and sufficient for the expected tree counts (<256 per location, as seen in the depth-sort limit).

---

## 9. Performance Expectations

| Metric | Before (display list replay) | After (native DX12) |
|---|---|---|
| Vertex format | 48 bytes | 20 bytes |
| CBV upload per draw | ~2 KB (PerFrameConstants) | ~176 bytes (TreeConstants) |
| PSO lookup | Hash-map per draw | Pre-bound, one set per batch |
| State setup cost per draw | Full uber-shader state (CBV + PSO lookup + descriptor tables) | Minimal (CBV only — PSO/VB/texture already set) |
| Draw calls per tree | 2 (display list replays) | 2 (branch + leaf) — same count, much lower cost per call |
| GPU memory per tree | ~1.5 MB | ~655 KB (non-indexed) or ~440 KB (indexed) |

Expected improvement: **~57% less GPU memory per tree, ~91% less CBV upload per draw, elimination of per-draw PSO hash lookup**.

---

## 10. Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Visual differences (color, blending) | Medium | High | Side-by-side screenshot comparison at Phase D3. Keep old code behind `#ifdef` until validated. |
| GameLogic depending on DX12 headers | Low | High | `ITreeRenderBackend` interface keeps GameLogic clean. `TreeMeshData` is pure C++ with no graphics API. |
| Upload ring buffer exhaustion with many trees generating simultaneously | Low | Medium | Trees generate once at level load. One-shot upload resources are created per tree and deferred-released after the GPU copy completes, so ring buffer capacity is no longer the bottleneck. However, if many trees upload in a single frame, the total committed upload memory spikes — consider batching uploads across frames if tree count exceeds ~90. |
| Fog parameter mismatch | Medium | Medium | WorldView + Proj split ensures eye-space fog distance matches uber shader. Read fog start/end/color from same source as uber-shader. Regression-test fog at D3. |
| Christmas mod color override | Low | Low | Handle in `ApplyBranchColor()` — same logic as current `RenderAlphas()`. |
| `GetActualHeight()` is protected | Low | Medium | Must be made public or wrapped in a public accessor before `TreeRenderer` can call it. Straightforward change in `tree.h`. |
| Texture SRV acquisition | Low | Medium | Requires a thin accessor on the translation layer (`GetTextureSRVGPUHandle`). If the translation layer is refactored away later, this accessor moves to the new texture manager. |
| `WorldObjectId::GetIndex()` reuse | Medium | High | Building slot indices are recycled. Using `GetIndex()` as a GPU resource map key can cause stale VB to be drawn for a new tree. Always use `GetUniqueId()`. |
| Matrix transpose convention mismatch | Medium | High | `row_major` HLSL + `XMMATRIX` (row-major in C++) should NOT need transpose. But if the uber shader's upload path transposes, the tree shader must match. Verify against `d3d12_backend.cpp` `PerFrameConstants` upload before writing `TreeRenderer`. |
| Color channel order (R↔B swap) | Medium | Medium | `intToArray` byte order + `glColor4ubv` semantics determine whether `m_branchColourArray` is RGBA or BGRA. New path bypasses translation layer, so must replicate the correct mapping. Verify with non-white trees at Phase D3. |
| Upload staging buffer lifetime | Low | High | The one-shot upload resource for `CopyBufferRegion` must survive until the GPU copy completes. Tie its release to the frame-end fence. Do not use the per-frame `UploadRingBuffer` which resets every frame. |

---

## 11. Checklist

- [ ] `TreeVertex` and `TreeMeshData` defined (A1)
- [ ] Tree members updated — mesh data replaces display list IDs (A2)
- [ ] `GenerateBranch()` emits to `TreeMeshData` with quad→triangle conversion; accumulates `m_hitcheckCentre`/`m_numLeafs` in leaf pass (A3)
- [ ] `Generate()` uses new path; `GetActualHeight()` made public or wrapped (A4)
- [ ] Build passes with mesh generation only (A5)
- [ ] `TreeConstants.hlsli` + tree shaders written and compiled; FXC build rules added to vcxproj (B1)
- [ ] Translation layer accessor `GetTextureSRVGPUHandle()` added (B2)
- [ ] `TreeRenderer` class implemented with Init/Shutdown/DrawTree/ReleaseTree; added to vcxproj (B3)
- [ ] Upload path tested (B4)
- [ ] `ITreeRenderBackend` interface wired (C1)
- [ ] `Location::RenderBuildingAlphas()` depth-sorted path dispatches trees to `TreeRenderer` (C2)
- [ ] `Tree::RenderAlphas()` gutted (C3)
- [ ] Display list members removed from Tree (C4)
- [ ] Tree destructor calls `g_treeRenderBackend->ReleaseTree(m_id.GetUniqueId())` (C5)
- [ ] `TreeRenderer` lifetime managed in app startup/shutdown (C6)
- [ ] Unnecessary `#include`s removed from tree.cpp (D1)
- [ ] All configs build (D2)
- [ ] Visual regression test passed — including fog at distance and non-white tree color channels (D3)
- [ ] Matrix transpose convention verified against uber shader's CBV upload path (D3)
- [ ] Color byte ordering verified: non-white trees match old rendering (no R↔B swap) (D3)
- [ ] Performance profiled with 100+ trees (D4)
