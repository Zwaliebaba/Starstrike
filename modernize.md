# Starstrike Modernization Recommendations

## Tech Stack Summary

| Aspect | Value |
|--------|-------|
| C++ Standard | `stdcpplatest` (all configurations) |
| Platform Toolset | v145 (MSVC 14.50) |
| Graphics API | DirectX 12 (via OpenGL shim layer) |
| NuGet Dependencies | CppWinRT 2.0, WindowsAppSDK 1.8, WinPixEventRuntime 1.0 |
| Architecture | 6 static libraries + 1 executable + 1 test DLL |
| Build Warnings | **19** (0 errors) — down from 425 |

---

## 📊 Current Status

> Last verified: full solution Build (Debug x64) — 0 errors, all projects up-to-date.

| Phase | Status | Summary |
|-------|--------|---------|
| Phase 1: Critical Bug Fixes | ✅ Complete | All 10 steps done + bonus `InputTransform` virtual dtor |
| Phase 2: Build Config Alignment | ✅ Complete | All 5 steps done |
| Phase 3: Warning Infrastructure | ✅ Complete | GameRender raised to `/W4`, 0 warnings |
| Phase 4: Warning Reduction — Correctness | ✅ Complete | Shadowing, signed/unsigned, `MIN` macro all fixed across all projects |
| Phase 5: Warning Reduction — Cosmetic | ✅ Complete | Unused params/locals/float literals fixed across all projects. 19 C4100 in `opengl_directx.cpp` intentionally skipped (DX12 coordination). |
| Phase 6: Test Infrastructure | ✅ Complete | `NeuronCore.Tests` project with 55 passing tests (GameMath, GameVector3, Transform3D) |
| Phase 7: Large Migrations | ⬜ Not Started | Detailed plan with 5 sub-phases: 7.1 `auto_vector`, 7.2 HashTable, 7.3 `using namespace`, 7.4 LList, 7.5 DArray family |
| Phase 8: Regression Prevention | ✅ Complete | Step 8.1 (docs fix) done. Step 8.2 already done in Phase 2. |

---

## 📎 Related Plans

| Plan | Location | Status | Overlap |
|------|----------|--------|---------|
| DX12 Migration | `GameRender/DX12Migration.md` | Phases 0–2 ✅, Phases 3–6 planned | §17 unreachable code in `renderer.cpp` will be removed by Phase 6. §21 `glColor4f` double literals are in GL call sites being eliminated. §19 signed/unsigned mismatches in GL renderers will disappear as renderers are rewritten. |
| String Modernization | `string.md` | ⬜ Not Started | Migrates `snprintf`/`strncpy`/`strlen`/`vsprintf` → `std::format`/`std::string`/`std::string_view`. Replaces §15 from this document. |
| Render Architecture | `GameRender/Render.md` | Reference | Describes rendering subsystem layout |

> **Coordination note:** Before fixing warnings in GL-layer files (`opengl_directx*.cpp`, renderer GL call sites), check whether the DX12 migration plan already schedules those files for rewrite. Avoid spending effort on code that will be replaced.

---

## 🔴 Critical Issues

### 1. Undefined Behavior: Deleting Abstract Classes Without Virtual Destructors

Three abstract classes are deleted through base pointers without virtual destructors (**C5205**). This is **undefined behavior** per the C++ standard and can cause memory corruption or crashes.

| Class | File | Triggered From |
|-------|------|----------------|
| `Movement2D` | `NeuronClient\movement2d.h:8` | `movement2d.cpp:95` via `std::unique_ptr` |
| `InputDriver` | `NeuronClient\inputdriver.h:42` | `input.cpp:23` via raw `delete` |
| `InputFilter` | `NeuronClient\inputfilter.h:9` | `inputfiltermanager.cpp:18` via raw `delete` |

**Fix:** Add `virtual ~Movement2D() = default;`, `virtual ~InputDriver() = default;`, and `virtual ~InputFilter() = default;` to each base class.

> ✅ **Fixed** (Phase 1) — Virtual destructors added to all three classes. Also added `virtual ~InputTransform() = default;` in `transform.h` (same pattern discovered during implementation).


### 2. Potentially Uninitialized Variables (C4701)

Variables used before guaranteed initialization — can cause random crashes or wrong behavior at runtime.

| File | Line | Variable |
|------|------|----------|
| `NeuronClient\inputdriver_xinput.cpp` | 591 | `type` |
| `NeuronClient\sound_library_3d_dsound.cpp` | 442 | `errCode` |

**Fix:** Initialize both variables at declaration.

> ✅ **Fixed** (Phase 1) — Both variables initialized at declaration.

---

### 3. Assignment Used as Condition (C4706)

```cpp
// inputdriver_xinput.cpp:484
if (m_state.isConnected = (ERROR_SUCCESS == dwResult))
```

This assigns the comparison result to `m_state.isConnected` and branches on it. The pattern is **intentional** — it's common in driver polling code to store and test in one expression. However, it triggers C4706 and is confusing to future readers.

**Fix:** Split into two statements:
```cpp
m_state.isConnected = (ERROR_SUCCESS == dwResult);
if (m_state.isConnected)
```

---

### 3b. Missing `break` in `switch` Fallthrough (Likely Bug)

```cpp
// inputdriver_xinput.cpp:526–537
case XInputLeftThumbstickLeft:
// ... 7 more direction sub-cases ...
case XInputRightThumbstickDown:
    spec.handler_id = 100; // Set default sensitivity
    // ← missing break; falls through to default
default:
    return STATE_DONE;
```

The preceding group (thumbstick/trigger axes) returns `STATE_WANT_OPTIONAL` after setting `handler_id = 100`. The direction sub-cases set the same `handler_id` but fall through to `default: return STATE_DONE` instead of also returning `STATE_WANT_OPTIONAL`. This means direction sub-axis inputs silently skip optional token parsing (sensitivity modifier).

**Fix:** Add `return STATE_WANT_OPTIONAL;` before the `default:` label (matching the axis cases above), or add `[[fallthrough]];` with a comment if the current behavior is intended.

---

## 🟠 High-Priority Improvements

### 4. Debug/Release Language Standard Mismatch

All projects compile with `stdcpplatest` in Debug but `stdcpp20` in Release:

