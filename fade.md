# ADR: Migrate Fade In/Out from Immediate-Mode Quad to Pixel Shader Global

## Context

The current fade system lives entirely on `Renderer` via three private members:

| Field          | Purpose                                                        |
|----------------|----------------------------------------------------------------|
| `m_fadedness`  | 0.0 = fully visible, 1.0 = fully black                        |
| `m_fadeRate`   | +ve = fading out, −ve = fading in, 0 = idle                   |
| `m_fadeDelay`  | Seconds to wait before the fade begins (used by `StartFadeIn`) |

`Renderer::RenderFadeOut()` renders the effect by drawing a fullscreen black quad
with `glBegin(GL_QUADS)` using alpha blending.  It also advances the fade timer
inline, coupling presentation with simulation.

### Callers (unchanged public API)

| Call site                  | Function            | Action                                     |
|----------------------------|---------------------|--------------------------------------------|
| `main.cpp:191`             | `StartFadeOut()`    | Location transition requested              |
| `global_world.cpp:1240`    | `StartFadeOut()`    | User clicked a mission location            |
| `main.cpp:416`             | `StartFadeIn(0.25)` | Entering global-world loop                 |
| `main.cpp:178,416`         | `StartFadeIn()`     | Entering location game loop                |
| `main.cpp:197,202,427`     | `IsFadeComplete()`  | Gate logic transitions on fade completion  |
| `global_world.cpp:1267`    | `IsFadeComplete()`  | Proceed to load requested location         |
| `script.cpp:419`           | `IsFadeComplete()`  | Block script until fade finishes           |

**None of these callers interact with the rendering details — only with state control
and completion queries.  The public API can remain identical.**

### Why change?

1. **The immediate-mode quad is a legacy OpenGL pattern.** The project already runs
   through a D3D12 backend (`opengl_directx.cpp`) that translates `glBegin`/`glEnd`
   into draw calls.  A fullscreen alpha-blended quad costs a separate draw call, PSO
   lookup, vertex upload, and blend-state change every frame, even when no fade is
   active (`m_fadedness == 0`).

2. **A shader global is cheaper and more correct.**  Multiplying final fragment colour
   by `(1 − fadedness)` inside the existing uber pixel shader applies the fade to
   *every* draw call automatically — including particles, UI text, and any new render
   paths — without an extra pass.

3. **Separation of concerns.**  The time-advance logic currently runs inside a render
   function.  Splitting it into an explicit `AdvanceFade()` tick keeps simulation and
   presentation on separate sides of the update boundary.

---

## Decision

Add a single `float FadeAlpha` field to the existing `PerFrameConstants` constant
buffer.  The pixel shader lerps every fragment toward black by that amount.  The
fullscreen-quad code path in `RenderFadeOut()` is removed.

---

## Detailed Plan

### Phase 1 — Constant Buffer Extension

#### 1a. C++ struct (`NeuronClient/d3d12_backend.h`)

Replace the existing `_miscPad` float at the end of `PerFrameConstants` with
`FadeAlpha`, preserving 256-byte alignment.

```cpp
// Before
UINT  FlatShading;
float PointSize;
float _miscPad;          // <-- repurpose

// After
UINT  FlatShading;
float PointSize;
float FadeAlpha;          // 0 = fully visible, 1 = fully black
```

> **No alignment change** — `_miscPad` was already a `float` at the same offset.
> The struct stays `alignas(256)` and does not grow.

#### 1b. HLSL cbuffer (`NeuronClient/Shaders/PixelShader.hlsl` and `VertexShader.hlsl`)

Mirror the rename in both shaders (they share the same cbuffer layout):

```hlsl
// Before
uint  FlatShading;
float PointSize;
float _miscPad;

// After
uint  FlatShading;
float PointSize;
float FadeAlpha;          // 0 = fully visible, 1 = fully black
```

#### 1c. Constant buffer upload (`NeuronClient/opengl_directx.cpp`)

After the existing `cb.PointSize = ...;` line, set the new field:

```cpp
cb.PointSize    = s_renderState.pointSize;
cb.FadeAlpha    = OpenGLD3D::GetFadeAlpha();   // new accessor
```

#### 1d. Fade state storage + accessor

Add a file-scope float (or a static inside the `OpenGLD3D` namespace) and a
setter/getter pair so the Renderer can push the value once per frame:

