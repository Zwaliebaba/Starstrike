# Codebase Improvement Roadmap

This document is the top-level index for all active modernisation initiatives. Each section links to a detailed plan. Work is tracked in pull requests tagged with `[improvement]` in the subject line.

---

## Active Initiatives

| Initiative | Detail Plan | Status | Priority |
|---|---|---|---|
| Math type migration | [MathPlan.md](MathPlan.md) | Phase 1 in progress | High |
| Matrix convention standardisation | [MatrixConv.md](MatrixConv.md) | Phases 0–4.2 complete; 4.3–5 pending | High |
| Server separation | [Server.md](Server.md) | Design complete; implementation pending | Medium |
| Simulation–rendering decoupling | [docs/gamelogic.md](docs/gamelogic.md) | Phase 0 in progress | High |
| `TreeRenderer` dependency reduction | [docs/tree.md](docs/tree.md) | Design complete; implementation pending | Medium |

---

## Standing Technical Debt

These items are tracked here as ongoing cleanup targets. They do not belong to any specific plan; fix them opportunistically when touching the surrounding code.

| Category | Approx. count | Risk | Notes |
|---|---|---|---|
| Unsafe C string functions (`sprintf`, `strcpy`, `strcat`) | 75+ | Buffer-overflow | Replace with `snprintf` / `std::format` / `std::string` in new or modified code |
| Raw `new`/`delete` without RAII | 100+ | Memory leaks, exception unsafety | Migrate to `std::unique_ptr` / `std::vector` in NeuronCore and new Starstrike code first |
| Plain `enum` (should be `enum class`) | 9 | Type-safety gap, implicit int conversion | Straightforward to fix file-by-file; no cross-cutting risk |
| `#ifndef` include guards (should be `#pragma once`) | 212+ | ODR risk from guard-name typos | Mechanical search-and-replace; low risk per file |
| Commented-out code blocks | 30+ | Readability, false archaeology | Delete; rely on git history |
| Stale `#define` branches | 6 | Dead code, confusion | Delete unreachable branches |

### Approach

- Do **not** create large clean-up PRs that touch hundreds of files at once.
- Fix debt items **in the same PR** as any functional change to the same file.
- Reviewers should reject new occurrences of unsafe C strings or raw `new`/`delete` in `NeuronCore` or `Starstrike` code.

---

## Initiative Summaries

### Math Type Migration ([MathPlan.md](MathPlan.md))

Replace the scalar legacy math types (`LegacyVector3`, `Matrix33`, `Matrix34`) in `NeuronClient` with SIMD-backed DirectXMath equivalents centralised in `NeuronCore/GameMath.h`.

**Target types:**

| Legacy | Replacement | Location |
|---|---|---|
| `LegacyVector3` | `GameVector3` (wraps `XMFLOAT3`) | `NeuronCore/GameVector3.h` |
| `Matrix33` | `GameMatrix` (wraps `XMFLOAT4X4`) | `NeuronCore/GameMatrix.h` |
| `Matrix34` | `GameMatrix` | `NeuronCore/GameMatrix.h` |

**Current status:** Storage types (`GameVector3`, `GameMatrix`) are defined in `NeuronCore/GameMath.h`. The compatibility shim phase and file-by-file migration have not started.

**Estimated remaining effort:** 2–3 weeks for a single developer.

---

### Matrix Convention Standardisation ([MatrixConv.md](MatrixConv.md))

Converge all matrix operations on the DirectX row-major, row-vector convention. Eliminate `ConvertToOpenGLFormat` and thread-unsafe static buffers.

**Phase completion:**

| Phase | Description | Status |
|---|---|---|
| 0 | Thread-safety fix for static buffers | ✅ Done |
| 1 | Expose `MatrixStack` directly | ✅ Done |
| 2 | Fix matrix operator semantics | ✅ Done |
| 3 | Introduce `Transform3D` type | ✅ Done |
| 4.1 | Migrate hierarchy composition to SIMD | ✅ Done |
| 4.2 | Shape/fragment render path uses SIMD | ✅ Done |
| 4.3–4.9 | Remaining matrix operations and call-sites | 🔄 In progress |
| 5 | Retire the OpenGL translation layer | ⏳ Pending |

---

### Server Separation ([Server.md](Server.md))

Produce a standalone `StarstrikeServer.exe` that runs on Windows Server Core (no GPU, no WinUI 3). The fix is a single `SERVER_BUILD` preprocessor guard in three `NeuronClient` source files that directly include `GameApp.h`.