| Project | Debug | Release | Notes |
|---------|-------|---------|-------|
| NeuronCore | `stdcpplatest` | `stdcpp20` | |
| NeuronClient | `stdcpplatest` | `stdcpp20` | |
| GameLogic | `stdcpplatest` | `stdcpp20` | |
| GameRender | `stdcpplatest` | `stdcpp20` | Also has orphaned Win32 configs with inconsistent standards (Debug\|Win32 = `stdcpp20`) |
| NeuronServer | `stdcpplatest` | `stdcpp20` | |
| Starstrike | `stdcpplatest` | `stdcpp20` | Uses `$(Configuration)` without `$(Platform)` — platform-agnostic conditions |

This can cause **ODR violations** and subtle behavioral differences when code uses C++23/26 features that silently degrade or behave differently in C++20 Release builds.

**Concrete evidence:** `NeuronCore.h:31` includes `<mdspan>`, which is a C++23 header. In Debug (`stdcpplatest`) this compiles natively; in Release (`stdcpp20`) it may fall back to a partial implementation or fail silently. This confirms the mismatch is not theoretical.

**Recommendation:** Align to `stdcpplatest` everywhere. The codebase already uses `<mdspan>` and `<ranges>` features beyond C++20, so downgrading to `stdcpp20` would require removing those usages. Aligning to `stdcpplatest` is the lower-risk direction.

**Risk:** Changing the Release standard may expose latent issues where C++23 semantics differ from C++20 (e.g., `auto(x)` decay-copy, `if consteval`). Build and run-test Release after this change.

> ✅ **Fixed** (Phase 2) — All six projects aligned to `stdcpplatest` in both Debug and Release. GameRender `Debug|Win32` also fixed before its removal.


### 5. Warning Level Inconsistency

`GameRender` compiles at `/W3` while all other projects use `/W4`. This hides warnings in GameRender that the other projects would catch.

| Project | Warning Level |
|---------|--------------|
| NeuronCore | `/W4` |
| NeuronClient | `/W4` |
| GameLogic | `/W4` |
| **GameRender** | **`/W3`** ← mismatch |
| NeuronServer | `/W4` |
| Starstrike | `/W4` |

**Fix:** Set `GameRender` to `/W4` to match the rest of the solution.

**⚠️ Warning count impact:** GameRender has 48+ renderer `.cpp` files currently at `/W3`. Raising to `/W4` will expose a significant number of new C4100/C4189/C4267 warnings (estimate: 30–80 new warnings). **Recommended approach:** Fix the existing 8 GameRender warnings at `/W3` first, then raise to `/W4`, then fix the new crop in a follow-up pass.

> ✅ **Fixed** (Phases 2–3) — GameRender raised to `/W4`. All exposed warnings fixed. 0 GameRender warnings at `/W4`.

---

### 6. AVX2 Missing in Release Builds (Solution-Wide)

Five of six projects enable `AdvancedVectorExtensions2` in Debug but **not in Release**. GameRender has no AVX2 in either configuration.

| Project | Debug AVX2 | Release AVX2 |
|---------|-----------|-------------|
| NeuronCore | ✅ | ❌ |
| NeuronClient | ✅ | ❌ |
| GameLogic | ✅ | ❌ |
| NeuronServer | ✅ | ❌ |
| Starstrike | ✅ | ❌ |
| **GameRender** | **❌** | **❌** |

Release builds miss SIMD optimizations for math-heavy code across the entire solution.

**Fix:** Add `<EnableEnhancedInstructionSet>AdvancedVectorExtensions2</EnableEnhancedInstructionSet>` to the Release `<ClCompile>` section of all six `.vcxproj` files. Add it to both Debug and Release for `GameRender.vcxproj`.

---

### 7. Format-String Bugs (Active Bugs)

These produce **wrong output** at runtime:

| File | Line | Warning | Issue |
|------|------|---------|-------|
| `sound_library_3d_dsound.cpp` | 357 | C4477 | `%s` used with `LPCTSTR` — should be `%ls` in Unicode builds |
| `taskmanager_interface_icons.cpp` | 1284 | C4474 | Too many arguments for `snprintf` format — extra argument silently dropped |

**Fix:** Change `%s` → `%ls` for wide strings; remove the extra argument from the `snprintf` call.

---

### 8. Pointer Truncation on x64 (C4311/C4302)

```cpp
// file_paths.cpp:57
int localeID = int(GetKeyboardLayout(0)) & 0xFFFF;
```

`HKL` is a 64-bit handle on x64. Casting to `int` truncates the pointer.

**Fix:** Use `static_cast<int>(reinterpret_cast<intptr_t>(GetKeyboardLayout(0)) & 0xFFFF)`.

---

### 9. Global `#pragma warning(disable:4244)` Undermines Warning Fixes

~~`NeuronCore/NeuronCore.h:13` globally disables C4244.~~ **Update:** The global disable has been removed from `NeuronCore.h`. C4244 is now suppressed **only** in `NeuronCore/pch.h:3`:
```cpp
#pragma warning(disable:4244) // TODO: Remove after fixing C4244 warnings in this project
```

All other projects (GameLogic, GameRender, NeuronClient, NeuronServer, Starstrike) have C4244 fully enabled and compile clean. The other disabled warnings (`C4201`, `C4238`, `C4324`) remain in `NeuronCore.h` — these are for legitimate MSVC extensions used by DirectX/WinRT headers.

**Remaining work:** Fix C4244 warnings in NeuronCore translation units, then remove the pragma from `NeuronCore/pch.h`. Not currently prioritized — will be addressed when NeuronCore warnings become relevant.

> 🟡 **Partially Fixed** — C4244 re-enabled in 5 of 6 projects. Only `NeuronCore/pch.h` still suppresses it.

---

### 10. `using namespace` at File Scope in `NeuronCore.h`

`NeuronCore/NeuronCore.h` lines 83 and 91:
```cpp
using namespace winrt;     // line 83
using namespace Neuron;    // line 91
```

This header is included by every `pch.h` in the solution. These directives pollute the global namespace in **every translation unit**, risking ambiguous symbol resolution as the codebase grows (e.g., `winrt::handle` vs. a future `Neuron::handle`).

**Fix:** Remove the `using namespace` directives from the header. Qualify usages at call sites or add `using` declarations in individual `.cpp` files where needed. This is a large but mechanical change — `using namespace Neuron` is the higher-risk one since `Neuron::` types are used everywhere.

---

### 11. `<iostream>` Included in Game Header