```
// NeuronClient/opengl_directx.cpp (or a small header)
namespace OpenGLD3D
{
    void  SetFadeAlpha(float alpha);   // called by Renderer before Render()
    float GetFadeAlpha();              // called by CB upload path
}
```

---

### Phase 2 — Pixel Shader Modification

At the **very end** of `main()` in `PixelShader.hlsl`, after fog and alpha test,
apply the fade:

```hlsl
    // --- Screen fade (applied last) ---
    color.rgb *= (1.0f - FadeAlpha);

    return color;
}
```

This is a single MAD per fragment.  When `FadeAlpha == 0` the multiply is 1.0 and
the compiler will not emit a branch, but the cost is a single scalar multiply —
negligible.

**Note:** This approach fades to **black** (matches the current `glColor4ub(0,0,0,…)`
quad).  If a future design needs fade-to-white or fade-to-colour, extend to
`float4 FadeColor` and use `lerp(color.rgb, FadeColor.rgb, FadeAlpha)`.

---

### Phase 3 — Renderer Refactor (`Starstrike/renderer.h` / `renderer.cpp`)

#### 3a. New `AdvanceFade()` method

Extract the time-advance logic that currently lives inside `RenderFadeOut()` into a
separate public method:

```cpp
// renderer.h
public:
    void AdvanceFade();   // call once per frame, before Render()
```

```cpp
// renderer.cpp
void Renderer::AdvanceFade()
{
    static double lastTime = GetHighResTime();
    double timeNow       = GetHighResTime();
    double timeIncrement = timeNow - lastTime;
    if (timeIncrement > 0.05)
        timeIncrement = 0.05;
    lastTime = timeNow;

    if (m_fadeDelay > 0.0f)
    {
        m_fadeDelay -= static_cast<float>(timeIncrement);
    }
    else
    {
        m_fadedness += m_fadeRate * static_cast<float>(timeIncrement);
        if (m_fadedness < 0.0f) { m_fadedness = 0.0f; m_fadeRate = 0.0f; }
        if (m_fadedness > 1.0f) { m_fadedness = 1.0f; m_fadeRate = 0.0f; }
    }
}
```

#### 3b. Push fade value to shader before rendering

Inside `Renderer::Render()`, after `AdvanceFade()`, push the value:

```cpp
void Renderer::Render()
{
#ifdef PROFILER_ENABLED
    g_app->m_profiler->RenderStarted();
#endif

    AdvanceFade();
    OpenGLD3D::SetFadeAlpha(m_fadedness);

    RenderFrame();

#ifdef PROFILER_ENABLED
    g_app->m_profiler->RenderEnded();
#endif
}
```

#### 3c. Gut `RenderFadeOut()`

Remove all GL draw calls from the body.  The method can be left as an empty stub
(or removed entirely with its call site in `RenderFrame()` deleted) since the pixel
shader now handles the visual effect.

```cpp
void Renderer::RenderFadeOut()
{
    // Fade is now applied per-fragment in the pixel shader via FadeAlpha.
    // Time advance has moved to AdvanceFade().
}
```

Remove the `RenderFadeOut()` call inside `RenderFrame()` to avoid running dead code.

#### 3d. Keep public API unchanged

`StartFadeOut()`, `StartFadeIn(float)`, and `IsFadeComplete()` remain identical —
they only manipulate `m_fadedness`, `m_fadeRate`, and `m_fadeDelay`, which still
live on `Renderer`.  **No caller changes required.**

---

### Phase 4 — Recompile Shaders

The project embeds precompiled shader bytecode in
`NeuronClient/CompiledShaders/PixelShader.h` and `VertexShader.h`.

After modifying the HLSL sources:

1. Recompile `PixelShader.hlsl` → `PixelShader.h` (using `fxc` or `dxc`,
   matching the existing compilation flags in the build).