**Files to change:**

| File | Change |
|---|---|
| `NeuronClient/server.cpp` | Guard `g_app` references with `#ifndef SERVER_BUILD` |
| `NeuronClient/servertoclient.cpp` | Guard `g_app` references with `#ifndef SERVER_BUILD` |
| `NeuronClient/preferences.cpp` | Guard `g_app->m_resource` reference |

**Deliverable:** Multi-stage `Dockerfile` for Windows Server Core container deployment.

**Estimated effort:** 2–3 days.

---

### Simulation–Rendering Decoupling ([docs/gamelogic.md](docs/gamelogic.md))

Split `GameLogic` into `GameSim` (pure simulation, no GPU headers) and `GameRender` (render companions). Enables `NeuronServer` to link game simulation directly and enables a headless bot client.

**Scope:** 67 of 77 `GameLogic/*.cpp` files currently include rendering dependencies.

**Reference implementation:** `Tree` — rendering fully extracted to `NeuronClient/tree_renderer.h/.cpp`. See [docs/gamelogic.md § Prior Art](docs/gamelogic.md#prior-art-treerenderer-as-reference-implementation) for the patterns that subsequent migrations must follow.

**Phase summary:**

| Phase | Description | Status |
|---|---|---|
| 0 | Audit + dead GL path cleanup in `Tree::RenderBranch` | 🔄 In progress |
| 1 | Break PCH dependency (`GameLogicPlatform.h`) | ⏳ Pending |
| 2 pre | `ShadowRenderer`, render registries, `IRenderBackend` | ⏳ Pending |
| 2 | Entity/building render companions (40+ files) | ⏳ Pending |
| 3 | `SimEventQueue` for side-effect decoupling | ⏳ Pending |
| 4 | Remove `Render()` from `WorldObject`/`Entity`/`Building` base classes | ⏳ Pending |
| 5 | Create `GameSim` and `GameRender` projects; delete `GameLogic` | ⏳ Pending |
| 6 | Separate simulation state from presentation state | ⏳ Pending |
| 7 | Headless `BotClient` executable | ⏳ Pending |

**Estimated effort:** 6–10 weeks total; each phase leaves the game shippable.

---

### `TreeRenderer` Dependency Reduction ([docs/tree.md](docs/tree.md))

Decouple `tree_renderer.cpp` from `d3d12_backend.h` and `opengl_directx.h`. After this work the tree renderer depends only on `Graphics::Core`, `PipelineState`, `RootSignature`, and a new lightweight `DrawConstants.h`.

**Step summary:**

| Step | Description | Status |
|---|---|---|
| 1 | Extract `DrawConstants` to a standalone header | ⏳ Pending |
| 2 | Move the root signature accessor to `Graphics::Core` | ⏳ Pending |
| 3 | Introduce `TreeDrawParams` struct | ⏳ Pending |
| 4 | Refactor `DrawTree()` to accept `TreeDrawParams` | ⏳ Pending |
| 5 | Update `location.cpp` call site | ⏳ Pending |
| 6 | Verify `Init()` call order | ⏳ Pending |
| 7 | Remove `d3d12_backend.h` / `opengl_directx.h` from `tree_renderer.cpp` | ⏳ Pending |
| 8 | Clean up cached texture fields from `tree_renderer.h` | ⏳ Pending |

**Note:** This is a prerequisite for moving `TreeRenderer` into the `GameRender` project (Phase 5 of the simulation–rendering decoupling initiative).

---

## Dependency Between Initiatives

```
TreeRenderer dependency reduction (docs/tree.md)
  └─► Must be complete before TreeRenderer moves to GameRender (gamelogic.md Phase 5)

Math type migration (MathPlan.md)
  └─► Progresses in parallel with simulation-rendering decoupling
  └─► Matrix convention work (MatrixConv.md) consumes new GameMath types

Server separation (Server.md)
  └─► Can be done at any time; independent of other initiatives
  └─► Delivers more value once simulation-rendering decoupling (Phase 5) is complete
      because NeuronServer can then link GameSim directly
```

---

## How to Contribute to an Initiative

1. Pick an item from one of the detailed plan documents.
2. Create a feature branch: `feature/<initiative>/<short-desc>`.
3. Follow the [Coding Standards](.github/coding-standards.md).
4. Open a PR with `[improvement]` in the title.
5. Reference the specific phase/step from the plan document.

Reviewers will check that the change leaves the game shippable (all configurations build and pass manual gameplay validation).