`NeuronClient/inputfilter.h:4` includes `<iostream>` — one of the heaviest C++ standard headers (pulls in locales, streams, sync primitives). In a game codebase where this header is included transitively through `pch.h`, it adds unnecessary compile-time and binary-size overhead.

**Fix:** Remove the `#include <iostream>` if unused. If stream output is needed, prefer `<iosfwd>` for declarations or use the project's `Neuron::DebugTrace` instead.

> ✅ **Fixed** (Phase 2) — `#include <iostream>` removed from `inputfilter.h`. It was unused.


### 12. `WIN32_LEAN_AND_MEAN` Commented Out

`NeuronCore/NeuronCore.h:65`:
```cpp
#if !defined WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
#endif
```

The define is commented out, so `<Windows.h>` pulls in the full Win32 API surface (Cryptography, DDE, RPC, Shell, Winsock-compat, etc.). Enabling it reduces PCH size and compile times.

**Fix:** Uncomment the `#define`. Note: `<winsock2.h>` is already included explicitly (line 68), so Winsock is not affected. Test that no code depends on headers excluded by `WIN32_LEAN_AND_MEAN` (e.g., `<mmsystem.h>` for audio — may need an explicit include in sound files).

> ✅ **Fixed** (Phase 2) — `WIN32_LEAN_AND_MEAN` uncommented. No missing-header errors encountered.

---

## 🟡 Build Warning Summary (19 remaining — all intentionally skipped)

> **Note:** The 19 remaining are all C4100 in `opengl_directx.cpp` (skipped per DX12 coordination note). The actionable warning count is **0**. C4244 is re-enabled in 5 of 6 projects (§9) — only NeuronCore still suppresses it. GameRender is at `/W4` (§5 fixed).

### Warning Distribution by Project

| Project | Warnings |
|---------|----------|
| NeuronClient | 19 (all `opengl_directx.cpp` C4100 — skipped) |
| GameLogic | 0 |
| Starstrike | 0 |
| GameRender | 0 |
| NeuronCore | 0 |
| NeuronServer | 0 |

### Warning Distribution by Category

| Warning | Count (approx) | Category | Risk |
|---------|----------------|----------|------|
| C4100 (unused parameters) | ~80 | Dead code | Medium |
| C4189 (unused locals) | ~35 | Dead code / potential bugs | Medium |
| C4018/C4245/C4267/C4389 (signed/unsigned) | ~100 | Type safety | High |
| C4305 (double→float truncation) | ~20 | Precision loss | Low |
| C4456/C4457 (variable shadowing) | ~15 | Correctness risk | High |
| C4702 (unreachable code) | 54 | Dead code in `renderer.cpp` | Medium |
| C5205 (UB: non-virtual dtor) | 3 | Undefined behavior | **Critical** |
| C4701 (uninitialized variable) | 2 | Runtime crash risk | **Critical** |
| C4706 (assignment as condition) | 1 | Potential logic bug | High |
| C4474/C4477 (format string) | 2 | Wrong output | High |
| C4311/C4302 (pointer truncation) | 2 | Data loss on x64 | High |

---

## 🟡 Modernization Opportunities

### 13. Custom Containers → STL

The codebase has **7** custom container classes that predate modern C++. These lack proper iterator support, use manual memory management (`new`/`delete`, shadow arrays), and miss STL optimizations.

#### Inheritance Graph

```
DArray<T>  ←──  FastDArray<T>  ←──  SliceDArray<T>
HashTable<T>  ←──  SortingHashTable<T>
LList<T>          (standalone)
auto_vector<T>    (standalone)
```

> **Migration order matters.** Leaf classes (`SliceDArray`, `SortingHashTable`) must be migrated first (or adapted), then their parents. A `using DArray = std::vector`-style typedef is **not** feasible because `DArray` has slot-tracking semantics (shadow array) that `std::vector` does not provide.

#### Full Inventory

| Custom Container | Location | Inherits | STL Replacement | Notes |
|-----------------|----------|----------|-----------------|-------|
| `DArray<T>` | `NeuronClient/darray.h` | — | `std::vector<T>` + slot bitmap, or a sparse set | Shadow `char*` array tracks used/unused slots |
| `FastDArray<T>` | `NeuronClient/fast_darray.h` | `DArray<T>` | Same as `DArray` replacement + free-list | Adds `int* freelist` for O(1) insert |
| `SliceDArray<T>` | `NeuronClient/slice_darray.h` | `FastDArray<T>` | Custom wrapper or remove slicing pattern | Adds per-frame iteration slicing |
| `LList<T>` | `NeuronClient/llist.h` | — | `std::vector<T>` (better cache locality) or `std::list<T>` | Intrusive doubly-linked list |
| `HashTable<T>` | `NeuronClient/hash_table.h` | — | `std::unordered_map<std::string, T>` | `char*` keys, open addressing |
| `SortingHashTable<T>` | `NeuronClient/sorting_hash_table.h` | `HashTable<T>` | `std::map<std::string, T>` (ordered) or sorted `std::vector` | Adds ordered iteration via index chain |
| `auto_vector<T>` | `NeuronClient/auto_vector.h` | — | `std::vector<std::unique_ptr<T>>` | Ownership-transfer semantics, third-party code (Reliable Software ©2003) |

#### Code Quality Issues in Container Headers

`slice_darray.h` and `sorting_hash_table.h` include `pch.h` *inside the header file* and self-include (e.g., `slice_darray.h:43` has `#include "pch.h"`, line 49 has `#include "slice_darray.h"`). This is a non-standard pattern — the template implementations are inlined after a self-include guard. While `#pragma once` prevents infinite recursion, this is confusing and should be refactored into separate `.inl` files or kept header-only without the self-include.

**Impact:** Improved cache performance, reduced maintenance burden, elimination of several classes of memory bugs.

**Effort:** Large — these containers are used pervasively in `GameLogic`, `NeuronClient`, `GameRender`, and `Starstrike`. A call-site count per container type is needed before committing to a migration order.

---

### 14. Legacy `MIN` Macro

`NeuronCore\net_lib.h` defines:
```cpp
#define MIN(a,b) ((a < b) ? a : b)
```

Despite `NOMINMAX` being set and `<algorithm>` being included globally via `pch.h`. Macro `MIN` is susceptible to double-evaluation bugs.

**Fix:** Replace with `std::min` throughout the networking code.

> ✅ **Fixed** (Phase 4) — `MIN` macro replaced with `std::min` in networking code.

