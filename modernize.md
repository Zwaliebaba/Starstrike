# Starstrike Modernization Recommendations

## Tech Stack Summary

| Aspect | Value |
|--------|-------|
| C++ Standard | `stdcpplatest` (all configurations) |
| Platform Toolset | v145 (MSVC 14.50) |
| Graphics API | DirectX 12 (via OpenGL shim layer) |
| NuGet Dependencies | CppWinRT 2.0, WindowsAppSDK 1.8, WinPixEventRuntime 1.0 |
| Architecture | 6 static libraries + 1 executable |
| Build Warnings | **259** (0 errors) — down from 425 |

---

## 📊 Current Status

> Last verified: full solution Rebuild (Debug x64).

| Phase | Status | Summary |
|-------|--------|---------|
| Phase 1: Critical Bug Fixes | ✅ Complete | All 10 steps done + bonus `InputTransform` virtual dtor |
| Phase 2: Build Config Alignment | ✅ Complete | All 5 steps done |
| Phase 3: Warning Infrastructure | ✅ Complete | GameRender raised to `/W4`, 0 warnings |
| Phase 4: Warning Reduction — Correctness | 🔄 Partial | Shadowing/signed-unsigned fixed in some Starstrike files. `MIN` macro replaced. GameLogic & most NeuronClient untouched. |
| Phase 5: Warning Reduction — Cosmetic | 🔄 Partial | Unused params/locals/float literals fixed in ~15 Starstrike files. GameLogic & most NeuronClient untouched. |
| Phase 6: Re-enable C4244 | ⬜ Not Started | |
| Phase 7: Test Infrastructure | ⬜ Not Started | |
| Phase 8: Large Migrations | ⬜ Not Started | Planning only |
| Phase 9: Regression Prevention | ⬜ Not Started | Step 9.3 (Win32 configs) already done in Phase 2 |

### Warning Counts (Full Rebuild Debug x64)

| Project | Original | Current | Delta |
|---------|----------|---------|-------|
| NeuronClient | 179 | 158 | −21 |
| GameLogic | 79 | 79 | 0 |
| Starstrike | 159 | 22 | −137 |
| GameRender | 8 | 0 | −8 |
| NeuronCore | 0 | 0 | — |
| NeuronServer | 0 | 0 | — |
| **Total** | **425** | **259** | **−166 (−39%)** |

> **Note:** 19 of the 158 NeuronClient warnings are in `opengl_directx.cpp` (all C4100), intentionally skipped per the DX12 coordination note. The "actionable" NeuronClient count is **139**.

---

## 📎 Related Plans

| Plan | Location | Status | Overlap |
|------|----------|--------|---------|
| DX12 Migration | `GameRender/DX12Migration.md` | Phases 0–2 ✅, Phases 3–6 planned | §17 unreachable code in `renderer.cpp` will be removed by Phase 6. §21 `glColor4f` double literals are in GL call sites being eliminated. §19 signed/unsigned mismatches in GL renderers will disappear as renderers are rewritten. |
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

`NeuronCore/NeuronCore.h:13` globally disables C4244:
```cpp
#pragma warning(disable:4244) // conversion from 'x' to 'y', possible loss of data
```

This header is included by every `pch.h` in the solution. **C4244 is the very class of warning** that §8 (pointer truncation), §19 (signed/unsigned), and §21 (double→float) are trying to fix. The current 425-warning count is artificially low — re-enabling C4244 will expose *far more* warnings.

**This is a blocker:** Fixes to §19 and §21 will not be verified by the compiler while C4244 is suppressed. The other disabled warnings (`C4201`, `C4238`, `C4324`) are less harmful but should also be reviewed.

**Fix:** Phase the re-enablement:
1. Fix all known C4244 sites in a single project (start with `GameLogic` — smallest at 79 warnings)
2. Re-enable C4244 for that project only (move the `#pragma` to per-project `pch.h` files instead of `NeuronCore.h`)
3. Repeat for each project
4. Remove the global disable from `NeuronCore.h` last

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

---

## 🟡 Build Warning Summary (259 remaining)

> **Note:** The 259 count is still artificially low because C4244 is globally disabled (§9). Re-enabling C4244 will expose additional warnings. GameRender is now at `/W4` (§5 fixed).