2. Recompile `VertexShader.hlsl` → `VertexShader.h` (layout changed even though
   the VS doesn't read `FadeAlpha` — the cbuffer offsets must match).
3. Full rebuild of `NeuronClient` and `Starstrike`.

---

## File Change Summary

| File | Change |
|------|--------|
| `NeuronClient/d3d12_backend.h` | Rename `_miscPad` → `FadeAlpha` in `PerFrameConstants` |
| `NeuronClient/Shaders/PixelShader.hlsl` | Rename `_miscPad` → `FadeAlpha`; add `color.rgb *= (1 - FadeAlpha)` before return |
| `NeuronClient/Shaders/VertexShader.hlsl` | Rename `_miscPad` → `FadeAlpha` (layout parity) |
| `NeuronClient/opengl_directx.cpp` | Set `cb.FadeAlpha` from accessor; add `SetFadeAlpha`/`GetFadeAlpha` |
| `NeuronClient/CompiledShaders/PixelShader.h` | Regenerated bytecode |
| `NeuronClient/CompiledShaders/VertexShader.h` | Regenerated bytecode |
| `Starstrike/renderer.h` | Add `AdvanceFade()` declaration |
| `Starstrike/renderer.cpp` | Add `AdvanceFade()` body; call in `Render()`; gut `RenderFadeOut()` |

---

## Consequences

### Positive

- **One fewer draw call per frame.**  The fullscreen quad, its PSO lookup, vertex
  upload, and blend-state toggle are eliminated.
- **Fade applies universally.**  Every fragment — particles, text, UI overlays — is
  faded equally.  The old quad only covered geometry rendered before it in the draw
  order; anything rendered afterward (e.g., debug HUD) was not faded.
- **Simulation / presentation split.**  `AdvanceFade()` runs in the update phase;
  the shader reads a constant.  No render-time mutation of game state.
- **Zero-cost when inactive.**  `FadeAlpha == 0` means `color * 1.0` — a single
  scalar multiply the GPU will pipeline away.
- **Extensible.**  Promoting to `float4 FadeColor` later enables fade-to-white,
  fade-to-colour, or tint effects with no structural change.
- **No caller changes.**  `StartFadeOut`, `StartFadeIn`, `IsFadeComplete` keep
  their existing signatures and semantics.

### Negative

- **Shader recompile required.**  Precompiled bytecode headers must be regenerated
  after editing the HLSL.  One-time cost.
- **Fade now affects *everything*.**  If any future HUD element must remain visible
  during a fade (e.g., a loading spinner), it would need to counter-multiply or use
  a separate render path that bypasses the uber shader.  The old quad approach
  allowed rendering on top of the fade.  If this becomes a requirement, the shader
  can branch on a per-draw `IgnoreFade` flag.
- **Minor coupling.**  `Renderer` now calls into `OpenGLD3D::SetFadeAlpha()`, adding
  a dependency from `Starstrike` → `NeuronClient`.  This follows the existing
  dependency direction (`Starstrike` already depends on `NeuronClient`), so no new
  edge in the dependency graph.

### Alternatives Considered

| Alternative | Why rejected |
|-------------|-------------|
| **Full-screen post-process pass** (render to off-screen RT, then blit with fade) | Overkill for a single scalar multiply; doubles bandwidth; adds RT management complexity |
| **Keep the quad but skip when `fadedness == 0`** | Still pays PSO/blend-state cost when active; doesn't fix the draw-order coverage issue |
| **Separate "post-process" cbuffer** | Adds a second root parameter and descriptor; unnecessary when the existing cbuffer has a free padding slot |

---

## Verification Checklist

- [ ] `PerFrameConstants` C++ struct and both HLSL cbuffers have identical layouts
      (byte-level match verified via `static_assert(offsetof(...))`)
- [ ] Precompiled shader headers regenerated and checked in
- [ ] `FadeAlpha` defaults to `0.0f` — no visual change when fade is inactive
- [ ] `StartFadeOut()` → screen goes black over ~1 second (same speed as before)
- [ ] `StartFadeIn(0.25f)` → 0.25 s delay then screen fades in
- [ ] `IsFadeComplete()` returns `true` at both ends (fully visible, fully black)
- [ ] Location transition (main.cpp loop) still works: fade out → load → fade in
- [ ] Global-world location click (global_world.cpp) still fades out correctly
- [ ] Script `m_waitForFade` (script.cpp) still gates on `IsFadeComplete()`
- [ ] FPS counter, input-mode overlay, and lag warning render correctly during fade
- [ ] Debug / Release × all platform configs build cleanly
- [ ] No visible frame-rate change (profile the pixel shader cost of the multiply)