---

### 15. Unsafe C String Functions

> **Moved to [`string.md`](string.md)** — full migration plan with per-file call-site inventory, 8 implementation phases, format string translation reference, and member field migration strategy.

---

### 16. No Unit Tests

The solution has **zero test projects**. The `copilot-instructions.md` mentions using the Visual Studio Native Unit Test Framework, but no tests exist.

**Recommendation:** Start with unit tests for `NeuronCore` math functions:
- `GameMath.h` — vector operations (`Normalize`, `Cross`, `Dot`, `RotateAround`, etc.)
- `GameVector3.h` — storage type construction, `Load()`/`Store()` round-trips
- `Transform3D.h` — matrix construction, `Right()`/`Up()`/`Forward()`/`Pos()` accessors

These are pure logic with no external dependencies and are critical for correctness.

> ✅ **Fixed** (Phase 6) — `NeuronCore.Tests` project created with 55 tests across 3 test classes (`GameMathTests`, `GameVector3Tests`, `Transform3DTests`). All tests passing. Project uses VS Native Unit Test Framework, matches solution conventions (v145, `stdcpplatest`, AVX2, `/W4`).

---

### 17. `renderer.cpp` Has 54 Lines of Unreachable Code (C4702)

Lines 483–587 in `Starstrike/renderer.cpp` (`Renderer::CheckOpenGLState`) are all unreachable after an early `return;` on line 483. This is OpenGL fixed-function state validation code (checks `GL_FRONT_FACE`, `GL_SHADE_MODEL`, lighting, blending, fog, texture state).

**Cross-reference:** This entire function validates OpenGL state that is emulated by the GL-to-D3D12 translation layer. **DX12 Migration Phase 6** (`GameRender/DX12Migration.md`) will remove the GL layer entirely, which will delete this function. If Phase 6 is far out, guard with `#ifdef _DEBUG` now to suppress the 54 C4702 warnings.

**Fix:** If DX12 Phase 6 is imminent (< 2 months), leave as-is to avoid churn. Otherwise, guard with `#ifdef _DEBUG` or delete the unreachable body.

---

## 🟢 Code Quality / Minor Fixes

### 18. Unused Variables & Parameters (~115 warnings)

Many are in base-class virtual methods with empty bodies (e.g., `EclButton::Render`, `Building::SetDetail`).

**Quick fixes:**
- Use `[[maybe_unused]]` on base-class virtual method parameters
- Delete truly unused locals (e.g., `int b = 10;` in `camera.cpp:230` and `spawnpoint.cpp:410`)

> ✅ **Fixed** (Phases 4–5) — All unused parameters annotated with `[[maybe_unused]]`, all unused locals removed across Starstrike, NeuronClient, and GameLogic. Only 19 C4100 in `opengl_directx.cpp` remain (intentionally skipped per DX12 coordination note).


### 19. Signed/Unsigned Mismatches (~100 warnings)

Most are loop variables iterating over `.Size()` methods that return `unsigned int`, while the loop counter is declared as `int`:
```cpp
for (int i = 0; i < m_items.Size(); ++i)  // Size() returns unsigned
```

**Fix:** Change loop counters to match return types, or use range-for where possible.

---

### 20. Variable Shadowing (~15 warnings)

Variables redeclared in inner scopes hiding outer declarations. Most problematic in:

| File | Shadowed Variable |
|------|------------------|
| `landscape.cpp` | `segStart`, `segEnd` (4 instances each) |
| `taskmanager_interface_icons.cpp` | `captionId`, `zone`, `boxX`, `boxY`, `boxH`, `i` |
| `location.cpp` | `team`, `i` |
| `global_world.cpp` | `levFile`, `i` |
| `renderer.cpp` | `i` |

**Fix:** Rename inner variables or restructure scopes.

> ✅ **Fixed** (Phase 4) — All shadowing warnings fixed across Starstrike, NeuronClient, and GameLogic: `taskmanager_interface_icons.cpp`, `global_world.cpp`, `level_file.cpp`, `building.cpp` (`i` → `j` in inner loop).


### 21. Double-to-Float Truncation (~20 warnings)

Literal doubles used where floats are expected:
```cpp
float m[blurSize] = {0.2, 0.3, 0.4, ...};  // Should be 0.2f, 0.3f, etc.
m_up.Set(0.15, 0.93, 0.31);                  // Should be 0.15f, etc.
glColor4f(0.5, 0.5, 1.0, 0.3);              // Should be 0.5f, etc.
```

**Fix:** Add `f` suffix to all float literals.

---

## 📋 Implementation Plan

### Phase 1: Critical Bug Fixes ✅ Complete

All items are independent — can be done in any order within the phase.

| Step | Section | Action | Files (Project) | Build Impact |
|------|---------|--------|-----------------|--------------|
| 1.1 | §1 | Add `virtual ~Movement2D() = default;` | `NeuronClient/movement2d.h` (NeuronClient) | Medium — header included by movement subclasses |
| 1.2 | §1 | Add `virtual ~InputDriver() = default;` | `NeuronClient/inputdriver.h` (NeuronClient) | Medium — header included by driver subclasses |
| 1.3 | §1 | Add `virtual ~InputFilter() = default;` | `NeuronClient/inputfilter.h` (NeuronClient) | Medium — header included by filter subclasses |
| 1.4 | §2 | Initialize `type` at declaration | `NeuronClient/inputdriver_xinput.cpp:591` (NeuronClient) | Low — `.cpp` only |
| 1.5 | §2 | Initialize `errCode` at declaration | `NeuronClient/sound_library_3d_dsound.cpp:442` (NeuronClient) | Low — `.cpp` only |
| 1.6 | §3 | Split assignment-as-condition into two statements | `NeuronClient/inputdriver_xinput.cpp:484` (NeuronClient) | Low — `.cpp` only |
| 1.7 | §7 | Change `%s` → `%ls` for wide string | `NeuronClient/sound_library_3d_dsound.cpp:357` (NeuronClient) | Low — `.cpp` only |
| 1.8 | §7 | Remove extra `snprintf` argument | `Starstrike/taskmanager_interface_icons.cpp:1284` (Starstrike) | Low — `.cpp` only |
| 1.9 | §8 | Fix `HKL` pointer truncation | `NeuronClient/file_paths.cpp:57` (NeuronClient) | Low — `.cpp` only |
| 1.10 | §3b | Add missing `break`/`return` before `default:` in `writeExtraSpecInfo` | `NeuronClient/inputdriver_xinput.cpp:534` (NeuronClient) | Low — `.cpp` only |