### Warning Distribution by Project

| Project | Warnings |
|---------|----------|
| NeuronClient | 179 |
| Starstrike | 159 |
| GameLogic | 79 |
| GameRender | 8 |
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

---

### 15. Unsafe C String Functions

Widespread use of `strlen`, `sprintf`, `snprintf` with raw `char*` buffers throughout the legacy codebase. The `_CRT_SECURE_NO_WARNINGS` define in `NeuronCore.h:3` suppresses security warnings globally.

The existing TODO comment says:
```cpp
#define _CRT_SECURE_NO_WARNINGS  // TODO [CI §1.7]: Remove after §1.1 (unsafe string elimination) is complete
```

**Recommendation:** Track this as a work item. Migrate hot paths to `std::format` / `std::string` (already used in `NeuronCore`). Fix the format-string bugs from §7 immediately.

---

### 16. No Unit Tests

The solution has **zero test projects**. The `copilot-instructions.md` mentions using the Visual Studio Native Unit Test Framework, but no tests exist.

**Recommendation:** Start with unit tests for `NeuronCore` math functions:
- `GameMath.h` — vector operations (`Normalize`, `Cross`, `Dot`, `RotateAround`, etc.)
- `GameVector3.h` — storage type construction, `Load()`/`Store()` round-trips
- `Transform3D.h` — matrix construction, `Right()`/`Up()`/`Forward()`/`Pos()` accessors

These are pure logic with no external dependencies and are critical for correctness.

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

> 🔄 **Partially Fixed** — Cleaned up in ~15 Starstrike files (`camera.cpp`, `script.cpp`, `taskmanager.cpp`, `WinMain.cpp`, etc.) and several NeuronClient files (`sound_instance.cpp`, `soundsystem.cpp`). **125 C4100 + 32 C4189 = 157 remain**, mostly in GameLogic and NeuronClient base-class virtual methods.


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

> 🔄 **Partially Fixed** — Fixed in `taskmanager_interface_icons.cpp` (`captionId`, `zone`, `boxX`, `boxY`, `boxH`, `i`), `global_world.cpp` (`levFile`, `i`), `level_file.cpp` (`i`). **17 remain** (15 C4456 + 2 C4457), mostly `landscape.cpp` (8× `segStart`/`segEnd`) and NeuronClient files.


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

### Phase 6: Re-enable C4244 (Effort: Medium–Large, ~3–5 days)

**Blocker awareness:** This phase cannot produce verified results until C4244 is re-enabled per-project. Must be done project-by-project to keep the warning count manageable.

| Step | Section | Action | Files | Build Impact | Dependencies |
|------|---------|--------|-------|--------------|-------|
| 6.1 | §9 | Move `#pragma warning(disable:4244)` from `NeuronCore.h` to each project's `pch.h` | All `pch.h` files + `NeuronCore/NeuronCore.h` | High — full rebuild | None |
| 6.2 | §9 | Remove `#pragma warning(disable:4244)` from `GameLogic/pch.h`, fix exposed warnings | `GameLogic/pch.h` + various `GameLogic/*.cpp` | Medium — GameLogic rebuild | 6.1 |
| 6.3 | §9 | Remove from `GameRender/pch.h`, fix exposed warnings | `GameRender/pch.h` + various `GameRender/*.cpp` | Medium — GameRender rebuild | 6.1 |
| 6.4 | §9 | Remove from `NeuronClient/pch.h`, fix exposed warnings | `NeuronClient/pch.h` + various `NeuronClient/*.cpp` | Medium — NeuronClient rebuild | 6.1 |
| 6.5 | §9 | Remove from `NeuronServer/pch.h`, fix exposed warnings | `NeuronServer/pch.h` + various `NeuronServer/*.cpp` | Medium — NeuronServer rebuild | 6.1 |
| 6.6 | §9 | Remove from `Starstrike/pch.h`, fix exposed warnings | `Starstrike/pch.h` + various `Starstrike/*.cpp` | Medium — Starstrike rebuild | 6.1 |
| 6.7 | §9 | Remove from `NeuronCore/pch.h` — C4244 now enabled everywhere | `NeuronCore/pch.h` | Medium — NeuronCore rebuild | 6.2–6.6 |

