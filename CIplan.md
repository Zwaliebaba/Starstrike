# Starstrike — Codebase Improvement Implementation Plan

> Step-by-step execution guide derived from `CI.md`.  Each step lists exact
> files, exact edits, build verification, and runtime smoke-test expectations.
>
> **Constraint:** Every step must leave the game in a buildable, runnable state.
>
> **Conventions:**
> - Build commands: `MSBuild Starstrike.slnx /p:Configuration=Debug /p:Platform=x64`
>   and `/p:Configuration=Release` — both must pass at every phase boundary.
> - Legacy projects (GameLogic, NeuronClient) keep raw pointers and C-style strings
>   unless a file is being substantially rewritten — see `CI.md` Exclusions.
> - `pch.h` is always the first `#include` in new `.cpp` files.

---

## Table of Contents

1. [Phase -1 — Active UB Hotfixes](#phase--1--active-ub-hotfixes)
2. [Phase 0 — Quick Wins](#phase-0--quick-wins)
3. [Phase 4.2 — Dead Preprocessor Branches (early)](#phase-42--dead-preprocessor-branches-early)
4. [Phase 7-Early — Build Config Fixes](#phase-7-early--build-config-fixes)
5. [Phase 1 — Security & Stability](#phase-1--security--stability)
6. [Phase 2 — Memory Safety](#phase-2--memory-safety)
7. [Phase 7-Remaining — Build Hygiene](#phase-7-remaining--build-hygiene)
8. [Phase 4-Remaining — Dead Code Cleanup](#phase-4-remaining--dead-code-cleanup)
9. [Phase 3 — Type Safety & Modern Idioms](#phase-3--type-safety--modern-idioms)
10. [Phase 5 — Code Duplication](#phase-5--code-duplication)
11. [Phase 6 — Performance](#phase-6--performance)

---

## Rollback Policy

- If a phase gate produces **> 50 new errors/warnings**, pause and
  re-scope: split the step into per-project sub-steps.
- If a step causes a **runtime regression** not caught by build, revert
  the commit immediately and add a regression test before retrying.
- **Never force-push** — all reverts should be forward commits with
  `revert:` prefix so the history documents the failure.

---

## Parallelism Opportunities

The following steps can run concurrently if assigned to different developers:

| Track A | Track B | Track C |
|---------|---------|---------|
| §1.1b GameLogic strings | §1.1c NeuronClient strings | §1.1d Starstrike strings |
| §4.1 GameLogic commented code | §4.3 Starstrike `camera.cpp` | §4.4 Starstrike `water.cpp` |
| §3.1b-d NeuronClient-only enums | §3.2 NeuronCore attributes | §3.5 Template DataWriter |

> Do **not** parallelize steps that modify pch-chain headers
> (`NeuronClient.h`, `NeuronCore.h`) — these trigger full rebuilds and
> merge conflicts.

---

## Phase -1 — Active UB Hotfixes

**Effort:** < 1 hour &nbsp;|&nbsp; **Risk:** SAFE — single-line fixes
**Commit message pattern:** `fix(UB): <description> [CI §-1.N]`

### Step -1.1 — Fix `delete`/`delete[]` mismatch in `EclButton`

**CI ref:** §-1.1 &nbsp;|&nbsp; **Category:** BUG

**File:** `NeuronClient/eclbutton.cpp` (lines 21-23)

**Current code:**
```cpp
EclButton::~EclButton()
{
  if (m_caption)
    delete m_caption;
  if (m_tooltip)
    delete m_tooltip;
}
```

**Change to:**
```cpp
EclButton::~EclButton()
{
  if (m_caption)
    delete[] m_caption;
  if (m_tooltip)
    delete[] m_tooltip;
}
```

**Why:** `m_caption` and `m_tooltip` are allocated with `new char[…]` in
`SetCaption()` (line 49) and `SetTooltip()` (line 65).  Using scalar
`delete` on array `new[]` is undefined behaviour.  `SetCaption` and
`SetTooltip` already use `delete[]` internally — only the destructor is
wrong.

**Build impact:** Low — modifies one `.cpp` file.
**Verification:** Build Debug|x64, Release|x64.  Launch app, verify no
debug-heap assertion on window close.

---

### Step -1.2 — Fix format-string injection in `startsequence.cpp`

**CI ref:** §-1.2 &nbsp;|&nbsp; **Category:** BUG

**File:** `Starstrike/startsequence.cpp` (line 135)

**Current code:**
```cpp
sprintf(theString, caption->m_caption);
```

**Change to:**
```cpp
snprintf(theString, sizeof(theString), "%s", caption->m_caption);
```

**Why:** `caption->m_caption` is user-visible text (`strdup`'d from data).
Any `%` character causes `sprintf` to read garbage from the stack.  Adding
`"%s"` as the format string and bounding with `snprintf` fixes both the
injection and the overflow risk.

**Build impact:** Low — one `.cpp` file.
**Verification:** Build both configs.  Run the start sequence — captions
should render identically.

---

### Step -1.3 — Fix unused `errorText` in `Fatal()`

**CI ref:** §-1.3 &nbsp;|&nbsp; **Category:** BUG

**File:** `NeuronCore/Debug.h` (two `Fatal()` overloads, lines ~39 and ~46)

**Current code (narrow):**
```cpp
template <class... Types>
void __declspec(noreturn) Fatal(const std::string_view _fmt, Types&&... _args)
{
  auto errorText = vformat(_fmt, std::make_format_args(_args...));
  __debugbreak();
  throw std::exception("Fatal Error");
}
```

**Change to (narrow):**
```cpp
template <class... Types>
void __declspec(noreturn) Fatal(const std::string_view _fmt, Types&&... _args)
{
  auto errorText = vformat(_fmt, std::make_format_args(_args...));
  OutputDebugStringA(errorText.c_str());
  __debugbreak();
  throw std::exception(errorText.c_str());
}
```

**Repeat for the wide overload:**
```cpp
template <class... Types>
void __declspec(noreturn) Fatal(const std::wstring_view _fmt, Types&&... _args)
{
  auto errorText = vformat(_fmt, std::make_wformat_args(_args...));
  OutputDebugStringW(errorText.c_str());
  __debugbreak();
  // std::exception only takes narrow strings — use winrt::to_string
  // (already used elsewhere in the codebase, e.g. resource.cpp)
  throw std::exception(winrt::to_string(errorText).c_str());
}
```

> ⚠️ **Do not** use `std::string(errorText.begin(), errorText.end())` —
> this truncates non-ASCII characters to garbage.  The codebase already
> uses `winrt::to_string()` (see `resource.cpp`) which performs proper
> UTF-16 → UTF-8 conversion.
>
> **Also note:** The `ASSERT` macro uses `_CRT_WIDE("Assert Failure")`
> which routes through the wide overload.  Every assertion failure hits
> this path, so the conversion must not throw.  `winrt::to_string()` is
> noexcept-safe for short strings allocated on the stack.

**Why:** `errorText` was computed but discarded — `Fatal` always threw
`"Fatal Error"` with no context.  Now the actual formatted message appears
in the debugger Output window and in the exception.

**Build impact:** MEDIUM — `Debug.h` is included by every project via
`NeuronCore.h` → `pch.h`.  Expect a full rebuild.
**Verification:** Build both configs.  Trigger an `ASSERT(false)` in a
debug build — confirm the Output window shows the formatted message.

---

### Phase -1 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ Launch app, load a level, verify rendering + input
```

---

## Phase 0 — Quick Wins

**Effort:** 1 day &nbsp;|&nbsp; **Risk:** SAFE
**Commit message pattern:** `cleanup: <description> [CI §0.N]`

### Step 0.1 — `NULL` → `nullptr` (all projects)

**CI ref:** §0.2 &nbsp;|&nbsp; **Category:** CLEANUP

This is a mechanical find-and-replace.  Process one project at a time to
keep diffs reviewable.

#### Step 0.1a — NeuronCore (2 files)

| File | Line(s) | Change |
|------|---------|--------|
| `net_socket_listener.cpp` | 43 | `(NetCallBack)NULL` → `nullptr` |
| `net_thread_win32.cpp` | 10 | 6× `NULL` → `nullptr` |

**Build:** Incremental NeuronCore only.

#### Step 0.1b — NeuronClient (4 files)

| File | Line(s) | Change |
|------|---------|--------|
| `NeuronClient.h` | 138-141 | `SAFE_FREE`/`SAFE_DELETE`/`SAFE_DELETE_ARRAY`/`SAFE_RELEASE` macros: `= NULL` → `= nullptr` |
| `preferences.h` | 64, 69 | Default params `= NULL` → `= nullptr` |
| `sorting_hash_table.h` | 158, 203 | `NULL` → `nullptr` |
| `system_info.h` | 14, 31 | `NULL` → `nullptr` |

**Build impact:** HIGH — `NeuronClient.h` is in the pch chain.  Full
rebuild of NeuronClient, GameLogic, Starstrike.
**Commit this sub-step separately** to isolate the rebuild.

#### Step 0.1c — Starstrike (~30 sites across 10 files)

| File | Line(s) | Notes |
|------|---------|-------|
| `particle_system.h` | 91 | `RGBAColour col = NULL` → `RGBAColour col = RGBAColour(0)` (see note below) |
| `particle_system.cpp` | 319 | `col != NULL` → `col != RGBAColour(0)` |
| `entity_grid.cpp` | 70-71, 212 | `= NULL` → `= nullptr` |
| `landscape.cpp` | 39, 41, 77, 99, 103, 525-528, 602-604 | `= NULL` → `= nullptr` |
| `GameApp.cpp` | 248, 262, 288, 325 | `= NULL` / `== NULL` → `nullptr` |
| `gamecursor.cpp` | 67 | `= NULL` → `= nullptr` |
| `routing_system.cpp` | 120 | `return NULL` → `return nullptr` |
| `script.cpp` | 711, 713, 722, 810, 852 | `NULL` → `nullptr` |
| `unit.cpp` | 260, 365, 535 | `NULL` → `nullptr` |
| `location.h` | 108-110 | `= NULL` → `= nullptr` |

> ⚠️ **`RGBAColour` note:** The default constructor `RGBAColour()` leaves
> `r,g,b,a` **uninitialized** (empty body at `rgb_colour.cpp:10-12`).
> `RGBAColour{}` ≠ `RGBAColour(0)`.  The original `= NULL` invoked
> `RGBAColour(int)` which sets all bytes to zero.  Use `RGBAColour(0)`
> to preserve identical semantics.  Do **not** use `{}` — it produces
> garbage and `col != RGBAColour{}` compares against uninitialized memory.

**Build:** Incremental Starstrike.

#### Step 0.1d — GameLogic

GameLogic files that use `NULL` are within the legacy convention scope.
Only change sites encountered during edits in later phases.  **Skip for
now.**

**Verification:** Build both configs.  Run app.

---

### Phase 0 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ No behavioral change — purely textual
```

---

## Phase 4.2 — Dead Preprocessor Branches (early)

**Effort:** 2 days &nbsp;|&nbsp; **Risk:** LOW — removing code that is never compiled
**CI ref:** §4.2
**Commit message pattern:** `cleanup(dead-code): remove <DEFINE> branches [CI §4.2]`

> **Execute before Phase 1.** Removing dead platform branches eliminates
> ~30 `sprintf`/`strcpy` sites from the Phase 1 scope.

**Pre-flight:** Verify no `.vcxproj` defines any of these macros:
```powershell
Get-ChildItem -Recurse *.vcxproj | Select-String 'DEMO2|JAMES_FIX|TARGET_OS_LINUX|TARGET_OS_MACOSX|TARGET_OS_VISTA|TARGET_VISTA_DEMO2|TARGET_FULLGAME|TARGET_DEMOGAME|TARGET_DEMO2' | ForEach-Object { "$($_.Filename):$($_.LineNumber): $($_.Line.Trim())" }
```
Expected: zero results (only `WIN32` and `NDEBUG`/`_DEBUG` are defined).

**Verification method:** For each file, diff the preprocessed output
(`cl /P /EP`) before and after to confirm no active code was removed.

### Step 4.2a — Remove `TARGET_OS_LINUX` / `TARGET_OS_MACOSX` branches

**Files (3):**

| File | Lines | Action |
|------|-------|--------|
| `Starstrike/GameApp.cpp` | 182 (`#if defined(TARGET_OS_LINUX) && …`), 239-260 (`#if defined(TARGET_OS_LINUX) \|\| …`) | Collapse: keep the `#else` (Windows) path. Delete the `#if`/`#elif`/`#endif` lines and the dead Linux/macOS blocks. |
| `NeuronClient/preferences.cpp` | 174, 186, 198, 211, 283, 290 | Each `#ifdef TARGET_OS_MACOSX` block: keep the `#else` path, remove the `#ifdef`/`#endif` wrapper and the macOS code. |
| `GameLogic/prefs_keybindings_window.cpp` | 232, 296 | Same pattern — keep `#else` path. |

**Build:** Incremental Starstrike + NeuronClient + GameLogic.

### Step 4.2b — Remove `TARGET_OS_VISTA` / `TARGET_VISTA_DEMO2` branches

**Files (4):**

| File | Lines | Action |
|------|-------|--------|
| `Starstrike/GameApp.cpp` | 282-300 (`#ifdef TARGET_OS_VISTA`), 292 (references `TARGET_VISTA_DEMO2`), 324 (`#ifdef TARGET_OS_VISTA`) | Delete entire `#ifdef … #endif` blocks (these contain the overlapping `sprintf` UB from §1.6 — deleting the block also fixes §1.6). |
| `Starstrike/main.h` | 11+ (`#ifdef TARGET_OS_VISTA` extern block) | Delete the `#ifdef … #endif` and the dead extern declarations inside. |
| `GameLogic/prefs_other_window.cpp` | 71 (`#ifdef TARGET_OS_VISTA`) | Delete block. |
| `NeuronClient/NeuronClient.h` | 95 (commented-out `#define TARGET_VISTA_DEMO2`) | Delete line. |

**Build:** Full rebuild (touches `NeuronClient.h`).

### Step 4.2c — Remove `DEMO2` branches

**Files (2):**

| File | Lines | Action |
|------|-------|--------|
| `GameLogic/ai.cpp` | 110 (`#ifdef DEMO2`) | Delete `#ifdef … #endif` block, keep the `#else` path. |
| `GameLogic/mainmenus.cpp` | 332 (`#ifdef DEMO2`) | Same. |

Also: `NeuronClient/NeuronClient.h` line 90 (`//#define TARGET_DEMOGAME`) and
line 92 (`//#define TARGET_DEMO2`) — delete commented-out lines.

Check `GameApp.cpp:182` references `TARGET_DEMOGAME` — already removed in
step 4.2a.

**Build:** Incremental GameLogic.

### Step 4.2d — Remove `JAMES_FIX` branches

**Files (2):**

| File | Lines | Action |
|------|-------|--------|
| `Starstrike/global_world.cpp` | 142-145 | Evaluate: if the code inside `#ifdef JAMES_FIX` is a correct fix, unconditionally keep it; if it's a workaround, remove it. Document the decision in the commit message. |
| `GameLogic/gunturret.cpp` | 254-258 | Same evaluation. |

**Build:** Incremental Starstrike + GameLogic.

### Step 4.2e — Clean up `NeuronClient.h` target selection block

**File:** `NeuronClient/NeuronClient.h` (lines 87-120)

**Current state (after steps 4.2a-d):**
```cpp
// === PICK ONE OF THESE TARGETS ===

#define TARGET_DEBUG

// === PICK ONE OF THESE TARGETS ===

#define DEBUG_RENDER_ENABLED
// ...
#ifdef TARGET_DEBUG
#define DARWINIA_GAMETYPE "debug"
#define CHEATMENU_ENABLED
#endif

#if !defined(TARGET_DEBUG) && \
    !defined(TARGET_FULLGAME) && \
    ... (all dead) ...
#error "Unknown target, cannot determine game type"
#endif
```

**Change to:**
```cpp
#define TARGET_DEBUG
#define DARWINIA_GAMETYPE "debug"

#ifdef _DEBUG
#define CHEATMENU_ENABLED
#endif

#define DEBUG_RENDER_ENABLED
```

> ⚠️ **`CHEATMENU_ENABLED` must stay debug-gated.**  It controls cheat
> menus (`cheat_window.cpp`, `debugmenu.cpp`), rocket cheats
> (`rocket.cpp`), and debug input (`user_input.cpp`).  The original code
> gated it on `#ifdef TARGET_DEBUG` which was always defined — so it was
> unconditionally active in both Debug and Release.  Now that we're
> removing the target selection system, gate on `_DEBUG` instead to
> prevent cheat menus from shipping in Release builds.

Also in this same commit, remove the duplicate `PROFILER_ENABLED`
definition (CI §5.2) to avoid a second full rebuild later:

```cpp
// DELETE these lines (were at lines 103-105):
#ifndef _OPENMP
#define PROFILER_ENABLED
#endif
```

The authoritative definition in `NeuronCore/NeuronCore.h` (line 8, under
`_DEBUG`) is sufficient.

Remove the `#if !defined(…) #error` guard — there is only one target now.
Remove comment banners and all dead `#define` / `#ifdef` lines.

**Build impact:** HIGH — full rebuild (pch header change).
**Verification:** Build both configs.  Preprocessor diff shows identical
active code.  Verify `CHEATMENU_ENABLED` is **not** defined in
Release|x64 (`cl /EP` check).

---

### Phase 4.2 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ Preprocessor diff confirms no active code removed
✅ §1.6 (sprintf overlapping UB) is now resolved — dead code deleted
✅ Runtime smoke test passes
```

---

## Phase 7-Early — Build Config Fixes

**Effort:** 2 hours &nbsp;|&nbsp; **Risk:** LOW (project file edits only)
**Commit message pattern:** `build: <description> [CI §7.N]`

### Step 7.5 — Add `AdditionalIncludeDirectories` to Release configs

**CI ref:** §7.5

**Pre-flight:** Attempt a clean Release|x64 build to confirm current state.

**Files (4 `.vcxproj`):**

For each project, duplicate the Debug `AdditionalIncludeDirectories` entry
into the Release `<ItemDefinitionGroup>`.  Alternatively, move it to a
non-conditional `<ItemDefinitionGroup>` shared by both configs.

| Project File | Debug Value | Action |
|-------------|-------------|--------|
| `NeuronCore/NeuronCore.vcxproj` | `$(SolutionDir)GameLogic;$(SolutionDir)NeuronClient;$(SolutionDir)StarStrike` | Add same to Release `ClCompile` block |
| `NeuronClient/NeuronClient.vcxproj` | `$(SolutionDir)NeuronCore;$(SolutionDir)GameLogic;$(SolutionDir)StarStrike` | Add same to Release `ClCompile` block |
| `GameLogic/GameLogic.vcxproj` | `$(SolutionDir)NeuronCore;$(SolutionDir)NeuronClient;$(SolutionDir)StarStrike` | Add same to Release `ClCompile` block |
| `Starstrike/Starstrike.vcxproj` | `$(SolutionDir)NeuronCore;$(SolutionDir)NeuronClient;$(SolutionDir)GameLogic` | Add same to Release `ClCompile` block |

**Verification:** Clean build Release|x64 succeeds.

---

### Step 7.7 — Add shader compilation settings for Release

**CI ref:** §7.7

**File:** `NeuronClient/NeuronClient.vcxproj` (lines 325-342)

Currently the `<FxCompile>` entries only have `Condition="Debug|x64"`
attributes.  Add matching Release entries:

For `PixelShader.hlsl`:
```xml
<VariableName Condition="'$(Configuration)|$(Platform)'=='Release|x64'">g_p%(Filename)</VariableName>
<HeaderFileOutput Condition="'$(Configuration)|$(Platform)'=='Release|x64'">$(ProjectDir)CompiledShaders\%(Filename).h</HeaderFileOutput>
<ShaderType Condition="'$(Configuration)|$(Platform)'=='Release|x64'">Pixel</ShaderType>
<ShaderModel Condition="'$(Configuration)|$(Platform)'=='Release|x64'">6.7</ShaderModel>
```

Repeat for `VertexShader.hlsl` (change `ShaderType` to `Vertex`).

**Note:** Omit `/Qembed_debug` from Release — debug info is not wanted in
Release shader bytecode.

**Verification:** Clean build Release|x64 — shaders recompile.

---

### Step 7.1 — Convert remaining 2 header guards

**CI ref:** §7.1

| File | Current | Change |
|------|---------|--------|
| `NeuronClient/auto_vector.h` | `#if !defined AUTO_VECTOR_H` / `#define AUTO_VECTOR_H` / `#endif` at EOF | Replace with `#pragma once` at top; remove `#define AUTO_VECTOR_H` and closing `#endif` |
| `NeuronClient/opengl_directx.h` | `#if !defined __OPENGL_DIRECTX` / `#define __OPENGL_DIRECTX` / `#endif` at EOF | Replace with `#pragma once` at top; remove `#define __OPENGL_DIRECTX` and closing `#endif` |

**Build:** Incremental NeuronClient.

---

### Phase 7-Early Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors (was this failing before? Note in commit.)
✅ Runtime smoke test passes
```

---

## Phase 1 — Security & Stability

**Effort:** 2-3 weeks &nbsp;|&nbsp; **Depends on:** Phase 4.2 (reduces scope by ~30 sites)
**Commit message pattern:** `security: <description> [CI §1.N]`

### Step 1.2 — Fix `MAX_PACKET_SIZE` collision

**CI ref:** §1.2 &nbsp;|&nbsp; **Category:** CLEANUP

**Files (2):**

| File | Change |
|------|--------|
| `NeuronCore/DataReader.h` | `#define MAX_PACKET_SIZE 1500` → `#define DATAREADER_BUFFER_SIZE 1500`; update `DATALOAD_SIZE` to use `DATAREADER_BUFFER_SIZE` |
| `NeuronCore/DataWriter.h` | `#define MAX_PACKET_SIZE 1500` → `#define DATAWRITER_BUFFER_SIZE 1500`; update `DATALOAD_SIZE` to use `DATAWRITER_BUFFER_SIZE` |

**Why first:** These headers are not included anywhere today, so this is
a zero-risk rename.  Doing it first prevents accidental collision when
someone adds `#include "DataWriter.h"` later.

**Build:** Incremental NeuronCore.

---

### Step 1.1 — Eliminate unsafe C string functions

**CI ref:** §1.1 &nbsp;|&nbsp; **Category:** SECURITY

Process one project at a time.  Within each project, process one file at a
time.  Each file is a separate commit.

**General rules:**
- `sprintf(buf, fmt, …)` → `snprintf(buf, sizeof(buf), fmt, …)`
- `strcpy(dst, src)` → `strncpy(dst, src, sizeof(dst)); dst[sizeof(dst)-1] = '\0';`
- `strcat(dst, src)` → `strncat(dst, src, sizeof(dst) - strlen(dst) - 1);`
- In Starstrike (modern layer), prefer `std::format` + `std::string` where
  the function already uses C++ features.
- Do **not** change function signatures in GameLogic/NeuronClient unless
  strictly necessary — legacy convention.

> ⚠️ **`sizeof(dst)` only works for stack-allocated arrays** where the
> compiler knows the full type (e.g. `char buf[512]`).  When `dst` is a
> `char*` parameter or member pointer, `sizeof(dst)` returns **8** (pointer
> size on x64), causing a silent buffer overflow.
>
> For pointer targets, use the known constant (e.g. `SIZE_ECLWINDOW_NAME`,
> `SIZE_ECLBUTTON_NAME`) or pass the buffer size as a parameter.  When in
> doubt, `grep` for the declaration and verify the type before applying the
> `sizeof` pattern.
>
> **Examples that are safe** (stack arrays):
> - `char langFilename[512];` → `snprintf(langFilename, sizeof(langFilename), …)` ✅
> - `char name[64];` → `snprintf(name, sizeof(name), …)` ✅
>
> **Examples that need the constant** (named-size buffers):
> - `static char currentButton[SIZE_ECLBUTTON_NAME];` → `strncpy(currentButton, …, SIZE_ECLBUTTON_NAME)` ✅
> - `char m_name[SIZE_ECLBUTTON_NAME];` → use `SIZE_ECLBUTTON_NAME`, not `sizeof(m_name)` (both happen to work here, but the constant documents intent)

#### Step 1.1b — GameLogic (90 sites)

Priority order (highest risk first):

| # | File | Sites | Key concern |
|---|------|-------|-------------|
| 1 | `input_field.cpp` | 6 | `m_buf[256]` overflow from user input |
| 2 | `darwinia_window.cpp` | 4 | `sprintf` into `char[64]` |
| 3 | `constructionyard.cpp` | 3 | `sprintf` into `char[64]` |
| 4 | `drop_down_menu.cpp` | 2 | `sprintf` / `strcpy` |
| 5 | `generichub.cpp` | 2 | `sprintf` |
| 6 | Remaining ~73 sites across 20+ files | — | Batch by file |

**Build:** After each file, incremental GameLogic.  After all files, full
build both configs.

#### Step 1.1c — NeuronClient (94 sites)

Priority order:

| # | File | Sites | Key concern |
|---|------|-------|-------------|
| 1 | `eclipse.cpp` | 50+ | Pervasive `strcpy` into fixed buffers |
| 2 | `clienttoserver.cpp` | 6 | Network input → `strcpy` |
| 3 | Remaining ~38 sites across 15+ files | — | Batch by file |

**Build:** After each file, incremental NeuronClient.

#### Step 1.1d — Starstrike (114 sites)

Priority order:

| # | File | Sites | Key concern |
|---|------|-------|-------------|
| 1 | `GameApp.cpp` | 20+ | Many `sprintf` into various `char[]` |
| 2 | `global_world.cpp` | 10+ | `sprintf` |
| 3 | `gamecursor.cpp` | 4 | `sprintf` into `char[512]` |
| 4 | `entity_grid.cpp` | 5 | `strcat` |
| 5 | Remaining ~75 sites | — | Batch by file |

**Build:** After each file, incremental Starstrike.

**Verification after all 1.1 sub-steps:**
- Build both configs.
- Run under AddressSanitizer (`/fsanitize=address`) in Debug: load a level,
  exercise menus, cursor, weapon systems.
- No buffer-overflow reports.

---

### Step 1.9 — Fix `const_cast` in `file_paths.cpp`

**CI ref:** §1.9 &nbsp;|&nbsp; **Category:** BUG

> ⚠️ **Compile-breaking change:** Changing `_default` to `const char*`
> while the return type stays `char*` will fail to compile — `return
> _default;` cannot implicitly convert `const char*` to `char*`.  The
> return type must also change to `const char*`, which cascades to 6 call
> sites.  All 6 callers were verified to only **read** through the
> returned pointer — none mutate it.

**Files (9):**

| # | File | Change |
|---|------|--------|
| 1 | `NeuronClient/preferences.h` line 69 | `char *GetString(char const *_key, char *_default=nullptr) const;` → `const char *GetString(char const *_key, const char *_default=nullptr) const;` |
| 2 | `NeuronClient/preferences.cpp` line 458 | `char* PrefsManager::GetString(const char* _key, char* _default) const` → `const char* PrefsManager::GetString(const char* _key, const char* _default) const` |
| 3 | `NeuronClient/file_paths.cpp` line 60 | Remove `const_cast<char*>(…)` wrapper |
| 4 | `GameLogic/prefs_other_window.cpp` line 104 | `char* bootloader = g_prefsManager->GetString(…)` → `const char* bootloader = …` |
| 5 | `NeuronClient/soundsystem.cpp` line 268 | `char* libName = g_prefsManager->GetString(…)` → `const char* libName = …` |
| 6 | `NeuronClient/clienttoserver.cpp` line 65 | `char* serverAddress = g_prefsManager->GetString(…)` → `const char* serverAddress = …` |
| 7 | `NeuronClient/prefs_sound_window.cpp` line 143 | `char *soundLib = g_prefsManager->GetString(…)` → `const char *soundLib = …` |
| 8 | `Starstrike/GameApp.cpp` line 99 | `char* language = g_prefsManager->GetString(…)` → `const char* language = …` |
| 9 | `Starstrike/GameApp.cpp` line 110 | `language = g_prefsManager->GetString(…)` — already `const char*` from line 99 after fix |

**Why:** `GetString` never mutates `_default` — it just returns it.
Callers that pass string literals (e.g. `"random"`, `"dsound"`, `"none"`)
are already passing `const char*` which is implicitly convertible to
`char*` in MSVC non-conformant mode — fixing this now avoids a break when
conformance mode is enabled later (§7.3).

**Build impact:** MEDIUM — `preferences.h` is widely included (42 files).
Incremental rebuild of NeuronClient + GameLogic + Starstrike.

**Verification:** Build both configs.  Search for any remaining
`const_cast` in the codebase to confirm this was the only site.

---

### Step 1.5 — Document `void*` dispatching (defer rewrite)

**CI ref:** §1.5 &nbsp;|&nbsp; **Category:** MODERNIZE (deferred)

**File:** `GameLogic/darwinia_window.cpp` (lines 300-309)

**Action:** Add a comment documenting the type-safety risk and cross-
referencing `CI.md §1.5`.  Do **not** rewrite — this is stable legacy code
and per the Exclusions section, modernization of GameLogic should only
happen during a deliberate module rewrite.

```cpp
// WARNING: Type-unsafe void* dispatch — see CI.md §1.5.
// dataType determines the cast target; mismatched dataType is UB.
```

**Build:** No code change, incremental GameLogic.

---

### Step 1.7 — Document `_CRT_SECURE_NO_WARNINGS` for later removal

**CI ref:** §1.7 &nbsp;|&nbsp; **Category:** CLEANUP (deferred)

**File:** `NeuronCore/NeuronCore.h` (line 4)

**Action:** Add a comment — do **not** remove yet (depends on §1.1
completion):

```cpp
#define _CRT_SECURE_NO_WARNINGS  // TODO [CI §1.7]: Remove after §1.1 (unsafe string elimination) is complete
```

---

### Phase 1 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ AddressSanitizer run — no buffer-overflow reports
✅ §1.6 already resolved in Phase 4.2
✅ Runtime smoke test passes
```

---

## Phase 2 — Memory Safety

**Effort:** 3-4 weeks &nbsp;|&nbsp; **Risk:** MEDIUM
**Commit message pattern:** `fix(memory): <description> [CI §2.N]`

### Step 2.3 — Fix leaked `strdup` allocations

**CI ref:** §2.3 &nbsp;|&nbsp; **Category:** BUG

Fix the 5 confirmed leak sites.  Process one file at a time.

| # | File | Line | Fix |
|---|------|------|-----|
| 1 | `Starstrike/startsequence.cpp` | 42 | Add `free(caption->m_caption)` before `strdup` reassignment, or convert `m_caption` to `std::string` and the struct to use `std::string`. |
| 2 | `NeuronClient/text_renderer.cpp` | 39 | Add `free(m_filename)` in the `TextRenderer` destructor. |
| 3 | `GameLogic/entity.cpp` | 63 | `m_names[]` is a static array — add a cleanup function called at shutdown, or accept as intentional process-lifetime allocation (document with comment). |
| 4 | `NeuronClient/user_info.cpp` | 23, 29 | `s_username` / `s_email` are static globals — same as above: accept as process-lifetime or add `atexit` cleanup. |
| 5 | `GameLogic/prefs_other_window.cpp` | 145 | Add a destructor or cleanup method that iterates `m_languages` and calls `free()` on each element before the `LList` is destroyed. |

**Build:** Incremental per-project after each file.
**Verification:** Run under `_CRTDBG_MAP_ALLOC` in Debug — verify leaked
`strdup` allocations no longer appear in the leak report.

---

### Step 2.4 — Fix `memset(this, …)` on non-trivial types

**CI ref:** §2.4 &nbsp;|&nbsp; **Category:** BUG

| # | File | Line | Fix |
|---|------|------|-----|
| 1 | `Starstrike/team.cpp` | 598 | Replace `memset(this, 0, sizeof(*this))` with explicit zeroing: `m_mousePos = {}; m_unitMove = 0; m_primaryFireTarget = 0; …` or `*this = TeamControls{};` |
| 2 | `NeuronClient/matrix33.cpp` | 388 | Add `static_assert(std::is_trivially_copyable_v<Matrix33>, "memset on non-trivial type");` before the `memset` call. If the assert fires, replace with `*this = Matrix33{};`. |
| 3 | `NeuronClient/matrix34.cpp` | 323 | Same: add `static_assert`, leave `memset` if it passes. |

**Build:** Incremental per-project.

---

### Step 2.1 — Convert Starstrike raw `new`/`delete` to RAII (selective)

**CI ref:** §2.1 &nbsp;|&nbsp; **Category:** MODERNIZE

Prioritize files that have the highest leak risk or the simplest
conversion.  Process one file at a time.

| # | File | Pattern | Conversion |
|---|------|---------|------------|
| 1 | `3d_sierpinski_gasket.cpp` | `m_points = new Vector3[n]` / `delete[] m_points` | `std::vector<Vector3> m_points` |
| 2 | `entity_grid.cpp:240-245` | Manual realloc (`new`/`memcpy`/`delete`) | `std::vector<WorldObjectId>` with `.push_back()`.  Add `.reserve(expectedEntityCount)` in the constructor.  Profile before/after. |
| 3 | `water.cpp` | 7× `new`/`delete` for vertex/normal arrays | `std::vector<T>` members |
| 4 | `global_internet.cpp` | 2× `new T[]` | `std::vector<T>` |

**Build:** Incremental Starstrike after each file.
**Verification:** Run under Debug CRT heap checking — no leak reports for
converted allocations.

> **Note:** `global_world.cpp` and `GameApp.cpp` have complex ownership
> graphs.  Defer those to a later pass or a dedicated rewrite session.

---

### Step 2.2 — Document move-semantics gap (defer rewrite)

**CI ref:** §2.2 &nbsp;|&nbsp; **Category:** MODERNIZE (deferred)

**Action:** Add `// TODO [CI §2.2]: Rule-of-Five` comments to the class
declarations of `BitmapRGBA`, `EclButton`, `EclWindow`.  Do not rewrite —
per Exclusions, these are deep legacy classes.

---

### Phase 2 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ Debug CRT heap check — no new leaks
✅ static_assert on matrix types — no fire
✅ Runtime smoke test passes
```

---

## Phase 7-Remaining — Build Hygiene

**Effort:** 1 week &nbsp;|&nbsp; **Risk:** LOW-MEDIUM
**Commit message pattern:** `build: <description> [CI §7.N]`

### Step 7.6 — Remove dead Win32 configs from GameLogic

**CI ref:** §7.6

**File:** `GameLogic/GameLogic.vcxproj`

Remove all `Condition="…Win32…"` blocks:
- `<ProjectConfiguration Include="Debug|Win32">` and `<ProjectConfiguration Include="Release|Win32">`
- `<PropertyGroup Condition="…Win32…" Label="Configuration">`
- `<ImportGroup Label="PropertySheets" Condition="…Win32…">`
- `<ItemDefinitionGroup Condition="…Win32…">`
- Any per-file `Condition="…Win32…"` (e.g. `pch.cpp` lines 264-265)

**Verification:** Solution loads cleanly.  Build both x64 configs.

---

### Step 7.3 — Enable Conformance Mode for all configs

**CI ref:** §7.3

**Files (3 `.vcxproj`):**

| Project | Debug\|x64 | Release\|x64 |
|---------|-----------|-------------|
| `NeuronClient/NeuronClient.vcxproj` | `false` → `true` | `true` ✅ already |
| `GameLogic/GameLogic.vcxproj` | `false` → `true` | `true` ✅ already |
| `Starstrike/Starstrike.vcxproj` | `false` → `true` | Add `<ConformanceMode>true</ConformanceMode>` |

**Build:** Full rebuild **both configs**.  Fix any resulting errors — these
indicate non-conformant code.  Document each fix in the commit.

**Expected errors:** Possible issues with implicit `const char*` → `char*`
conversions (the `preferences.h` fix in §1.9 should have resolved the
main one).

---

### Step 7.2 — Raise warning level to /W4

**CI ref:** §7.2

**Files (4 `.vcxproj`):**

| Project | Current | Change |
|---------|---------|--------|
| `NeuronCore/NeuronCore.vcxproj` | `/W3` | `/W4` |
| `NeuronClient/NeuronClient.vcxproj` | `/W3` | `/W4` |
| `GameLogic/GameLogic.vcxproj` | `/W3` | `/W4` |
| `NeuronServer/NeuronServer.vcxproj` | `/W3` | `/W4` |

**Build:** Full rebuild both configs.  Address new warnings — expect:
- C4100 (unreferenced parameter) — add `(void)param;` or `[[maybe_unused]]`
- C4189 (unreferenced local variable) — delete or use
- C4505 (unreferenced local function) — delete

This may produce a large number of warnings.  Process one project at a time,
starting with NeuronCore (smallest).

---

### Step 7.4 — Audit `#pragma warning(disable:…)`

**CI ref:** §7.4

**File:** `NeuronCore/NeuronCore.h` (lines 11-14)

Process one warning at a time:

1. Comment out `#pragma warning(disable:4201)`.  Build.  If errors appear,
   add `#pragma warning(suppress:4201)` locally where the warning fires.
   If clean, delete the global suppression.
2. Repeat for 4238, 4244, 4324.

**Build:** Full rebuild after each warning is re-enabled.

---

### Step 7.8 — Evaluate NeuronServer

**CI ref:** §7.8

**Decision required:** Is `NeuronServer` a placeholder for future work?

- If **yes**: add a comment to `NeuronServer.h` documenting its purpose.
- If **no**: remove the project from the solution and delete the directory.

---

### Phase 7 Gate

```
✅ Build Debug|x64   — zero errors, zero warnings (at /W4)
✅ Build Release|x64 — zero errors, zero warnings (at /W4)
✅ Conformance mode true for all projects, all configs
✅ Runtime smoke test passes
```

---

## Phase 4-Remaining — Dead Code Cleanup

**Effort:** 3 days &nbsp;|&nbsp; **Risk:** LOW
**Commit message pattern:** `cleanup: <description> [CI §4.N]`

### Step 4.1 — Remove commented-out code blocks

**CI ref:** §4.1

Process one project at a time.  Each project is one commit.

**Order:**
1. **GameLogic** (15+ blocks across `ai.cpp`, `airstrike.cpp`, `anthill.cpp`,
   `armyant.cpp`, `armour.cpp`, `blueprintstore.cpp`, `building.cpp`,
   `darwinian.cpp`, `entity.cpp`, `entity_leg.cpp`, `generichub.cpp`,
   `officer.cpp`, `spirit.cpp`, `tripod.cpp`, `virii.cpp`, `weapons.cpp`,
   `drop_down_menu.cpp`)
2. **NeuronClient** (`clienttoserver.cpp`, `control_bindings.cpp`,
   `hash_table.h`, `sound_filter.cpp`, `preferences.cpp`)
3. **Starstrike** (`camera.cpp`, `user_input.cpp`)

**Build:** Incremental per-project.

---

### Step 4.3 — Rename `fixMeUp` variables

**CI ref:** §4.3

**File:** `Starstrike/camera.cpp`

| Line | Current | Renamed |
|------|---------|---------|
| 291 | `static float fixMeUp = 45000.0f;` | `static float s_cameraBootFov = 45000.0f;` |
| 327 | `static bool fixMeUp = true;` | `static bool s_cameraXNeedsInit = true;` |
| 382 | `static bool fixMeUp = true;` | `static bool s_cameraYNeedsInit = true;` |

Update all references within each function scope.

**Build:** Incremental Starstrike.

---

### Step 4.4 — Remove `JAK HACK`

**CI ref:** §4.4

**File:** `Starstrike/water.cpp` (line 452)

Delete the disabled hack comment and any associated dead code block.

---

### Step 4.5 — Remove `DELETEME` tags

**CI ref:** §4.5

| File | Action |
|------|--------|
| `Starstrike/3d_sierpinski_gasket.cpp` lines 7-8 | Verify `GameApp.h` and `camera.h` symbols are unused in this file. If unused, delete both `#include` lines. |
| `Starstrike/main.cpp` line 374 | Delete the `// DELETEME` line and associated debug code. |

**Build:** Incremental Starstrike.

---

### ~~Step 5.2 — Remove duplicate `PROFILER_ENABLED`~~ ✅ MOVED

Batched into Step 4.2e (same `NeuronClient.h` commit) to avoid a
redundant full rebuild.  See Step 4.2e for details.

---

### Phase 4 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ No behavioral change — purely dead code removal
✅ Runtime smoke test passes
```

---

## Phase 3 — Type Safety & Modern Idioms

**Effort:** 4-6 weeks &nbsp;|&nbsp; **Depends on:** Phase 1 (for NeuronCore net layer)
**Commit message pattern:** `modernize: <description> [CI §3.N]`

### Step 3.1a — Convert `NetRetCode` to `enum class`

**CI ref:** §3.1

**File:** `NeuronCore/net_lib.h`

1. Change `enum NetRetCode { … }` → `enum class NetRetCode : int { … }`.
2. Find all references: `NetOk`, `NetFailed`, `NetBadArgs`, etc.
3. Update to `NetRetCode::NetOk`, etc.  (Scope limited to `net_*.cpp` files
   in NeuronCore — no cross-project impact.)

**Build:** Incremental NeuronCore.

---

### Step 3.1b-g — Convert NeuronClient enums (one per step)

Process each enum in a separate commit.  Order by blast radius (smallest
first):

| Step | Enum | File | Scope |
|------|------|------|-------|
| 3.1b | `InputParserState` | `inputdriver.h` | NeuronClient only |
| 3.1c | `InputCondition` | `inputdriver.h` | NeuronClient only |
| 3.1d | Network update types | `networkupdate.h` | NeuronClient only |
| 3.1e | `InputType` | `input_types.h` | NeuronClient + Starstrike |
| 3.1f | `InputMode` | `input_types.h` | NeuronClient + Starstrike |
| 3.1g | `ControlType` | `control_types.h` | NeuronClient + Starstrike |

For each:
1. Add `enum class` with explicit underlying type (`: int`).
2. Find all references (switch cases, comparisons, assignments).
3. Qualify all enum values.
4. Build, fix errors, repeat.

**Build:** Full build after each step (headers are in pch chain).

> **Defer:** `entity.h` and `building.h` anonymous enums — too many
> comparison sites, too high risk without test coverage.

---

### Step 3.2 — Add `[[nodiscard]]`, `noexcept`, `constexpr` to NeuronCore

**CI ref:** §3.2

Process one header at a time:

| File | Change |
|------|--------|
| `TimerCore.h` | Add `noexcept` to all 6 static getters (lines 23-32) |
| `Hash.h` | Add `[[nodiscard]]` to `HashRange()`, `HashState()` |
| `MathCommon.h` | Add `constexpr` to math constants |

**Build:** Full rebuild (headers in pch chain).

---

### Step 3.3 — `std::string_view` in NeuronCore net APIs

**CI ref:** §3.3

**File:** `NeuronCore/net_socket.h` — change `Connect(char*, …)` to
`Connect(std::string_view, …)`.  Update implementation in `net_socket.cpp`.

**Build:** Full rebuild (header change).

---

### Step 3.4 — Modernise NeuronCore networking (4 sub-steps)

**CI ref:** §3.4

One file per commit, build after each:

| Step | File(s) | Changes |
|------|---------|---------|
| 3.4a | `net_mutex.h`, `net_mutex.cpp` | `CRITICAL_SECTION` → `std::mutex`; `NetMutexLock` → `std::lock_guard` |
| 3.4b | `net_thread_win32.cpp` | `CreateThread()` → `std::jthread`; remove `NULL` args |
| 3.4c | `net_socket.h`, `net_socket.cpp` | `memset` → in-class init; C-casts → `static_cast`; `(FILE*)0` → `nullptr`; magic numbers → constants |
| 3.4d | `net_socket_listener.h`, `net_socket_listener.cpp` | `(NetCallBack)NULL` → `nullptr`; function pointers → `std::function` |

Also in `net_lib.h`: `MIN()` macro → `std::min()`.

---

### Step 3.5 — Template `DataWriter` write methods

**CI ref:** §3.5

**File:** `NeuronCore/DataWriter.h`

Replace 7 identical methods with:
```cpp
template <typename T>
  requires std::is_arithmetic_v<T>
void Write(const T& value)
{
  ASSERT_TEXT(m_offset + sizeof(T) <= DATAWRITER_BUFFER_SIZE,
              "DataWriter overflow writing {} bytes", sizeof(T));
  memcpy(m_data + m_offset, &value, sizeof(T));
  m_offset += sizeof(T);
}
```

Keep `WriteChar`, `WriteByte`, `WriteString`, `WriteRaw` as-is.

**Build:** Incremental NeuronCore.

---

### Phase 3 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ All enum class conversions compile without implicit int fallback
✅ Runtime smoke test — networking functions work correctly
```

---

## Phase 5 — Code Duplication

**Effort:** 2-3 weeks &nbsp;|&nbsp; **Depends on:** Phase 3.1 for §5.4
**Commit message pattern:** `refactor: <description> [CI §5.N]`

### Step 5.3 — Extract OpenGL quad-rendering helpers

**CI ref:** §5.3

1. Create `NeuronClient/RenderHelpers.h` / `RenderHelpers.cpp`.
2. Extract `RenderQuad()` / `RenderTexturedQuad()` from the most common
   `glBegin(GL_QUADS)` patterns.
3. Replace call sites one file at a time, building after each.

**Build:** Add new files to `NeuronClient.vcxproj`.  Incremental builds.

---

### Step 5.4 — Refactor `Building::CreateBuilding` (deferred)

**CI ref:** §5.4

**Precondition:** §3.1 enum class for `building.h` — currently deferred.
This step is **blocked**.  Document in `building.cpp` with a TODO comment.

---

### Step 5.5 — Consolidate entity boilerplate (deferred)

**CI ref:** §5.5

This is a large structural refactor across 30+ legacy files.  Per the
Exclusions section, defer until a deliberate GameLogic rewrite.  Document
with a TODO in `entity.h`.

---

### Phase 5 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ No behavioral change — refactored code produces identical rendering
✅ Runtime smoke test passes
```

---

## Phase 6 — Performance

**Effort:** 1 week &nbsp;|&nbsp; **Risk:** MEDIUM (behavioral changes)
**Commit message pattern:** `perf: <description> [CI §6.N]`

### ~~Step 6.3 — Convert entity grid manual realloc to `std::vector`~~ ✅ MERGED

This work is identical to Step 2.1 row #2 (`entity_grid.cpp:240-245`).
Removed as a duplicate — the `.reserve()` guidance has been folded into
Step 2.1.

---

### Step 6.2 — Implement dirty-region tracking for location recalc

**CI ref:** §6.2

**File:** `Starstrike/location.cpp` (lines 602, 637)

This is a design-level change requiring understanding of the spatial grid
system.  Steps:

1. Add a dirty-region bitfield or set to the location manager.
2. When cells are modified, mark them dirty.
3. In the recalculation pass, only process dirty cells.
4. Clear dirty flags after recalculation.

**Build:** Incremental Starstrike.
**Verification:** Profile before and after — target ≥ 5× speedup for
partial updates.

---

### Phase 6 Gate

```
✅ Build Debug|x64   — zero errors
✅ Build Release|x64 — zero errors
✅ Performance improvement measured and documented
✅ No visual or gameplay regressions
✅ Runtime smoke test passes
```

---

## Step 1.8 — `atoi`/`atof` hardening (long-tail)

**CI ref:** §1.8 &nbsp;|&nbsp; **Category:** SECURITY &nbsp;|&nbsp; **Count:** 250 sites

This is a background task to be done incrementally alongside other work.
It is not gated — no phase depends on it.

**Rules:**
- In legacy projects: `atoi(s)` → `strtol(s, nullptr, 10)` (or `strtof`).
- In Starstrike: `std::from_chars` where feasible.
- Process one file at a time, commit per file.

---

## Step 1.7-Final — Remove `_CRT_SECURE_NO_WARNINGS`

**CI ref:** §1.7 &nbsp;|&nbsp; **Depends on:** §1.1 (all sub-steps complete)

**File:** `NeuronCore/NeuronCore.h` (line 4)

Delete `#define _CRT_SECURE_NO_WARNINGS`.

**Build:** Full rebuild both configs.  Fix any remaining warnings — these
indicate missed `sprintf`/`strcpy` sites from Phase 1.

---

## Progress Tracker

| Phase | Section | Status |
|-------|---------|--------|
| **-1** | §-1.1 Fix `delete`/`delete[]` mismatch | ✅ DONE |
| **-1** | §-1.2 Fix format-string injection | ✅ DONE |
| **-1** | §-1.3 Fix `Fatal()` errorText | ✅ DONE |
| **0** | §0.2 `NULL` → `nullptr` | ✅ DONE |
| **4.2** | §4.2a Remove `TARGET_OS_LINUX`/`MACOSX` | ✅ DONE |
| **4.2** | §4.2b Remove `TARGET_OS_VISTA` | ✅ DONE |
| **4.2** | §4.2c Remove `DEMO2` | ✅ DONE |
| **4.2** | §4.2d Remove `JAMES_FIX` | ✅ DONE |
| **4.2** | §4.2e Clean `NeuronClient.h` target block | ✅ DONE |
| **7** | §7.5 Release include dirs | ✅ DONE |
| **7** | §7.7 Shader Release config | ✅ DONE |
| **7** | §7.1 Last 2 header guards | ✅ DONE |
| **1** | §1.2 `MAX_PACKET_SIZE` rename | ✅ DONE |
| **1** | §1.1b GameLogic unsafe strings (90) | ✅ DONE — 5 deliberate skips (`strcpy(m_string,m_buf)` in `input_field.cpp`, `m_string` is `char*` with unknown external size) |
| **1** | §1.1c NeuronClient unsafe strings (94) | ☐ ~86 sites remaining |
| **1** | §1.1d Starstrike unsafe strings (114) | ☐ ~105 sites remaining |
| **1** | §1.9 `const_cast` fix | ✅ DONE — cascaded to `SetLanguage`/`SetProfileName` const, `clienttoserver.cpp` uses mutable buffer for `NetSocket::Connect` |
| **1** | §1.5 Document `void*` dispatch | ✅ DONE |
| **1** | §1.7 Document `_CRT_SECURE_NO_WARNINGS` | ✅ DONE |
| **2** | §2.3 Fix `strdup` leaks (5 sites) | ☐ |
| **2** | §2.4 Fix `memset(this,…)` | ☐ |
| **2** | §2.1 Starstrike RAII conversion | ☐ |
| **2** | §2.2 Document move-semantics gap | ☐ |
| **7** | §7.6 Remove Win32 configs | ☐ |
| **7** | §7.3 Enable Conformance Mode | ☐ |
| **7** | §7.2 Raise to /W4 | ☐ |
| **7** | §7.4 Audit warning suppressions | ☐ |
| **7** | §7.8 Evaluate NeuronServer | ☐ |
| **4** | §4.1 Remove commented code | ☐ |
| **4** | §4.3 Rename `fixMeUp` | ☐ |
| **4** | §4.4 Remove JAK HACK | ☐ |
| **4** | §4.5 Remove DELETEME | ☐ |
| **4.2e** | §5.2 Remove duplicate `PROFILER_ENABLED` (batched) | ☐ |
| **3** | §3.1a `NetRetCode` enum class | ☐ |
| **3** | §3.1b-g NeuronClient enums | ☐ |
| **3** | §3.2 `[[nodiscard]]`/`noexcept`/`constexpr` | ☐ |
| **3** | §3.3 `std::string_view` net APIs | ☐ |
| **3** | §3.4a-d Modernise net layer | ☐ |
| **3** | §3.5 Template DataWriter | ☐ |
| **5** | §5.3 Extract GL quad helpers | ☐ |
| **5** | §5.4 Building factory (BLOCKED) | ☐ |
| **5** | §5.5 Entity boilerplate (DEFERRED) | ☐ |
| **2** | §6.3 Entity grid `std::vector` (merged into §2.1) | ✅ MERGED |
| **6** | §6.2 Dirty-region recalc | ☐ |
| — | §1.8 `atoi`/`atof` hardening (background) | ☐ |
| — | §1.7-Final Remove `_CRT_SECURE_NO_WARNINGS` | ☐ |

---

*Each checked box should include a commit hash.  Update this tracker as
work progresses.*