**Verification:** Build Debug x64 + Release x64. Confirm 0 errors. Run the game, load a level, verify input works (including gamepad sensitivity parsing), sound plays, and UI text renders correctly.

---

### Phase 2: Build Configuration Alignment (Effort: Small, ~0.5 day)

All items are `.vcxproj` edits — no code changes. Group to minimize confusion.

| Step | Section | Action | Files | Build Impact |
|------|---------|--------|-------|--------------|
| 2.1 | §4 | Set `LanguageStandard` to `stdcpplatest` in Release for all projects. Also fix GameRender `Debug|Win32` (currently `stdcpp20`). | All 6 `.vcxproj` files | High — full rebuild of Release |
| 2.2 | §6 | Add `AdvancedVectorExtensions2` to Release `<ClCompile>` for all projects; add to both configs for GameRender | All 6 `.vcxproj` files | High — full rebuild of Release |
| 2.3 | §11 | Remove `#include <iostream>` from `inputfilter.h` (if unused) | `NeuronClient/inputfilter.h` | Medium — triggers NeuronClient rebuild |
| 2.4 | §12 | Uncomment `WIN32_LEAN_AND_MEAN` define | `NeuronCore/NeuronCore.h:65` | High — full solution rebuild (PCH change) |
| 2.5 | §5 | Remove orphaned `Debug|Win32` and `Release|Win32` configs from GameRender | `GameRender.vcxproj` | Low — no active build uses Win32 |

**Verification:** Full rebuild of both Debug x64 and Release x64. If 2.4 causes missing-header errors in sound or multimedia code, add explicit `#include <mmsystem.h>` to the affected `.cpp` files. Run the game, verify audio and input still work.

**Risk for 2.1:** Changing Release language standard may expose latent issues where C++23 semantics differ from C++20. Run a full playtest of Release after this change.

**Risk for 2.4:** `WIN32_LEAN_AND_MEAN` excludes `<mmsystem.h>`, `<shellapi.h>`, `<commdlg.h>`, and others from `<Windows.h>`. Sound code using `WAVEFORMATEX` or `PlaySound` may fail to compile. If 2.4 causes cascading errors, defer it to a separate mini-phase and add explicit includes to affected `.cpp` files first.

---

### Phase 3: Warning Infrastructure (Effort: Medium, ~1–2 days)

Must be done in order: fix existing GameRender warnings before raising `/W4`.

| Step | Section | Action | Files | Build Impact | Dependencies |
|------|---------|--------|-------|--------------|--------------|
| 3.1 | §5 | Fix the existing 8 GameRender warnings at `/W3` | Various `GameRender/*.cpp` | Low — `.cpp` only | None |
| 3.2 | §5 | Raise `GameRender` to `/W4` | `GameRender.vcxproj` | Medium — GameRender rebuild | 3.1 |
| 3.3 | §5 | Fix new warnings exposed by `/W4` in GameRender (~30–80 expected) | Various `GameRender/*.cpp` | Low — `.cpp` only | 3.2 |
| 3.4 | §17 | Remove/guard unreachable code in `renderer.cpp` (if DX12 Phase 6 > 2 months out) | `Starstrike/renderer.cpp:483–587` | Low — `.cpp` only | Check DX12 timeline |

**Verification:** Build Debug x64. Confirm GameRender warning count is 0 at `/W4`. Run the game, load a level with many building types to exercise GameRender code paths.

---

### Phase 4: Warning Reduction — Correctness (Effort: Medium, ~2–3 days)

These warnings indicate potential correctness bugs. Address before cosmetic warnings.

| Step | Section | Action | Files | Build Impact |
|------|---------|--------|-------|--------------|
| 4.1 | §20 | Fix variable shadowing (~15 warnings) | `landscape.cpp`, `taskmanager_interface_icons.cpp`, `location.cpp`, `global_world.cpp`, `renderer.cpp` | Low — `.cpp` only |
| 4.2 | §19 | Fix signed/unsigned mismatches (~100 warnings) | Across `NeuronClient`, `Starstrike`, `GameLogic` | Low — `.cpp` only |
| 4.3 | §14 | Replace `MIN` macro with `std::min` | `NeuronCore/net_lib.h` + networking `.cpp` files | Medium — `net_lib.h` is included by networking code |

**Verification:** Build Debug x64 + Release x64. Confirm warning count drops significantly. Run the game with multiplayer to verify networking (step 4.3).

---

### Phase 5: Warning Reduction — Cosmetic (Effort: Medium, ~2–3 days)

Lower-risk warnings. Can be parallelized across developers.

| Step | Section | Action | Files | Build Impact |
|------|---------|--------|-------|--------------|
| 5.1 | §18 | Clean up unused parameters (`[[maybe_unused]]` on virtual base methods) | Across all projects | Low — `.h` changes but low-fan-out headers |
| 5.2 | §18 | Delete unused locals | `camera.cpp`, `spawnpoint.cpp`, `sound_filter.cpp`, others | Low — `.cpp` only |
| 5.3 | §21 | Add `f` suffix to float literals (~20 warnings) | Across `NeuronClient`, `Starstrike` | Low — `.cpp` only |

**Verification:** Build Debug x64. Target: 0 remaining warnings (excluding C4244 — addressed separately in Phase 6).

---

### Phase 6: Test Infrastructure (Effort: Medium, ~2–3 days)

| Step | Section | Action | Files | Build Impact |
|------|---------|--------|-------|--------------|
| 6.1 | §16 | Create `NeuronCore.Tests` project (VS Native Unit Test Framework) | New `NeuronCore.Tests/NeuronCore.Tests.vcxproj` + `pch.h/cpp` | None — new project |
| 6.2 | §16 | Add tests for `GameMath.h` vector operations | New `NeuronCore.Tests/GameMathTests.cpp` | None — new file |
| 6.3 | §16 | Add tests for `GameVector3.h` `Load()`/`Store()` round-trips | New `NeuronCore.Tests/GameVector3Tests.cpp` | None — new file |
| 6.4 | §16 | Add tests for `Transform3D.h` accessors and matrix construction | New `NeuronCore.Tests/Transform3DTests.cpp` | None — new file |