> **Parallelism:** After step 6.1, steps 6.2–6.6 are **independent** and can be done in parallel across developers. Only 6.7 (NeuronCore itself) must wait for all others, since NeuronCore warnings propagate through dependents.

**Verification:** After each sub-step, build the affected project and confirm 0 C4244 warnings. After 6.7, full rebuild of Debug x64 + Release x64 with 0 warnings.

---

### Phase 7: Test Infrastructure (Effort: Medium, ~2–3 days)

Can be started in parallel with Phases 3–6.

| Step | Section | Action | Files | Build Impact |
|------|---------|--------|-------|--------------|
| 7.1 | §16 | Create `NeuronCore.Tests` project (VS Native Unit Test Framework) | New `NeuronCore.Tests/NeuronCore.Tests.vcxproj` + `pch.h/cpp` | None — new project |
| 7.2 | §16 | Add tests for `GameMath.h` vector operations | New `NeuronCore.Tests/GameMathTests.cpp` | None — new file |
| 7.3 | §16 | Add tests for `GameVector3.h` `Load()`/`Store()` round-trips | New `NeuronCore.Tests/GameVector3Tests.cpp` | None — new file |
| 7.4 | §16 | Add tests for `Transform3D.h` accessors and matrix construction | New `NeuronCore.Tests/Transform3DTests.cpp` | None — new file |

**Verification:** Run all tests via Test Explorer. All green. These tests serve as a regression safety net for Phases 4–6.

---

### Phase 8: Large Migrations — Planning Only (Effort: Large, multi-sprint)

These are too large for inline fixes. Each needs its own implementation plan document.

| Item | Section | Deliverable | Dependencies |
|------|---------|-------------|--------------|
| 8.1 | §13 | Container migration plan: call-site counts per container, migration order (leaves first), adapter strategy | Phase 5 complete (clean warning baseline) |
| 8.2 | §15 | Unsafe string elimination plan: audit `sprintf`/`strlen`/`snprintf` call sites, prioritize by security risk, migration to `std::format` | Phase 6 complete (C4244 re-enabled) |
| 8.3 | §10 | `using namespace` removal plan: grep for `winrt::` and `Neuron::` qualified vs. unqualified usage, estimate per-project effort | Phase 7 complete (tests provide safety net) |

---

### Phase 9: Regression Prevention & Cleanup (Effort: Small, ~1 day)

Lock in the zero-warning state and clean up residual inconsistencies.

| Step | Section | Action | Files | Build Impact | Dependencies |
|------|---------|--------|-------|--------------|------|
| 9.1 | — | Enable `/WX` (treat warnings as errors) in all projects for both Debug and Release | All 6 `.vcxproj` files | None — no new warnings if Phases 1–6 are complete | Phases 1–6 complete (0 warnings) |
| 9.2 | — | Fix `docs/architecture.md` line 57: change `ComPtr<>` → `winrt::com_ptr<>` to match `copilot-instructions.md` | `docs/architecture.md` | None — documentation only | None |
| 9.3 | — | ~~Remove orphaned Win32 configurations from `GameRender.vcxproj`~~ | `GameRender.vcxproj` | — | ✅ Done in Phase 2 (step 2.5) |

**Verification:** Full rebuild of Debug x64 + Release x64. Confirm `/WX` is active — any new warning will now break the build. Verify `docs/architecture.md` is consistent with `copilot-instructions.md`.

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
- [ ] `/WX` (treat warnings as errors) enabled in all projects
- [ ] C4244 re-enabled globally (no `#pragma warning(disable:4244)`)
- [ ] Total warning count: **0** across all projects and configurations
- [ ] `NeuronCore.Tests` project exists with passing math function tests
- [x] NeuronServer included in all build configuration fixes (language standard, AVX2, C4244)
- [ ] `docs/architecture.md` consistent with `copilot-instructions.md` (`winrt::com_ptr`, not `ComPtr`)
- [x] GameRender orphaned Win32 configurations removed
- [ ] Game builds and runs correctly in both Debug x64 and Release x64 after every phase
