# DX11 Text & Sprite Renderer — Migration Plan

## Goal

Replace all `g_imRenderer` usage in `TextRenderer` (and across the codebase for textured-quad / sprite drawing) with a dedicated, batched DX11 renderer. This eliminates the per-character Begin/End overhead in the immediate-mode emulation layer and provides a reusable primitive for any 2D/3D textured quad.

---

## 1  Current State — What's Wrong

| Issue | Detail |
|---|---|
| **Per-character draw calls** | Every visible glyph in `DrawText2DSimple`, `DrawText3DSimple`, etc. calls `Begin(PRIM_QUADS)` → 4× `Vertex2f`/`TexCoord2f` → `End()`. Each `End()` triggers a full pipeline flush: quad→triangle expansion, `Map(WRITE_DISCARD)` on the VB, constant-buffer upload, `IASet*`, `VSSet*`, `PSSet*`, `Draw()`. A 20-character string = **20 draw calls**. |
| **Outline multiplier** | Outline mode adds **8 extra draw calls per character** (the 3×3 kernel minus centre). A 20-char outlined string = **180 draw calls**. |
| **Redundant state setup** | Blend state, texture bind, sampler, rasteriser state are set identically for every character but never batched. |
| **Scattered textured-quad code** | `mainmenus.cpp`, `renderer.cpp`, `gamecursor.cpp`, and `debug_render.cpp` all duplicate the same "bind texture → 4 vertices → draw" pattern through `g_imRenderer`. |

---

## 2  Design — `SpriteBatch`

A single new class that handles both text rendering and general textured-quad drawing.

### 2.1  Vertex format (lightweight)

```cpp
struct SpriteVertex
{
  DirectX::XMFLOAT3 pos;       // 12 bytes — x, y, z (z = 0 for 2D)
  DirectX::XMFLOAT2 uv;        //  8 bytes
  DirectX::XMFLOAT4 color;     // 16 bytes
};                               // 36 bytes total
```

This is **12 bytes smaller** per vertex than `ImVertex` (no normal). Lighting and fog are never used for text or UI sprites — they can be dropped from the shader entirely.

### 2.2  Batching strategy

```
Begin(texture, blendState)
  AddQuad(...)   ← appends 4 verts to CPU buffer
  AddQuad(...)
  ...
End()            ← single Map + Draw
```

- An **index buffer** with a pre-built quad pattern (0-1-2, 0-2-3, 4-5-6, 4-6-7, …) is created once at init for the maximum batch size. This avoids the quad→triangle vertex expansion that `ImRenderer` does on the CPU.
- Maximum batch size: **4096 quads** (16 384 vertices, 24 576 indices) — enough for the longest text + outline in one draw.
- `End()` does a single `Map(WRITE_DISCARD)` on the VB and one `DrawIndexed()`.
- If the texture or blend state changes mid-batch, the batch auto-flushes and starts a new one (enables future mixed-sprite batching).

### 2.3  Shader pair (`sprite_vs.hlsl` / `sprite_ps.hlsl`)

**Vertex shader** — minimal constant buffer:

```hlsl
cbuffer CBSprite : register(b0)
{
  float4x4 gViewProj;   // combined view × projection (no world — positions are pre-transformed)
};

struct VSIn  { float3 pos : POSITION; float2 uv : TEXCOORD; float4 color : COLOR; };
struct PSIn  { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 color : COLOR; };

PSIn main(VSIn v)
{
  PSIn o;
  o.pos   = mul(float4(v.pos, 1.0), gViewProj);
  o.uv    = v.uv;
  o.color = v.color;
  return o;
}
```

**Pixel shader** — texture sample × vertex colour, optional alpha clip:

```hlsl
Texture2D    gTex : register(t0);
SamplerState gSmp : register(s0);

struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 color : COLOR; };

float4 main(PSIn p) : SV_Target
{
  float4 c = gTex.Sample(gSmp, p.uv) * p.color;
  clip(c.a - 0.001);   // cheap alpha kill
  return c;
}
```

No lighting, no fog, no world matrix — the constant buffer shrinks from **272 bytes** to **64 bytes**.

### 2.4  Class interface