**Verification:** Run all tests via Test Explorer. All green. These tests serve as a regression safety net for future changes.

---

### Phase 7: Large Migrations (Effort: Large, multi-sprint)

Each sub-phase is a self-contained migration with its own call-site inventory, migration strategy, and verification plan. Sub-phases are ordered by risk (lowest first) and can run in parallel where noted.

---

#### Phase 7.1: `auto_vector<T>` → `std::vector<std::unique_ptr<T>>` (Effort: Small, ~1 day)

**Rationale:** `auto_vector` is a third-party class (Reliable Software ©2003) that wraps `std::vector<T*>` with ownership semantics. Modern C++ provides `std::vector<std::unique_ptr<T>>` which is safer, better optimized, and universally understood.

**Scope:**

| Metric | Count |
|--------|-------|
| Template instantiations | 12 |
| Member declarations | 7 |
| Unique files | 13 |
| Project spread | NeuronClient only (+ 2 refs in GameRender headers) |

**Member declarations:**

| File | Declaration |
|------|-------------|
| `inputdriver_chord.h` | `auto_vector<InputSpecList> m_specs` |
| `inputdriver_conjoin.h` | `auto_vector<InputSpecList> m_specs` |
| `inputdriver_pipe.h` | `auto_vector<InputFilterWithArgs> m_specs` |
| `inputdriver_prefs.h` | `auto_vector<std::string> m_keys` |
| `inputfilter_withdelta.h` | `auto_vector<const InputFilterSpec> m_specs` |
| `inputfilter_withdelta.h` | `auto_vector<InputDetails> m_oldDetails` |
| `inputfilter_withdelta.h` | `auto_vector<InputDetails> m_details` |

**API mapping:**

| `auto_vector` | `std::vector<std::unique_ptr<T>>` |
|---------------|-----------------------------------|
| `push_back(std::unique_ptr<T>)` | `push_back(std::move(ptr))` |
| `operator[](i)` → `T*` | `vec[i].get()` |
| `assign(i, std::unique_ptr<T>)` | `vec[i] = std::move(ptr)` |
| `assign_direct(i, T*)` | `vec[i].reset(ptr)` |
| `erase(idx)` | `vec.erase(vec.begin() + idx)` |
| `size()` | `vec.size()` |
| `clear()` | `vec.clear()` |
| `begin()`/`end()` | Iterator derefs yield `unique_ptr<T>&` — callers using `*it` to get `T*` need `.get()` |
| `pop_back()` → `unique_ptr<T>` | `auto p = std::move(vec.back()); vec.pop_back();` |

**Steps:**

| Step | Action | Files |
|------|--------|-------|
| 7.1.1 | Replace `auto_vector<T>` member types with `std::vector<std::unique_ptr<T>>` in all 7 header declarations | 5 `.h` files |
| 7.1.2 | Update all call sites (13 files) — `push_back`, `operator[]`, `assign`, `erase`, iterator patterns | 8 `.cpp` + 5 `.h` files |
| 7.1.3 | Delete `auto_vector.h` and remove from `NeuronClient.vcxproj` | 1 file |
| 7.1.4 | Build Debug x64 + Release x64. Run the game, test keyboard/gamepad rebinding (exercises input driver/filter code) | — |

**Risk:** Low. Confined to input subsystem. No cross-project data flow.

---

#### Phase 7.2: `HashTable<T>` / `SortingHashTable<T>` → STL (Effort: Small–Medium, ~1–2 days)

**Rationale:** `HashTable<T>` uses `char*` keys with a custom hash, open addressing, and manual memory management. `SortingHashTable<T>` adds ordered iteration via an index chain. STL replacements are faster, safer, and well-tested.

**Scope:**

| Metric | `HashTable` | `SortingHashTable` |
|--------|-------------|--------------------|
| Template instantiations (outside own header) | 17 | 1 |
| Member declarations | 4 (in `resource.h`) + 3 (in `language_table.h`, `preferences.h`, `sample_cache.h`) | 1 (in `profiler.h`) |
| Unique consumer files | 7 | 2 |
| Project spread | NeuronClient only | NeuronClient only |

**STL mapping:**

| Custom | Replacement | Notes |
|--------|-------------|-------|
| `HashTable<T>` | `std::unordered_map<std::string, T>` | `char*` keys → `std::string` keys. `GetIndex()`/`ValidIndex()` patterns disappear. |
| `SortingHashTable<T>` | `std::map<std::string, T>` | Ordered iteration is built-in. `StartOrderedWalk()`/`GetNextOrderedIndex()` → range-for. |

**Steps:**

| Step | Action | Files |
|------|--------|-------|
| 7.2.1 | Migrate `SortingHashTable` (leaf) — only used in `profiler.h` | `profiler.h`, `profiler.cpp` |
| 7.2.2 | Migrate `HashTable` in `resource.h` (4 static members) | `resource.h`, `resource.cpp` |
| 7.2.3 | Migrate `HashTable` in `language_table.h` (2 members + helper functions) | `language_table.h`, `language_table.cpp` |
| 7.2.4 | Migrate `HashTable` in `preferences.h` and `sample_cache.h` | `preferences.h`, `preferences.cpp`, `sample_cache.h`, `sample_cache.cpp` |
| 7.2.5 | Delete `hash_table.h`, `sorting_hash_table.h` and remove from project | 2 files |
| 7.2.6 | Build + verify. Run the game, check resource loading (textures, shapes), language strings, preferences save/load, sound sample cache | — |

**Risk:** Medium. `resource.h` uses `HashTable<int>` for OpenGL display lists and textures — these are GL-layer code that will be removed by DX12 migration. Consider whether to migrate or defer to DX12 Phase 6. `language_table` and `preferences` are stable and worth migrating now.

---

#### Phase 7.3: `using namespace` Removal from `NeuronCore.h` (Effort: Medium, ~2–3 days)

**Rationale:** `NeuronCore.h` (included by every PCH) has `using namespace winrt;` and `using namespace Neuron;` at file scope, polluting the global namespace in all 308 `.cpp` files.

**Scope:**

| Namespace | Unqualified usage sites | Already-qualified sites | Files with explicit `using namespace` |
|-----------|------------------------|------------------------|--------------------------------------|
| `winrt` | ~42 (`com_ptr`: 17, `hstring`: 6, `handle`: 19) | Rare | 0 `.cpp` files (all comes from header) |
| `Neuron` | ~225 (`Transform3D`: 83, `DebugTrace`: 66, `GameVector3`: 38, `SpriteBatch`: 19, `GameMatrix`: 11, `Fatal`: 8) | ~33 (`Transform3D`: 31, `Fatal`: 2) | 8 `.cpp` files already have explicit `using namespace Neuron` |