```cpp
// NeuronClient/sprite_batch.h

class SpriteBatch
{
public:
  void Initialise(ID3D11Device* device, ID3D11DeviceContext* context);
  void Shutdown();

  // --- 2D helpers (set up ortho projection internally) ---
  void Begin2D(int screenW, int screenH, BlendStateId blend = BLEND_ALPHA);
  void End2D();

  // --- 3D helpers (caller provides view·proj) ---
  void Begin3D(const DirectX::XMMATRIX& viewProj, BlendStateId blend = BLEND_ALPHA);
  void End3D();

  // --- Primitive operations (valid between Begin/End) ---
  void SetTexture(int textureId);
  void SetTexture(ID3D11ShaderResourceView* srv);
  void SetBlendState(BlendStateId blend);          // auto-flushes if changed
  void SetColor(float r, float g, float b, float a = 1.0f);

  // Add a 2D textured quad (z = 0)
  void AddQuad2D(float x, float y, float w, float h,
                 float u0, float v0, float u1, float v1);

  // Add a 3D billboard quad
  void AddQuad3D(const DirectX::XMFLOAT3& topLeft,
                 const DirectX::XMFLOAT3& topRight,
                 const DirectX::XMFLOAT3& bottomRight,
                 const DirectX::XMFLOAT3& bottomLeft,
                 float u0, float v0, float u1, float v1);

private:
  void Flush();
  // ... D3D11 resources, batch buffer, state tracking ...
};

extern SpriteBatch* g_spriteBatch;
```

---

## 3  `TextRenderer` Rewrite

### 3.1  What changes

| Area | Before | After |
|---|---|---|
| **Per-glyph loop** | `Begin` / 4× Vertex+TexCoord / `End` per char | `AddQuad2D` or `AddQuad3D` per char — append only, no D3D calls |
| **Draw call count** | N (or 9N for outline) | **1** (or **2** for outline: 1 outline batch + 1 foreground batch) |
| **Blend/texture state** | Set every character | Set once in `Begin2D`/`Begin3D`, auto-managed |
| **Matrix save/restore** | `BeginText2D` / `EndText2D` manually saves `g_imRenderer` matrices | `Begin2D(screenW, screenH)` internally sets ortho; `End2D()` restores nothing (self-contained) |
| **Outline rendering** | 8 per-character draw calls in a nested loop | Pre-offset all outline quads in a single batch, then flush once; draw foreground quads in second batch |

### 3.2  `DrawText2DSimple` — before vs after (pseudo-code)

**Before (current):**
```
SetBlendState(shadow ? SUBTRACTIVE : ADDITIVE)
BindTexture(m_textureID)
for each char:
    Begin(PRIM_QUADS)         // ← state setup
      TexCoord / Vertex ×4
    End()                     // ← Map + Draw
UnbindTexture()
```

**After:**
```
g_spriteBatch->Begin2D(screenW, screenH, shadow ? SUBTRACTIVE : ADDITIVE)
g_spriteBatch->SetTexture(m_textureID)
for each char:
    g_spriteBatch->AddQuad2D(x, y, horiSize, size, texX, texY, ...)   // ← append only
g_spriteBatch->End2D()        // ← single Map + DrawIndexed
```

### 3.3  `DrawText3DSimple` — same idea

```
g_spriteBatch->Begin3D(viewProj, shadow ? SUBTRACTIVE : ADDITIVE)
g_spriteBatch->SetTexture(m_textureID)
for each char:
    g_spriteBatch->AddQuad3D(p0, p1, p2, p3, texX, texY, ...)
g_spriteBatch->End3D()
```

### 3.4  Members removed from `TextRenderer`

- ✅ `m_projectionMatrix[16]`, `m_modelviewMatrix[16]` — legacy OpenGL leftovers, removed.
- `m_savedProjMatrix`, `m_savedViewMatrix`, `m_savedWorldMatrix` — **retained**. Still needed by `BeginText2D` / `EndText2D` to save/restore `g_imRenderer` matrices for callers that use `g_imRenderer` between the bracket (e.g. drawing background quads then text).

### 3.5  `BeginText2D` / `EndText2D`

Retained as-is. Callers still bracket text drawing with `g_imRenderer` geometry draws (e.g. background quads in `renderer.cpp`), so these must continue to save/restore the `g_imRenderer` projection, view, and world matrices. The text draw methods themselves are fully self-contained via `SpriteBatch`.

### 3.6  `SetColour` — vertex colour migration

`TextRenderer` previously relied on callers setting vertex colour via `g_imRenderer->Color4f()` before draw calls. Since `SpriteBatch` has its own colour state, a new `SetColour(float r, float g, float b, float a = 1.0f)` method was added to `TextRenderer`:

- Stores colour in `DirectX::XMFLOAT4 m_colour` (default white).
- All draw methods call `g_spriteBatch->SetColor(m_colour)` after `Begin`.
- Matches the existing `SetRenderShadow` / `SetRenderOutline` pattern.
- All callers that previously used `g_imRenderer->Color4f()` or `Color4ub()` before text draws have been migrated to `font.SetColour()`.

---

## 4  Combining with Texture/Sprite Display Migration

### 4.1  Overlap analysis

The following call sites use the **exact same pattern** (bind texture → 4-vertex quad → draw) through `g_imRenderer`:

| File | Usage |
|---|---|
| `GameLogic/mainmenus.cpp` | Darwinian sprite in About window |
| `Starstrike/renderer.cpp` | "Private Demo" logo overlay |
| `Starstrike/gamecursor.cpp` | Cursor sprite + cursor shadow |
| `NeuronClient/debug_render.cpp` | Debug screen-space quad |

All of these are textured (or solid-colour) 2D quads with no lighting or fog — **exactly what `SpriteBatch` is designed for**.

### 4.2  Recommendation: **yes, combine**

`SpriteBatch` should be built as a general-purpose 2D/3D sprite primitive, not a text-only facility. The `AddQuad2D` / `AddQuad3D` API is sufficient for all the above call sites. Benefits:

- One renderer to maintain instead of N scattered `g_imRenderer` quad sequences.
- All sprite draws in a frame can be batched together if textures match (future optimisation: texture atlas / sprite sheet support).
- The same shader pair serves both text and sprites.
- The migration of each call site is mechanical: replace `Begin(PRIM_QUADS)` / `Vertex2f` / `End()` with `AddQuad2D()`.

### 4.3  What stays on `ImRenderer`

`ImRenderer` should continue to handle:

- **Line rendering** (debug wireframes, selection boxes, grid overlays)
- **3D mesh rendering** with lighting/fog (landscape, buildings, units)
- **Triangle fans/strips** for arbitrary geometry

These use the full `ImVertex` format (normals) and the lighting/fog constant buffer. `SpriteBatch` is not a replacement for them.

---

## 5  File Plan

| File | Action | Status |
|---|---|---|
| `NeuronClient/sprite_batch.h` | **New** — class declaration | ✅ Done |
| `NeuronClient/sprite_batch.cpp` | **New** — implementation (init, shutdown, begin/end, add quad, flush) | ✅ Done |
| `NeuronClient/Shaders/sprite_vs.hlsl` | **New** — vertex shader | ✅ Done |
| `NeuronClient/Shaders/sprite_ps.hlsl` | **New** — pixel shader | ✅ Done |
| `NeuronClient/CompiledShaders/sprite_vs.h` | **Generated** — compiled shader byte array (FxCompile build step) | ✅ Done |
| `NeuronClient/CompiledShaders/sprite_ps.h` | **Generated** — compiled shader byte array (FxCompile build step) | ✅ Done |
| `NeuronClient/text_renderer.h` | **Modify** — remove legacy matrix members, add `SetColour`, add `m_colour` | ✅ Done |
| `NeuronClient/text_renderer.cpp` | **Modify** — rewrite draw methods to use `SpriteBatch` | ✅ Done |
| `NeuronClient/window_manager.cpp` | **Modify** — instantiate/destroy `g_spriteBatch` | ✅ Done |
| `NeuronClient/NeuronClient.vcxproj` | **Modify** — add new files + FxCompile entries | ✅ Done |
| `NeuronClient/NeuronClient.vcxproj.filters` | **Modify** — add new files to filters | ✅ Done |
| `NeuronClient/pch.h` | Check — no new includes needed | ✅ Verified |

### Colour migration (callers):

| File | Change | Status |
|---|---|---|
| `Starstrike/game_menu.cpp` | `g_imRenderer->Color4f` → `g_gameFont.SetColour` (outline buttons, title) | ✅ Done |
| `Starstrike/renderer.cpp` | Shadow, outline, private demo, lag text | ✅ Done |
| `Starstrike/global_world.cpp` | Shadow island names, grey inactive | ✅ Done |
| `GameLogic/trunkport.cpp` | Shadow port labels | ✅ Done |
| `Starstrike/taskmanager_interface_icons.cpp` | All 14 shadow/outline + colour pairs | ✅ Done |
| `GameLogic/generator.cpp` | Counter shadow text | ✅ Done |
| `GameLogic/mine.cpp` | Counter shadow text | ✅ Done |
| `GameLogic/prefs_graphics_window.cpp` | Red dash text | ✅ Done |
| `GameLogic/prefs_sound_window.cpp` | Red CPU usage text | ✅ Done |
| `GameLogic/prefs_other_window.cpp` | Grey disabled difficulty text | ✅ Done |
| `GameLogic/darwinia_window.cpp` | Shadow caption, disabled label colour | ✅ Done |
| `Starstrike/location_input.cpp` | Team colour text | ✅ Done |
| `Starstrike/startsequence.cpp` | Translucent caption text | ✅ Done |