**Strategy:** Two-pass approach. `winrt` is smaller scope and lower risk — do it first.

**Steps:**

| Step | Action | Files | Effort |
|------|--------|-------|--------|
| 7.3.1 | Remove `using namespace winrt;` from `NeuronCore.h`. Add `winrt::` qualifier to ~42 unqualified usages (`com_ptr`, `hstring`, `handle`) | ~15 files across NeuronCore, NeuronClient, GameRender | Small (~0.5 day) |
| 7.3.2 | Remove `using namespace Neuron;` from `NeuronCore.h`. Add `using namespace Neuron;` to each `.cpp` file that uses `Neuron::` types unqualified, OR qualify all ~225 usages | ~60–80 `.cpp` files | Medium (~1.5–2 days) |
| 7.3.3 | Build + run full test suite (55 tests). Run the game, exercise rendering/math/input paths | — | — |

**Decision point for 7.3.2:** Two strategies:
- **Option A — Per-file `using namespace Neuron;`**: Add the directive to each `.cpp` that needs it. Fastest, lowest risk, but doesn't eliminate the pattern — just moves it from global to per-TU.
- **Option B — Full qualification**: Replace all `DebugTrace(...)` with `Neuron::DebugTrace(...)`, etc. Cleaner long-term but ~225 edits across 60+ files.

**Recommendation:** Option A for initial migration (mechanical, low risk). Option B can be done incrementally per-file as files are touched for other work.

**Risk:** Medium. Ambiguity bugs are unlikely since `winrt` and `Neuron` namespaces don't currently overlap, but the change touches every project's compilation. Full rebuild required.

---

#### Phase 7.4: `LList<T>` → `std::vector<T>` (Effort: Large, ~1–2 weeks)

**Rationale:** `LList` is the most pervasive custom container — a doubly-linked list with index-based access (O(n) random access with a sequential-read cache). Modern `std::vector` provides better cache locality and O(1) random access. The index-based API (`GetData(i)`, `operator[]`, `ValidIndex(i)`) maps naturally to `std::vector`.

**Scope:**

| Metric | Count |
|--------|-------|
| Template instantiations (outside header) | 120 |
| Member declarations | 59 |
| Unique consumer files | 71 |
| Project spread | GameLogic (52), NeuronClient (63), GameRender (13), Starstrike (62) |
| API call sites (approximate) | `GetData`: 607, `Size`: 586, `PutData`: 269, `ValidIndex`: 240, `RemoveData`: 54, `EmptyAndDelete`: 51, `Empty`: 33, `PutDataAtStart`: 18, `PutDataAtEnd`: 16 |

**API mapping:**

| `LList<T>` | `std::vector<T>` | Notes |
|-------------|-------------------|-------|
| `PutData(x)` / `PutDataAtEnd(x)` | `push_back(x)` | |
| `PutDataAtStart(x)` | `insert(begin(), x)` | O(n) — but only 18 call sites, verify perf |
| `PutDataAtIndex(x, i)` | `insert(begin() + i, x)` | |
| `GetData(i)` | `vec[i]` or `vec.at(i)` | 607 call sites — bulk of the work |
| `GetPointer(i)` | `&vec[i]` | |
| `RemoveData(i)` | `erase(begin() + i)` | ⚠️ Invalidates indices > i. Verify callers handle this. |
| `FindData(x)` | `std::find(begin(), end(), x) - begin()` | Returns index; -1 if not found |
| `Size()` | `size()` (returns `size_t`) | ⚠️ 586 call sites — signed/unsigned mismatch likely. Use `static_cast<int>(size())` or wrapper. |
| `ValidIndex(i)` | `i >= 0 && i < static_cast<int>(size())` | 240 call sites — may need helper or `std::ssize` |
| `Empty()` | `clear()` | |
| `EmptyAndDelete()` | Loop `delete` then `clear()`, or switch to `std::vector<std::unique_ptr<T>>` | 51 call sites — ownership pattern |
| `operator[](i)` | `operator[](i)` | Direct replacement |

**⚠️ Critical migration risk — `RemoveData` index invalidation:**
`LList::RemoveData(i)` removes the node at index `i`. Because `LList` is a linked list, other indices shift (items after `i` get index-1). `std::vector::erase` has the same shift behavior, so this is compatible. However, code that removes items during forward iteration (`for (int i = 0; i < list.Size(); ++i) { if (...) list.RemoveData(i); }`) will skip the next element — this bug exists in both `LList` and `std::vector` but must be audited during migration.

**⚠️ Critical migration risk — `EmptyAndDelete` ownership:**
51 call sites use `EmptyAndDelete()`, which calls `delete` on each element. This indicates raw-pointer ownership. Ideal migration target is `std::vector<std::unique_ptr<T>>` for these, but that changes the element type from `T*` to `std::unique_ptr<T>` and cascades to all access patterns. **Recommended approach:** First pass keeps `std::vector<T*>` + explicit delete loop. Second pass migrates to `unique_ptr` ownership per-class as time permits.

**Steps:**

| Step | Action | Files | Effort |
|------|--------|-------|--------|
| 7.4.1 | Create `LListCompat<T>` adapter: a `std::vector<T>` wrapper exposing the `LList` API (`GetData`, `PutData`, `Size`, `ValidIndex`, `RemoveData`, `Empty`, `EmptyAndDelete`). This allows incremental migration — swap typedef, build, verify. | New `NeuronClient/llist_compat.h` | Small |
| 7.4.2 | Migrate **GameLogic** `LList` usages (52 refs, standalone project, no rendering dependencies) — swap to `LListCompat<T>` | ~15 GameLogic files | Medium |
| 7.4.3 | Migrate **GameRender** `LList` usages (13 refs) | ~8 GameRender files | Small |
| 7.4.4 | Migrate **NeuronClient** `LList` usages (63 refs) | ~25 NeuronClient files | Medium |
| 7.4.5 | Migrate **Starstrike** `LList` usages (62 refs) | ~25 Starstrike files | Medium |
| 7.4.6 | Replace `LListCompat<T>` with direct `std::vector<T>` (remove adapter layer) | All migrated files | Medium |
| 7.4.7 | Delete `llist.h` and remove from project | 1 file | Trivial |
| 7.4.8 | Build + full test suite. Run the game, load all level types, verify entities/buildings/effects/sound | — | — |

**Risk:** High. Largest migration by call-site count. The adapter strategy (7.4.1) mitigates risk by allowing incremental swap-and-verify per project.

---

#### Phase 7.5: `DArray<T>` Family → STL (Effort: Large, ~1–2 weeks)

**Rationale:** `DArray<T>` is a sparse array with a shadow bitmap tracking used/unused slots. `FastDArray<T>` adds a free-list for O(1) insert. `SliceDArray<T>` adds per-frame iteration slicing. These have no direct STL equivalent — a custom adapter is required.

**Scope:**

| Container | Instantiations | Member decls | Files | Projects |
|-----------|---------------|--------------|-------|----------|
| `DArray<T>` | 38 | 9 | 12 | NeuronClient, GameLogic |
| `FastDArray<T>` | 10 | 8 | 8 | NeuronClient, Starstrike |
| `SliceDArray<T>` | 5 | 4 | 2 | NeuronClient, Starstrike |

**Migration strategy:**

The slot-tracking and free-list semantics don't map to any single STL container. Two options:

- **Option A — Sparse set**: Replace with a custom `SparseArray<T>` built on `std::vector<T>` + `std::vector<bool>` (shadow) + `std::vector<int>` (free-list). Same API, modern internals. Eliminates `new[]`/`delete[]` manual memory, gains cache-friendly layout.
- **Option B — `std::vector<std::optional<T>>`**: Uses `std::optional` as the "used/unused" indicator. Simpler, but wastes memory for large `T` (stores `T` even when unused) and no free-list.

**Recommendation:** Option A. The `SparseArray<T>` adapter preserves the exact semantics while modernizing the internals. The `SliceDArray` slicing feature should be a separate concern (iterator adapter or view) rather than baked into the container.

**Steps:**

| Step | Action | Files | Effort |
|------|--------|-------|--------|
| 7.5.1 | Design and implement `SparseArray<T>` (replaces `DArray`/`FastDArray`) with unit tests | New `NeuronCore/SparseArray.h` + tests | Medium |
| 7.5.2 | Design slicing adapter for `SliceDArray` semantics (or inline the pattern at call sites) | Design doc | Small |
| 7.5.3 | Migrate `DArray` usages (mostly `soundsystem.h`) | 12 files | Medium |
| 7.5.4 | Migrate `FastDArray` usages (`location.h`, `team.h`, `landscape_renderer.h`, `water.h`) | 8 files | Medium |
| 7.5.5 | Migrate `SliceDArray` usages (`location.h`, `team.h`, `particle_system.h`, `unit.h`) | 4 files | Small |
| 7.5.6 | Delete `darray.h`, `fast_darray.h`, `slice_darray.h` and remove from project | 3 files | Trivial |
| 7.5.7 | Build + full test suite. Stress-test: load a level with many entities (exercises `SliceDArray<Entity*>` iteration) | — | — |

**Risk:** High. `SliceDArray<Entity*>` in `team.h` and `SliceDArray<Spirit>` / `SliceDArray<Laser>` in `location.h` are iterated every frame — performance-critical. Profile before and after.

**Dependency:** Phase 7.4 (`LList` migration) should complete first to reduce simultaneous container churn.

---

#### Phase 7 Summary

| Sub-phase | Target | Files | Effort | Risk | Can parallelize with |
|-----------|--------|-------|--------|------|---------------------|
| 7.1 | `auto_vector` → `std::vector<unique_ptr>` | 13 | ~1 day | Low | Any |
| 7.2 | `HashTable`/`SortingHashTable` → STL maps | 9 | ~1–2 days | Medium | 7.1, 7.3 |
| 7.3 | `using namespace` removal | ~80 | ~2–3 days | Medium | 7.1, 7.2 |
| 7.4 | `LList` → `std::vector` | 71 | ~1–2 weeks | High | After 7.1–7.3 |
| 7.5 | `DArray` family → `SparseArray` | 22 | ~1–2 weeks | High | After 7.4 |

> **Note:** Unsafe string elimination (formerly 7.6) has been moved to a dedicated plan: [`string.md`](string.md). It can run in parallel with container migrations (7.1–7.5).

**Recommended execution order:** 7.1 → 7.2 → 7.3 → 7.4 → 7.5 (∥ `string.md`)

---

### Phase 8: Regression Prevention & Cleanup (Effort: Small, ~1 day)

Lock in the zero-warning state and clean up residual inconsistencies.

| Step | Section | Action | Files | Build Impact | Dependencies |
|------|---------|--------|-------|--------------|------|
| 8.1 | — | Fix `docs/architecture.md` line 57: change `ComPtr<>` → `winrt::com_ptr<>` to match `copilot-instructions.md` | `docs/architecture.md` | None — documentation only | None |
| 8.2 | — | ~~Remove orphaned Win32 configurations from `GameRender.vcxproj`~~ | `GameRender.vcxproj` | — | ✅ Done in Phase 2 (step 2.5) |

**Verification:** Full rebuild of Debug x64 + Release x64. Verify `docs/architecture.md` is consistent with `copilot-instructions.md`.

---

## ✅ Success Criteria

- [x] Zero C5205 (UB: non-virtual destructor) warnings
- [x] Zero C4701 (uninitialized variable) warnings
- [x] Zero C4706 (assignment as condition) warnings
- [x] Zero C4474/C4477 (format string) warnings
- [x] Zero C4311/C4302 (pointer truncation) warnings
- [x] Switch fallthrough bug in `inputdriver_xinput.cpp` fixed (§3b)
- [x] Debug and Release use the same C++ language standard across all 6 projects
- [x] AVX2 enabled in both Debug and Release for **all 6 projects** (including GameRender)
- [x] All projects compile at `/W4`
- [x] `NeuronCore.Tests` project exists with passing math function tests (55 tests, 3 classes)
- [x] NeuronServer included in all build configuration fixes (language standard, AVX2, C4244)
- [x] `docs/architecture.md` consistent with `copilot-instructions.md` (`winrt::com_ptr`, not `ComPtr`)
- [x] GameRender orphaned Win32 configurations removed
- [ ] Game builds and runs correctly in both Debug x64 and Release x64 after every phase