### Sprite migration (follow-up, same `SpriteBatch`):

| File | Action | Status |
|---|---|---|
| `GameLogic/mainmenus.cpp` | **Modify** — replace `g_imRenderer` quad with `g_spriteBatch->AddQuad2D` | ✅ Done |
| `Starstrike/renderer.cpp` | **Modify** — replace logo quad + Darwin logo shadow/main quads | ✅ Done |
| `Starstrike/gamecursor.cpp` | **Modify** — replace 2D cursor + 3D cursor quads | ✅ Done |
| `NeuronClient/debug_render.cpp` | **Keep on `ImRenderer`** — solid-colour untextured quad, not suitable for `SpriteBatch` | ⬚ N/A |

---

## 6  Implementation Order

1. ✅ **Create `SpriteBatch` class** with `Initialise`, `Shutdown`, `Begin2D`/`End2D`, `Begin3D`/`End3D`, `AddQuad2D`, `AddQuad3D`, `Flush`.
2. ✅ **Write & compile shaders** (`sprite_vs.hlsl`, `sprite_ps.hlsl`) — FxCompile entries in vcxproj, compiled to header byte arrays.
3. ✅ **Instantiate `g_spriteBatch`** in `window_manager.cpp` — created after `g_textureManager`, destroyed before it.
4. ✅ **Convert `TextRenderer`** — all draw methods rewritten:
   - `DrawText2DSimple` — `AddQuad2D` per glyph, single `End2D()` flush
   - `DrawText2DUp` / `DrawText2DDown` — `AddQuad3D` with rotated corner positions
   - `DrawText3DSimple` — `AddQuad3D` with billboard corners, `Begin3D(viewProj)`
   - `DrawText3D` (oriented) — same pattern with custom front/up vectors
   - `BeginText2D` / `EndText2D` — retained for callers that use `g_imRenderer` between the bracket
5. ✅ **Add `SetColour`** to `TextRenderer` and migrate all callers from `g_imRenderer->Color4f()` (13 files updated).
6. ✅ **Convert sprite call sites** — `gamecursor.cpp` (2D + 3D), `renderer.cpp` (logo + Darwin overlay), `mainmenus.cpp` (Darwinian sprite). `debug_render.cpp` kept on `ImRenderer` (untextured solid quad).
7. ✅ **Remove dead code**:
   - `ImRenderer`: removed `Color4fv`, `Rotatef`, `SetAlphaClipThreshold`, `SetDrawEnabled`/`IsDrawEnabled`, `m_drawEnabled` — none had any callers.
   - `startsequence.cpp`: removed dead ortho matrix setup on `g_imRenderer` (SpriteBatch sets its own), removed dead `Color4f(white)` (text now uses `SetColour`, default is white), updated comment.
   - `g_imRenderer` include retained in `text_renderer.cpp` — still needed for `BeginText2D`/`EndText2D` matrix save/restore and 3D view/proj access.
8. ⬚ **Build & verify** on Release x64 — Debug x64 verified ✅.

---

## 7  Performance Summary

| Metric | Before (`ImRenderer`) | After (`SpriteBatch`) |
|---|---|---|
| Draw calls per 20-char string | 20 | **1** |
| Draw calls per 20-char outlined string | 180 | **2** |
| VB Map/Unmap per string | 20–180 | **1–2** |
| CB upload per string | 20–180 | **1–2** |
| Vertex size | 48 bytes | **36 bytes** |
| CB size | 272 bytes | **64 bytes** |
| Shader complexity | Lighting + fog branches | **Tex × Color only** |

---

## 8  Risks & Mitigations

| Risk | Mitigation | Status |
|---|---|---|
| UV / winding order bugs during conversion | Visual diff test with a screenshot comparison | ⬚ Needs visual testing |
| `g_spriteBatch` lifetime vs `g_imRenderer` | Init `SpriteBatch` right after `ImRenderer`; shut down in reverse order | ✅ Done — init after `g_textureManager`, destroy before it |
| Outline rendering looks different | The outline pass uses the same offsets and blend state; visual diff test | ⬚ Needs visual testing |
| Thread safety | Text rendering is single-threaded (main render loop); `SpriteBatch` does not need to be thread-safe | ✅ N/A |
| Vertex colour lost during migration | Added `TextRenderer::SetColour()` and migrated all callers | ✅ Done |
