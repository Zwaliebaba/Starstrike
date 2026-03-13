# Starstrike — Codebase Improvement Plan

> Generated from a full static analysis of every project in the solution.
> Findings are grouped by severity, then by project.  Work items within each
> phase are ordered so that earlier items never depend on later ones.
>
> **Last updated:** Full re-analysis pass with structural recommendations applied.
> Items marked ✅ DONE have been verified in the current source tree.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Execution Summary](#execution-summary)
- [Exclusions — Legacy Conventions](#exclusions--legacy-conventions)
- [Verification Protocol](#verification-protocol)
- [Phase -1 — Active UB Hotfixes (< 1 hour)](#phase--1--active-ub-hotfixes--1-hour)
- [Phase 0 — Quick Wins (< 1 day each)](#phase-0--quick-wins--1-day-each)
- [Phase 1 — Security & Stability](#phase-1--security--stability)
- [Phase 2 — Memory Safety](#phase-2--memory-safety)
- [Phase 3 — Type Safety & Modern Idioms](#phase-3--type-safety--modern-idioms)
- [Phase 4 — Dead Code & Preprocessor Hygiene](#phase-4--dead-code--preprocessor-hygiene)
- [Phase 5 — Code Duplication](#phase-5--code-duplication)
- [Phase 6 — Performance](#phase-6--performance)
- [Phase 7 — Header & Build Hygiene](#phase-7--header--build-hygiene)
- [External / Generated — Informational Only](#external--generated--informational-only)
- [Appendix A — Full TODO/FIXME/HACK Inventory](#appendix-a--full-todofixmehack-inventory)
- [Appendix B — Per-File Issue Matrix](#appendix-b--per-file-issue-matrix)

---

## Architecture Overview

```
Starstrike (Main App — MSIX, WinUI 3, DirectX 12)
 ├─► NeuronClient  (rendering, input, UI, audio — static lib)
 │    └─► NeuronCore   (core utilities, math, networking — static lib)
 ├─► NeuronCore
 └─► GameLogic     (entities, buildings, AI, menus — static lib)
      ├─► NeuronClient
      └─► NeuronCore

NeuronServer (network server — static lib, nearly empty — see §7.8)
 └─► NeuronCore
```

> **Note:** The original plan listed GameLogic as standalone.  The `.vcxproj`
> shows GameLogic references both NeuronClient and NeuronCore.  Its `pch.h`
> includes `NeuronClient.h` which in turn includes `NeuronCore.h`, creating a
> transitive dependency on the full engine stack.

| Setting              | Value                                  |
|----------------------|----------------------------------------|
| Platform             | x64 only (GameLogic also declares Win32 configs — see §7.6) |
| Configurations       | Debug / Release                        |
| Toolset              | MSVC v145 (v143 fallback for Starstrike) |
| C++ Standard         | `stdcpplatest` (Debug); `stdcpp20` (Release); Starstrike uses `stdcpp17` on v143 fallback |
| Windows SDK          | 10.0.26100.7705                        |
| Minimum OS           | Windows 10 (10.0.17763.0)             |
| Warning Level        | W4 (Starstrike), W3 (libraries)        |
| Conformance Mode     | NeuronCore: `true` both configs; GameLogic Debug x64: `false`, Release: `true`; NeuronClient Debug: `false`, Release: `true`; Starstrike Debug: `false`, Release: not set |
| Header Guard Style   | 99 % `#pragma once` (248 headers), 2 legacy `#if !defined` (`auto_vector.h`, `opengl_directx.h`) |
| Total Source         | ~92 k lines (GameLogic 33 k, NeuronClient 37 k, Starstrike 21 k, NeuronCore 2 k) |

---

## Execution Summary

> Items marked **BUG** or **SECURITY** should be addressed before any
> **MODERNIZE** or **CLEANUP** work in the same phase.

| Phase | Open Items | Est. Effort | Blocked By | Category Mix |
|-------|-----------|-------------|------------|--------------|
| **-1** Active UB Hotfixes | 3 | 1 hour | — | BUG only |
| **0** Quick Wins | 2 | 1 day | — | BUG, CLEANUP |
| **4.2** Dead preprocessor branches | 1 | 2 days | — | CLEANUP (do before 1.1 — removes ~30 dead `sprintf`/`strcpy` sites) |
| **7.5** Release include dirs | 1 | 1 hour | — | BUG (do early — may unblock Release builds) |
| **1** Security & Stability | 7 | 2-3 weeks | §4.2 reduces scope of §1.1 | SECURITY, BUG |
| **2** Memory Safety | 4 | 3-4 weeks | — | BUG, MODERNIZE |
| **7** Build Hygiene (remaining) | 6 | 1 week | — | CLEANUP |
| **4** Dead Code (remaining) | 4 | 3 days | — | CLEANUP |
| **3** Type Safety | 5 | 4-6 weeks | §1.1 for NeuronCore net layer | MODERNIZE |
| **5** Duplication | 5 | 2-3 weeks | §3.1 for factory refactor | MODERNIZE |
| **6** Performance | 2 | 1 week | — | MODERNIZE |

### Key Dependencies

```
§4.2 (dead preprocessor) ──► §1.1 (unsafe strings — reduced scope)
§1.1 (unsafe strings)    ──► §1.8 (remove _CRT_SECURE_NO_WARNINGS)
§3.1 (enum class)        ──► §5.4 (building factory refactor)
§2.2 (EclButton fix)     ──► §2.3 (move semantics for EclButton)
```

---

## Exclusions — Legacy Conventions

The `.github/copilot-instructions.md` states:

> *Legacy code (NeuronClient, GameLogic) uses raw pointers and C-style
> strings; keep those patterns when editing those areas.*

The following modernization items conflict with this convention.  They should
only be attempted as part of a deliberate, whole-module rewrite — **not
piecemeal**:

| Item | Conflict |
|------|----------|
| §2.1 rows in GameLogic/NeuronClient | Converting raw `new`/`delete` to smart pointers in legacy code |
| §3.1 `entity.h` and `building.h` anonymous enums | `enum class` conversion touches 100s of comparison sites across all legacy code |
| §2.3 `EclButton`/`EclWindow` move semantics | Deep legacy UI classes with extensive raw-pointer usage |
| §5.4 `Building::CreateBuilding` factory | Requires §3.1 first; touches 60+ entity types |
| §5.5 Entity `Advance()`/`Render()` consolidation | Structural refactor across 30+ legacy entity files |

Limit modernization to **NeuronCore** and **Starstrike** unless a file is
already being substantially rewritten for other reasons.

---

## Verification Protocol

| Scenario | Verification |
|----------|-------------|
| **Every commit** | `Debug\|x64` and `Release\|x64` must compile with zero errors. |
| **Runtime smoke test** | Launch the app, load a level, verify rendering and input are functional. |
| **§1.1 (string safety)** | Run under AddressSanitizer (`/fsanitize=address`) in Debug to catch buffer overflows. |
| **§2.x (memory safety)** | Run under Debug CRT heap checking (`_CRTDBG_MAP_ALLOC`) to catch leaks and mismatched alloc/free. |
| **§3.1 (enum class)** | Full build + level load — implicit integer conversions will become compile errors. |
| **§4.2 (dead preprocessor)** | Diff the preprocessed output (`/P`) before and after to confirm no active code was removed. |

---

## Phase -1 — Active UB Hotfixes (< 1 hour)

These are **active undefined-behaviour bugs** that can be fixed trivially.
Ship before anything else.

### -1.1  Fix `delete` / `delete[]` mismatch in `EclButton`

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🔴 CRITICAL &nbsp;|&nbsp; **Project:** NeuronClient

The destructor uses **scalar `delete`**, but `SetCaption` / `SetTooltip`
allocate with **array `new char[]`**:

```cpp
// eclbutton.cpp — DESTRUCTOR (lines 21-23)
delete m_caption;   // ← WRONG — m_caption was allocated with new char[]
delete m_tooltip;   // ← WRONG — m_tooltip was allocated with new char[]

// eclbutton.cpp — SetCaption (line 46) / SetTooltip (line 64)
delete [] m_caption;  // ← Correct form used here
m_caption = new char [strlen(_caption) + 1];
```

Using `delete` on memory allocated with `new[]` is **undefined behaviour**.
On MSVC this typically corrupts the debug heap.

**Action:** Change the destructor to use `delete[]` on both members.

---

### -1.2  Fix format-string injection in `startsequence.cpp`

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🔴 HIGH &nbsp;|&nbsp; **Project:** Starstrike

```cpp
// startsequence.cpp:135
sprintf(theString, caption->m_caption);  // m_caption used AS the format string!
```

`caption->m_caption` is a `strdup`'d user-visible string.  Any `%` character
in the caption causes undefined behaviour (reads garbage from the stack).

**Action:** Replace with
`snprintf(theString, sizeof(theString), "%s", caption->m_caption)`.

---

### -1.3  Fix unused `errorText` in `Fatal()`

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🟠 HIGH &nbsp;|&nbsp; **Project:** NeuronCore

`Debug.h` builds the formatted error message (`errorText`) but never
passes it to `__debugbreak()` or the exception — the exception always
throws the literal `"Fatal Error"`.

| File | Lines |
|------|-------|
| `NeuronCore/Debug.h` | ~40 (narrow overload), ~47 (wide overload) |

**Action:** Include `errorText` in the exception message and/or output it
via `OutputDebugStringA`/`OutputDebugStringW` before `__debugbreak()`.

---

## Phase 0 — Quick Wins (< 1 day each)

Small, safe fixes that improve correctness or silence noise immediately.

### 0.1  ~~Remove redundant `volatile` on atomics~~ ✅ DONE

**Project:** NeuronCore &nbsp;|&nbsp; **Category:** CLEANUP

**Verified:** `ASyncLoader.h` now uses clean `std::atomic_bool` members with
`m_isLoading.wait(true)` / `notify_all()` instead of a busy-wait loop.
No `volatile` qualifier remains.

---

### 0.2  `NULL` → `nullptr`

**Category:** CLEANUP &nbsp;|&nbsp; **Project:** all &nbsp;|&nbsp; **Risk:** SAFE &nbsp;|&nbsp; **Count:** ~50 sites

| File | Lines (examples) | Notes |
|------|------------------|-------|
| `NeuronClient/NeuronClient.h` | 138-141 | `SAFE_FREE`/`SAFE_DELETE`/`SAFE_DELETE_ARRAY`/`SAFE_RELEASE` macros use `NULL` |
| `NeuronClient/preferences.h` | 64, 69 | Default params `=NULL` |
| `NeuronClient/sorting_hash_table.h` | 158, 203 | `DEBUG_ASSERT` / assignment |
| `NeuronClient/system_info.h` | 14, 31 | Member inits |
| `NeuronCore/net_socket_listener.cpp` | 43 | `(NetCallBack)NULL` cast |
| `NeuronCore/net_thread_win32.cpp` | 10 | 6× `NULL` in one line |
| `Starstrike/entity_grid.cpp` | 70-71, 212 | Member inits |
| `Starstrike/landscape.cpp` | 39, 41, 77, 99, 103, 525-528, 602-604 | Extensive |
| `Starstrike/particle_system.h` | 91 | `RGBAColour col = NULL` — **type error** (implicit `int → struct`) |
| `Starstrike/particle_system.cpp` | 319 | `col != NULL` |
| `Starstrike/GameApp.cpp` | 248, 262, 288, 325 | Various |
| `Starstrike/gamecursor.cpp` | 67 | Member init |
| `Starstrike/routing_system.cpp` | 120 | Return value |
| `Starstrike/script.cpp` | 711, 713, 722, 810, 852 | Multiple |
| `Starstrike/unit.cpp` | 260, 365, 535 | Mixed |
| `Starstrike/location.h` | 108-110 | Default params |
| `Starstrike/GameApp.h` | 44 | Comment reference |

**Action:** Replace with `nullptr`.  Fix `particle_system.h` to use a
default-constructed `RGBAColour` instead of `NULL`.

---

## Phase 1 — Security & Stability

### 1.1  Eliminate unsafe C string functions

**Severity:** 🔴 CRITICAL &nbsp;|&nbsp; **Count:** 298 call sites

`sprintf`, `strcpy`, `strcat` with no bounds checking are buffer-overflow
vectors.  Split by project for independent commits:

| Project | `sprintf` | `strcpy` | `strcat` | Total |
|---------|-----------|----------|----------|-------|
| GameLogic | 65 | 23 | 2 | 90 |
| NeuronClient | 32 | 57 | 5 | 94 |
| NeuronCore | 0 | 0 | 0 | 0 |
| Starstrike | 68 | 41 | 5 | 114 |
| **Total** | **165** | **121** | **12** | **298** |

#### 1.1a  NeuronCore ✅ ALREADY CLEAN

No unsafe C string calls remain.

#### 1.1b  GameLogic — 90 sites

**Category:** SECURITY

Key high-risk sites:

| File | Lines (sample) | Pattern |
|------|----------------|---------|
| `input_field.cpp` | 111, 112, 113, 275, 278, 281 | `strcpy`/`strcat`/`sprintf` into `m_buf[256]` |
| `darwinia_window.cpp` | 317, 325, 339, 343 | `sprintf` into `char[64]` |
| `drop_down_menu.cpp` | 253 | `sprintf` |
| `generichub.cpp` | 249, 252 | `sprintf` |
| `constructionyard.cpp` | 35, 43, 349 | `sprintf` into `char[64]` |

**Action:** Replace with `snprintf` / `strncpy` (legacy convention —
do not convert to `std::string`).

#### 1.1c  NeuronClient — 94 sites

**Category:** SECURITY

| File | Lines (sample) | Pattern |
|------|----------------|---------|
| `eclipse.cpp` | 50+ sites | `strcpy` throughout |
| `clienttoserver.cpp` | 34, 46, 56-64 | `strcpy`; raw `new` |

**Action:** Replace with `snprintf` / `strncpy` (legacy convention).

#### 1.1d  Starstrike — 114 sites

**Category:** SECURITY

| File | Lines (sample) | Pattern |
|------|----------------|---------|
| `GameApp.cpp` | 104, 183, 185, 196, 200, 214-221, 250-273, 316, 325 | `sprintf` into fixed `char[]` |
| `gamecursor.cpp` | 959, 969 | `sprintf` into `char[512]` |
| `global_world.cpp` | 193+ | `sprintf` |

**Action:** Replace with `snprintf` / `strncpy`.  Where the function is
new or already being rewritten, prefer `std::format` / `std::string`
(Starstrike is the modern app layer).

---

### 1.2  Fix `MAX_PACKET_SIZE` macro collision

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Project:** NeuronCore

`MAX_PACKET_SIZE` is defined with **two different values** in the same
project:

| File | Value | Used by |
|------|-------|---------|
| `NeuronCore/net_lib.h` | `512` | `net_udp_packet.h` → `char m_data[MAX_PACKET_SIZE]` |
| `NeuronCore/DataReader.h` | `1500` | `DataReader` / `DataWriter` buffer sizing |
| `NeuronCore/DataWriter.h` | `1500` | (same) |

Neither `DataReader.h` nor `DataWriter.h` are currently `#include`'d by any
`.cpp` file, so the collision does not trigger a compiler warning **today**.
This is a latent code-smell, not an active bug.

**Action:** Rename the constant in `DataReader.h` / `DataWriter.h` to
`DATAWRITER_BUFFER_SIZE` (or similar) so no collision is possible regardless
of future include order.  The network-layer value in `net_lib.h` (512) is
the authoritative packet size.

---

### 1.3  ~~Fix potential logic error in `net_socket_listener.cpp`~~

**Severity:** 🟢 FALSE POSITIVE &nbsp;|&nbsp; **Project:** NeuronCore

**Verified:** The `(!NetIsAddrInUse)` conditional is correct.  No change needed.

---

### 1.4  ~~Fix memory leak in `net_socket_listener.cpp`~~ ✅ DONE

**Severity:** 🔴 HIGH &nbsp;|&nbsp; **Project:** NeuronCore

**Fixed:** `datasize <= 0` guard added; allocation wrapped in
`std::make_unique` with `.release()`.

---

### 1.5  Fix unsafe `void*` dispatching in `darwinia_window.cpp`

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Project:** GameLogic

`CreateValueControl()` casts a `void* value` to `unsigned char*`, `int*`,
`float*`, `char*` based on a runtime `dataType` parameter with no compile-
time safety.  This pattern is stable legacy code that has worked for years;
the risk is maintainability, not an active crash.

| File | Lines |
|------|-------|
| `GameLogic/darwinia_window.cpp` | 300-309 |

**Action:** Replace with a type-safe variant (`std::variant` or tagged
union) or template the function — **only if this file is being rewritten
for other reasons** (see [Exclusions](#exclusions--legacy-conventions)).

---

### 1.6  Fix `sprintf` overlapping source/destination (UB)

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🟠 HIGH &nbsp;|&nbsp; **Project:** Starstrike

```cpp
// GameApp.cpp:325  (inside #ifdef TARGET_OS_VISTA — dead code today, but still UB)
sprintf(dir, "%s\\", dir);  // dir is both input and output
```

Reading and writing the same buffer through `sprintf` is undefined behaviour
per the C standard.  Even though this code path is currently unreachable
(`TARGET_OS_VISTA` is never defined), it should be fixed or removed.

**Action:** Remove the entire `TARGET_OS_VISTA` branch (see §4.2), or
rewrite with `snprintf` into a temporary buffer.

---

### 1.7  Audit `_CRT_SECURE_NO_WARNINGS` global suppression

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Project:** NeuronCore

`NeuronCore.h` line 4 defines `_CRT_SECURE_NO_WARNINGS`, which propagates
through every project's `pch.h` chain.  This globally silences MSVC warnings
for every unsafe `sprintf`/`strcpy`/`strcat` call — the exact functions
listed in §1.1.

**Action:** Remove once §1.1 is addressed.  Until then, leave in place to
avoid a flood of warnings.  **Depends on:** §1.1.

---

### 1.8  `atoi` / `atof` with no error checking

**Category:** SECURITY &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 250 sites

`atoi` / `atof` / `atol` perform no overflow detection and have no way to
report parse failure.

| Project | Count |
|---------|-------|
| GameLogic | 86 |
| NeuronClient | 68 |
| Starstrike | 96 |
| NeuronCore | 0 |
| **Total** | **250** |

**Action:**
- In **NeuronCore**: already clean.
- In **legacy projects**: replace with `strtol` / `strtof` as a first pass
  (adds error detection with minimal API change).
- In **Starstrike** (modern layer): prefer `std::from_chars` where feasible.

---

### 1.9  `const_cast` workaround in `file_paths.cpp`

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🟢 LOW &nbsp;|&nbsp; **Project:** NeuronClient

```cpp
// file_paths.cpp:60
string kb = g_prefsManager->GetString(
    KEYBOARD_LAYOUT.c_str(), const_cast<char*>(defLocale.c_str()));
```

The root cause is `PrefsManager::GetString` (`preferences.h:69`) taking
`char *_default` instead of `const char *_default`.

**Action:** Change `GetString`'s `_default` parameter to `const char*`.
Remove the `const_cast`.

---
## Phase 2 — Memory Safety

### 2.1  Convert raw `new`/`delete` to RAII containers

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟠 HIGH &nbsp;|&nbsp; **Count:** 100+ allocation sites

The codebase relies on manual `new`/`delete` across every project.  Missed
`delete` paths cause leaks; double-`delete` paths cause crashes.

> **Scope note:** Per the [Exclusions](#exclusions--legacy-conventions),
> limit RAII conversion to **Starstrike** and **NeuronCore**.  In
> GameLogic/NeuronClient, only convert allocations that are being touched
> for other reasons (e.g. fixing a leak).

**Key areas (Starstrike — in scope):**

| File | Lines (sample) | Pattern |
|------|----------------|---------|
| `3d_sierpinski_gasket.cpp` | 16, 65 | `new T[]` / `delete[] m_points` |
| `entity_grid.cpp` | 70-80, 137-138, 226, 240, 245, 262 | Dynamic arrays |
| ~~`explosion.cpp`~~ | | ✅ Migrated to `std::vector` members |
| `water.cpp` | 65, 79-80, 204, 227, 289, 343-344 | Multiple heap arrays |
| `global_world.cpp` | 77-89, 312, 348, 740, 1249-1251 | Global structures |
| `GameApp.cpp` | 66-120, 315, 347-397 | App subsystem pointers |
| `gamecursor.cpp` | 36-67, 236, 261 | MouseCursor objects |
| `global_internet.cpp` | 87-88, 115-117 | Array allocations |

**Key areas (Legacy — convert only when already editing):**

| File | Lines (sample) | Pattern |
|------|----------------|---------|
| `NeuronClient/bitmap.cpp` | 366-378 | `m_pixels = new RGBAColour[…]` |
| `NeuronClient/clienttoserver.cpp` | 34, 46, 56-64, 309-425 | Network letters |
| `NeuronClient/2d_array.h` | 60, 67, 79 | Template container |
| `NeuronClient/bounded_array.h` | 55, 64, 77, 86 | Template container |
| `NeuronClient/hash_table.h` | 134-136, 193-194, 207-208 | Hash map backing arrays |
| `NeuronClient/eclbutton.cpp` | 49, 54, 65 | `new char[]` for caption/tooltip |
| `GameLogic/building.cpp` | 627-795 | 60+ factory `new` calls |
| `GameLogic/engineer.cpp` | 360 | `delete position` |
| `GameLogic/entity.cpp` | 71 | `delete theFile` |
| `GameLogic/spam.cpp` | 476 | `delete target` |
| `GameLogic/spawnpoint.cpp` | 244 | `delete availableLinks` |

**Action (Starstrike):**
1. Convert owned array members to `std::vector<T>` or `std::unique_ptr<T[]>`.
2. Convert owned single-object members to `std::unique_ptr<T>`.

**Action (Legacy — opportunistic):**
1. Template containers (`2d_array.h`, `bounded_array.h`, `hash_table.h`):
   replace internal `new T[]` with `std::vector<T>`.
2. Factory allocations (`Building::CreateBuilding`): return
   `std::unique_ptr<Building>` — only after §3.1 enum class conversion.

---

### 2.2  Add move semantics to resource-owning classes

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM

Classes that own heap resources lack move constructors / assignment:

| Class | File |
|-------|------|
| `BitmapRGBA` | `NeuronClient/bitmap.h` |
| `EclButton` | `NeuronClient/eclbutton.h` |
| `EclWindow` | `NeuronClient/eclwindow.h` |

**Action:** Implement Rule-of-Five or convert to `std::vector` / `std::string` /
smart-pointer members to get implicit moves.  See
[Exclusions](#exclusions--legacy-conventions) — prefer minimal fixes
(e.g. `= delete` copy ops to prevent accidental copies) over full rewrites.

**Depends on:** §-1.1 (EclButton destructor fix).

---

### 2.3  Audit `strdup` / `free` pairing

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 42 `strdup` sites

`strdup` allocates with `malloc` — the returned pointer **must** be released
with `free`, never `delete`.  Most sites pair correctly, but several never
free the memory at all:

| File | Line | Expression | Freed? |
|------|------|-----------|--------|
| `GameLogic/entity.cpp` | 63 | `m_names[entityIndex] = strdup(…)` | ❌ Static array, never freed |
| `NeuronClient/text_renderer.cpp` | 39 | `m_filename = strdup(…)` | ❌ No destructor frees it |
| `NeuronClient/user_info.cpp` | 23, 29 | `s_username`/`s_email = strdup(…)` | ❌ Static globals, never freed |
| `GameLogic/prefs_other_window.cpp` | 145 | `m_languages.PutData(strdup(lang))` | ❌ LList destroyed but data not freed |
| `Starstrike/startsequence.cpp` | 42 | `caption->m_caption = strdup(…)` | ❌ Caption struct never freed |
| `NeuronClient/system_info.cpp` | 48 | `m_deviceNames[i] = strdup(…)` | ✅ `system_info.h:35` frees |
| `NeuronClient/shape.cpp` | 16-17, 47, 58, … | `m_name`/`m_parentName = strdup(…)` | ✅ Freed in destructors |
| `NeuronClient/preferences.cpp` | 37, 73, 80, … | `m_key`/`m_str = strdup(…)` | ✅ Freed in destructor |
| `Starstrike/gamecursor.cpp` | 959, 969 | `m_mainFilename`/`m_shadowFilename` | ✅ Freed at 978-979 |

**Action:** Fix each leaked `strdup` by adding a matching `free` in the
destructor.  In Starstrike, prefer converting the member to `std::string`.

---

### 2.4  Fix `memset(this, 0, sizeof(*this))` on non-trivial types

**Category:** BUG &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM

`memset` on `this` is undefined behaviour if the type has non-trivial
members (vtable pointers, non-POD members, etc.).

| File | Line | Type | Trivial? |
|------|------|------|----------|
| `NeuronClient/matrix33.cpp` | 388 | `Matrix33` | Likely POD — verify |
| `NeuronClient/matrix34.cpp` | 323 | `Matrix34` | Likely POD — verify |
| `Starstrike/team.cpp` | 598 | `TeamControls` | ❌ Contains `LegacyVector3` member |

**Action:** For `TeamControls::Clear()`, replace `memset` with explicit
member zeroing or `= TeamControls{}`.  For the matrix types, verify they are
trivially copyable (`static_assert(std::is_trivially_copyable_v<Matrix33>)`
— if so, `memset` is safe but should be documented with a comment.

---

## Phase 3 — Type Safety & Modern Idioms

### 3.1  Convert old-style enums to `enum class`

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟠 HIGH &nbsp;|&nbsp; **Count:** 9 enums

> ⚠️ **Risk:** The `entity.h` and `building.h` anonymous enums have 60+
> values used across hundreds of comparison sites in GameLogic and
> Starstrike.  This is a multi-week refactor with high regression risk.
> See [Exclusions](#exclusions--legacy-conventions).

| File | Enum | Current | Scope |
|------|------|---------|-------|
| `NeuronCore/net_lib.h` | `NetRetCode` | Plain `enum` | NeuronCore only — **safe to convert first** |
| `NeuronClient/control_types.h` | `ControlType` | Plain `enum` | NeuronClient + Starstrike |
| `NeuronClient/inputdriver.h` | `InputCondition` | Plain `enum` | NeuronClient |
| `NeuronClient/inputdriver.h` | `InputParserState` | Plain `enum` | NeuronClient |
| `NeuronClient/input_types.h` | `InputType` | Plain `enum` | NeuronClient + Starstrike |
| `NeuronClient/input_types.h` | `InputMode` | Plain `enum` | NeuronClient + Starstrike |
| `NeuronClient/networkupdate.h` | Network update types | Plain `enum` | NeuronClient |
| `GameLogic/entity.h` | Entity types | Anonymous `enum` | ⚠️ **All projects** — defer |
| `GameLogic/building.h` | Building types (60+ values) | Anonymous `enum` | ⚠️ **All projects** — defer |

**Action:** Convert `NetRetCode` first (NeuronCore-only, minimal blast
radius).  Then convert the NeuronClient enums one at a time.  **Defer**
`entity.h` and `building.h` until test coverage exists.

---

### 3.2  Add `[[nodiscard]]`, `noexcept`, `constexpr` where appropriate

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM

| Idiom | Where to Add |
|-------|-------------|
| `[[nodiscard]]` | Getter functions returning allocated memory, `Hash.h` `HashRange()`/`HashState()`, network socket operations |
| `noexcept` | All `TimerCore.h` static getters (6 functions at lines 23-32), trivial accessors across NeuronCore |
| `constexpr` | Math constants, magic numbers used as array sizes |

**Action:** Audit NeuronCore first (modern C++ project), then extend to
Starstrike headers.  Do **not** retrofit into GameLogic/NeuronClient legacy
code unless editing those functions for other reasons.

---

### 3.3  Replace `const char*` with `std::string_view` in NeuronCore APIs

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM

NeuronCore is the modern layer but still exposes `const char*` / `char*`
parameters in the networking layer (`net_socket.h` `Connect(char*, …)`).

**Action:** Where the callee does not need to own the string, change to
`std::string_view`.

---

### 3.4  Modernise NeuronCore networking layer

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Files:** `net_*.h/cpp` (8 files)

| Legacy Pattern | Modern Replacement |
|----------------|--------------------|
| `CRITICAL_SECTION` (`net_mutex.h` via `net_lib_win32.h` macro) | `std::mutex` / `std::lock_guard` |
| `CreateThread()` (`net_thread_win32.cpp` line 10) | `std::thread` / `std::jthread` |
| Function-pointer callbacks (`net_lib_win32.h`) | `std::function<>` or `std::move_only_function` |
| `memset()` on structs in constructors (`net_socket.cpp` lines 16-18) | Aggregate / in-class init |
| `MIN()` macro (`net_lib.h`) | `std::min()` |
| C-style casts (`net_socket.cpp` lines 191, 200-201, 229) | `static_cast<>` / `reinterpret_cast<>` |
| `(NetCallBack)NULL` (`net_socket_listener.cpp` line 43) | `nullptr` |
| `(FILE *)0` (`net_socket.cpp` line 12, 43) | `nullptr` |
| Hardcoded magic numbers (`100`, `10000`, `10`) | Named constants |

**Action:** Modernise one file at a time, build after each, starting with
`net_mutex` → `net_thread` → `net_socket` → `net_socket_listener`.

---

### 3.5  Template DataWriter boilerplate

**Category:** MODERNIZE &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **File:** `NeuronCore/DataWriter.h`

7 identical methods (`WriteInt16`/`WriteUInt16`/`WriteInt32`/`WriteUInt32`/
`WriteInt64`/`WriteUInt64`/`WriteFloat`) differ only in the type and the
assert message string.

**Action:** Replace with a single `Write<T>(const T& value)` template
constrained to arithmetic types.  Keep `WriteChar`/`WriteByte` (single-byte
special cases), `WriteString`, and `WriteRaw`.

---

## Phase 4 — Dead Code & Preprocessor Hygiene

### 4.1  Remove commented-out code blocks

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 30+ blocks across 20+ files

Version control preserves history; commented code should be removed.

| File | Lines (sample) | Description |
|------|----------------|-------------|
| `NeuronClient/clienttoserver.cpp` | 154-167 | Old `gethostbyname()` code |
| `NeuronClient/control_bindings.cpp` | 100-106 | Disabled `unique_ptr` logic |
| `NeuronClient/hash_table.h` | 102-114 | Alternative hash function |
| `NeuronClient/sound_filter.cpp` | 166-175, 297-305 | Audio processing blocks |
| `GameLogic/ai.cpp` | 477-482 | Original AI loop |
| `GameLogic/airstrike.cpp` | 221-222 | Dead conditional |
| `GameLogic/anthill.cpp` | 386-389 | Objective iteration |
| `GameLogic/armyant.cpp` | 494 | Push-from-obstructions |
| `GameLogic/armour.cpp` | 472 | Commented `#ifdef DEBUG_RENDER_ENABLED` |
| `GameLogic/blueprintstore.cpp` | 315-316 | Commented loop |
| `GameLogic/building.cpp` | 499 | Alternate hit-test |
| `GameLogic/darwinian.cpp` | 495-496 | Control flow |
| `GameLogic/entity.cpp` | 356-357 | Alternatives |
| `GameLogic/entity_leg.cpp` | 73 | Score calculation |
| `GameLogic/generichub.cpp` | 65 | Power link |
| `GameLogic/officer.cpp` | 876 | Alpha fade |
| `GameLogic/spirit.cpp` | 148 | Velocity calc |
| `GameLogic/tripod.cpp` | 430-432 | Multiple lines |
| `GameLogic/virii.cpp` | 203 | Position update |
| `GameLogic/weapons.cpp` | 651 | Shape loading |
| `GameLogic/drop_down_menu.cpp` | 337 | Menu index |
| `NeuronClient/preferences.cpp` | 591 | `strdup` line |
| `Starstrike/camera.cpp` | 70, 72, 277, 667, 1004, 1010, 1333-1336, 1449-1452, 1517-1523 | Camera input |
| `Starstrike/user_input.cpp` | 92-94 | Commented `PROFILER_ENABLED` block |

**Action:** Delete all.  Commit with a message listing what was removed.

---

### 4.2  Remove stale preprocessor branches

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 9 defines

> **Execute before §1.1** — removing dead platform branches eliminates
> ~30 `sprintf`/`strcpy` sites, reducing the scope and diff noise of the
> unsafe-string pass.

| Define | Files | Status |
|--------|-------|--------|
| `DEMO2` | `ai.cpp:110`, `mainmenus.cpp:332` | Dead demo build — `#ifdef DEMO2` never defined |
| `JAMES_FIX` | `global_world.cpp:142-145`, `gunturret.cpp:254-258` | Developer-specific fix guard |
| `TARGET_OS_LINUX` | `GameApp.cpp:182,239,246`, `preferences.cpp:290` | Unsupported platform — never defined |
| `TARGET_OS_MACOSX` | `GameApp.cpp:260`, `preferences.cpp:174,186,198,211,283,290`, `prefs_keybindings_window.cpp:232,296` | Unsupported platform — never defined |
| `TARGET_OS_VISTA` | `GameApp.cpp:282,300,324`, `main.h:11`, `prefs_other_window.cpp:71` | Never defined; creates dead extern declarations in `main.h` |
| `TARGET_VISTA_DEMO2` | `GameApp.cpp:292`; defined as commented-out in `NeuronClient.h:95` | Dead build variant |
| `TARGET_FULLGAME` | `NeuronClient.h:89` (commented out) | Dead target — only referenced in `#error` guard |
| `TARGET_DEMOGAME` | `NeuronClient.h:90` (commented out), `GameApp.cpp:182` | Dead target |
| `TARGET_DEMO2` | `NeuronClient.h:92` (commented out) | Dead target — only in `#error` guard |

> **Correction from prior version:** `DEBUG_RENDER_ENABLED` is **not dead**.
> It is `#define`'d unconditionally at `NeuronClient.h:99` (outside any
> `#ifdef` guard).  The code it gates in `airstrike.cpp:241`,
> `anthill.cpp:382`, `darwinian.cpp:1983` **is** compiled.  It should be
> evaluated for usefulness but is **not** a stale define to collapse.

**Action:** Verify each define is never set in any active configuration
(check `.vcxproj` preprocessor definitions).  Collapse each dead branch,
keeping the active path.  For the `NeuronClient.h` target selection block
(lines 87-120), remove all commented-out `#define` lines and the `#error`
guard, leaving only `#define TARGET_DEBUG`.

Use preprocessed output diff (`/P` flag) to verify no active code was
removed (see [Verification Protocol](#verification-protocol)).

---

### 4.3  Rename/document `fixMeUp` static variables

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **File:** `Starstrike/camera.cpp`

Three statics named `fixMeUp` (lines 291, 327, 382) indicate incomplete
work.

**Action:** Rename to descriptive names (`s_cameraBootFov`,
`s_cameraXInitialised`, `s_cameraYInitialised`) and document intent.

---

### 4.4  Remove `JAK HACK` in `water.cpp`

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW &nbsp;|&nbsp; **File:** `Starstrike/water.cpp` line 452

Disabled hack comment — if the hack is no longer active, remove the dead
code block entirely.

---

### 4.5  Remove `DELETEME`-tagged includes and code

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW

| File | Line | Content | Cross-ref |
|------|------|---------|-----------|
| `Starstrike/3d_sierpinski_gasket.cpp` | 7-8 | `#include "GameApp.h" // DELETEME` and `#include "camera.h" // DELETEME` | Appendix A #11 |
| `Starstrike/main.cpp` | 374 | `// DELETEME: for debug purposes only` | Appendix A #12 |

**Action:** Verify the includes are truly unused (grep for symbols from those
headers used in the file), then remove.

---

## Phase 5 — Code Duplication

### 5.1  Consolidate `DataWriter` write methods

(See Phase 3.5 above — listed here for cross-reference.)

---

### 5.2  `PROFILER_ENABLED` double definition

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW

`PROFILER_ENABLED` is defined in two places that both flow through the same
pch chain:

| File | Line | Guard |
|------|------|-------|
| `NeuronCore/NeuronCore.h` | 8 | `#if defined (_DEBUG)` |
| `NeuronClient/NeuronClient.h` | 104 | `#ifndef _OPENMP` |

Both definitions expand to the same empty macro, so no functional difference
today, but the NeuronClient definition has a **different guard condition**
(it defines `PROFILER_ENABLED` even in Release if `_OPENMP` is not set).

**Action:** Remove the `NeuronClient.h` definition.  Rely solely on the
`NeuronCore.h` definition under `_DEBUG`.

---

### 5.3  Extract repeated OpenGL quad-rendering helpers

**Category:** MODERNIZE &nbsp;|&nbsp; **Projects:** GameLogic, Starstrike

`glBegin(GL_QUADS) / glVertex3fv / glEnd` sequences appear 20+ times
across `darwinia_window.cpp`, `blueprintstore.cpp`, `water.cpp`,
`clouds.cpp` and others.

**Action:** Extract into a shared `RenderQuad()` / `RenderTexturedQuad()`
utility, or migrate the draw calls to `SpriteBatch` where appropriate.

---

### 5.4  Refactor `Building::CreateBuilding` factory

**Category:** MODERNIZE &nbsp;|&nbsp; **File:** `GameLogic/building.cpp` lines 627-795

169 lines of `if (_type == TypeX) building = new X();` chains.

**Action:** Replace with a static registration map:

```cpp
using BuildingFactory = std::unique_ptr<Building>(*)();
static const std::unordered_map<int, BuildingFactory> s_factories = { … };
```

**Depends on:** §3.1 (enum class for building types).
See [Exclusions](#exclusions--legacy-conventions).

---

### 5.5  Consolidate entity `Advance()`/`Render()` boilerplate

**Category:** MODERNIZE &nbsp;|&nbsp; **Projects:** GameLogic (30+ entity files)

Many entity types repeat identical health-check, movement, and render
preamble code.

**Action:** Factor common logic into `Entity::AdvanceBase()` /
`Entity::RenderBase()` called by each override.  Alternatively, investigate
a component/composition approach for shared behaviours.

See [Exclusions](#exclusions--legacy-conventions).

---

## Phase 6 — Performance

### 6.1  ~~Replace busy-wait in `ASyncLoader`~~ ✅ DONE

**File:** `NeuronCore/ASyncLoader.h`

**Verified:** Now uses C++20 `std::atomic::wait(true)` / `notify_all()`
instead of a spin loop.  No busy-wait remains.

---

### 6.2  Fix slow area-recalculation in `location.cpp`

**Category:** MODERNIZE &nbsp;|&nbsp; **File:** `Starstrike/location.cpp` lines 602, 637

Existing TODO: *"This is WAY too slow, should only recalculate the areas
affected."*

**Action:** Implement dirty-region tracking so only modified cells are
recalculated.

---

### 6.3  Pre-allocate entity grid arrays

**Category:** MODERNIZE &nbsp;|&nbsp; **File:** `Starstrike/entity_grid.cpp`

Dynamic array resizing without `.reserve()` or pre-allocation hints.
`entity_grid.cpp:244` uses manual `new`/`memcpy`/`delete` to grow a
neighbour array — a textbook case for `std::vector`.

**Action:** Profile and add `reserve()` calls based on expected entity
counts.  Convert the manual realloc pattern at line 240-245 to
`std::vector<WorldObjectId>`.

---

## Phase 7 — Header & Build Hygiene

### 7.1  ~~Convert `#ifndef` guards to `#pragma once`~~ ✅ MOSTLY DONE

**Count:** 248 of 250 headers now use `#pragma once`.

| File | Current Guard | Action |
|------|---------------|--------|
| `NeuronClient/auto_vector.h` | `#if !defined AUTO_VECTOR_H` | Convert to `#pragma once` |
| `NeuronClient/opengl_directx.h` | `#if !defined __OPENGL_DIRECTX` | Convert to `#pragma once` |

All other headers across all projects already use `#pragma once`.

---

### 7.2  Raise warning level on library projects

**Category:** CLEANUP &nbsp;|&nbsp; Currently GameLogic, NeuronClient, NeuronCore, and NeuronServer compile at
`/W3`.  Starstrike compiles at `/W4`.

**Action:** Raise all libraries to `/W4` and address new warnings.
Consider enabling `/we4505` (unreferenced local function) and `/we4189`
(unreferenced local variable) to catch dead code automatically.

---

### 7.3  Enable Conformance Mode consistently

**Category:** CLEANUP

| Project | Debug | Release |
|---------|-------|---------|
| NeuronCore | `true` ✅ | `true` ✅ |
| NeuronClient | **`false`** ❌ | `true` ✅ |
| GameLogic (x64) | **`false`** ❌ | `true` ✅ |
| Starstrike | **`false`** ❌ | not set (inherits default) |

**Action:** Enable `ConformanceMode=true` for all projects in all
configurations.  Fix any resulting compilation errors — these indicate
non-conformant code that may break with future compiler updates.

---

### 7.4  Audit legacy `#pragma warning(disable:…)` in `NeuronCore.h`

**Category:** CLEANUP

Warnings 4201, 4238, 4244, 4324 are globally suppressed at
`NeuronCore.h` lines 11-14.  Combined with `_CRT_SECURE_NO_WARNINGS`
(line 4) and `_WINSOCK_DEPRECATED_NO_WARNINGS` (line 5), these suppress a
significant amount of useful diagnostic information.

**Action:** Re-enable one at a time, build, fix or locally suppress where
truly needed, and remove the global suppression.

---

### 7.5  Verify `AdditionalIncludeDirectories` in Release configs

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟡 MEDIUM

Multiple projects set `AdditionalIncludeDirectories` (cross-project
`#include` search paths) **only in the Debug configuration**:

| Project | Debug | Release |
|---------|-------|---------|
| NeuronCore | `$(SolutionDir)GameLogic;$(SolutionDir)NeuronClient;$(SolutionDir)StarStrike` | ❌ Not set |
| NeuronClient | `$(SolutionDir)NeuronCore;$(SolutionDir)GameLogic;$(SolutionDir)StarStrike` | ❌ Not set |
| GameLogic | `$(SolutionDir)NeuronCore;$(SolutionDir)NeuronClient;$(SolutionDir)StarStrike` | ❌ Not set |
| Starstrike | `$(SolutionDir)NeuronCore;$(SolutionDir)NeuronClient;$(SolutionDir)GameLogic` | ❌ Not set |

The pch chain's `NeuronClient.h` → `NeuronCore.h` transitive includes
likely resolve all cross-project headers today, which is why Release builds
succeed.  However, any future `#include` outside this chain would silently
break only in Release.

**Action:** Attempt a clean `Release|x64` build first.  If it succeeds,
this is a latent risk, not an active bug — still worth fixing by moving
`AdditionalIncludeDirectories` to a non-conditional `ItemDefinitionGroup`
or duplicating it into the Release configuration.

---

### 7.6  Remove dead Win32 configurations from GameLogic

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW

`GameLogic.vcxproj` defines `Debug|Win32` and `Release|Win32`
configurations, but the solution is x64-only.  These configurations add
clutter and may have stale settings.

**Action:** Remove the Win32 `ProjectConfiguration` items and all
`Condition="…Win32…"` blocks from `GameLogic.vcxproj`.

---

### 7.7  Shader compilation only configured for Debug

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW

`NeuronClient.vcxproj` configures HLSL shader compilation (`FxCompile` for
`PixelShader.hlsl` and `VertexShader.hlsl`) with `VariableName`,
`HeaderFileOutput`, `ShaderType`, `ShaderModel`, and `AdditionalOptions`
**only** under `Condition="Debug|x64"`.  Release builds may use stale
compiled shader headers or default settings.

**Action:** Add matching `FxCompile` settings for the Release configuration.

---

### 7.8  Evaluate NeuronServer project

**Category:** CLEANUP &nbsp;|&nbsp; **Severity:** 🟢 LOW

`NeuronServer` contains only 3 files totalling 5 lines of code (`pch.cpp`,
`pch.h`, `NeuronServer.h`) and no functional logic.  It is a placeholder.

**Action:** Decide whether to keep it as a scaffold for future server work
or remove it from the solution to reduce build times and clutter.

---

## External / Generated — Informational Only

The following files appear in the codebase but should **not** be modified
directly.  Issues in them are noted here for awareness, not as action items.

### `NeuronClient/d3dx12.h` — Microsoft-provided helper

Copyright (c) Microsoft Corporation, MIT License.  7,779 lines.
Contains `TODO` at line 5062 (SDK version check).  This is Microsoft's
responsibility — do not edit.

### `NeuronClient/CompiledShaders/*.h` — HLSL compiler output

`PixelShader.h` and `VertexShader.h` are generated by the HLSL compiler
(`FxCompile` step in the build).  The `#if 0` blocks around shader debug
info are standard compiler output.  Regenerated automatically when shaders
are recompiled — do not edit.

---

## Appendix A — Full TODO/FIXME/HACK Inventory

> Items marked with a § cross-reference are actionable in the corresponding
> plan section.  Items without a cross-reference are informational — they
> represent acknowledged tech debt or future work.

| # | File | Line | Text | Cross-ref |
|---|------|------|------|-----------|
| 1 | `Starstrike/location.cpp` | 602 | TODO: This is WAY too slow, should only recalculate the areas affected | §6.2 |
| 2 | `Starstrike/location.cpp` | 637 | TODO: This is WAY too slow, should only recalculate the areas affected | §6.2 |
| 3 | `Starstrike/location.cpp` | 1338 | TODO: This is sent by the server. ACK!!! | — |
| 4 | `Starstrike/location.cpp` | 1357 | TODO: When do we want to create a marker? | — |
| 5 | `Starstrike/camera.cpp` | 71 | TODO: Support mouse/joystick | — |
| 6 | `Starstrike/camera.cpp` | 85 | TODO: Really? | — |
| 7 | `Starstrike/camera.cpp` | 576 | TODO: Support mouse/joystick | — |
| 8 | `Starstrike/camera.cpp` | 1439 | JAMES_TODO: Support directional firing mode | §4.2 |
| 9 | `Starstrike/location_input.cpp` | 245 | TODO: Really? | — |
| 10 | `Starstrike/water.cpp` | 452 | JAK HACK (DISABLED) | §4.4 |
| 11 | `Starstrike/3d_sierpinski_gasket.cpp` | 7-8 | DELETEME (includes) | §4.5 |
| 12 | `Starstrike/main.cpp` | 374 | DELETEME: for debug purposes only | §4.5 |
| 13 | `GameLogic/entity_leg.cpp` | 104 | FIXME — cosTheta should never be greater than one, yet sometimes it is | — |
| 14 | `NeuronClient/inputdriver_alias.cpp` | 72 | TODO: This isn't quite right | — |
| 15 | `NeuronClient/inputdriver_idle.cpp` | 136 | TODO: This | — |
| 16 | `NeuronClient/keydefs.h` | 103 | TODO: Any key not implemented yet | — |
| 17 | `NeuronClient/opengl_directx.cpp` | 970 | TODO | — |
| 18 | `NeuronClient/opengl_directx.cpp` | 1024 | TODO | — |
| 19 | `NeuronClient/opengl_directx.cpp` | 2082 | TODO: implement via constant buffer clip planes in shader | — |
| 20 | `NeuronClient/soundsystem.cpp` | 13 | FIXME | — |
| 21 | `NeuronClient/GraphicsCommon.h` | 38 | XXX | — |
| 22 | `NeuronClient/sound_parameter.cpp` | 142 | TODO: version compatibility check | — |

---

## Appendix B — Per-File Issue Matrix

Legend: **S** = Security, **M** = Memory, **T** = Type Safety, **D** = Dead Code,
**P** = Performance, **H** = Header Hygiene, **DUP** = Duplication

Severity key: ●○ = two distinct issues in column (e.g. separate bug + leak)

### NeuronCore

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `ASyncLoader.h` | | | | | | | | ✅ Uses `atomic::wait`/`notify_all`; no busy-wait; no `volatile` |
| `DataReader.h` | ● | | ● | | | | | `MAX_PACKET_SIZE` redefined to 1500 (collision with `net_lib.h` 512); raw ptr param → use `std::span` |
| `DataWriter.h` | ● | | | | | | ● | `MAX_PACKET_SIZE` redefined to 1500; 7 identical write methods |
| `Debug.h` | | ● | | | | | | `errorText` unused in `Fatal()` |
| `FileSys.h` | | | | | | | | Legacy A/W suffix |
| `Hash.h` | | | ● | | | | | C-style pointer casts in `HashRange()` |
| `MathCommon.h` | | | | | | | | ✅ Already uses `std::bit_width` |
| `NeuronCore.h` | | | | | | ● | | Global warning suppressions; `_CRT_SECURE_NO_WARNINGS`; `_WINSOCK_DEPRECATED_NO_WARNINGS` |
| `TimerCore.h` | | | ● | | | | | Missing `noexcept` on 6 static getters |
| `net_lib.h` | ● | | ● | | | | | Plain enum; `MIN` macro; `MAX_PACKET_SIZE` = 512 (authoritative value) |
| `net_lib_win32.h` | | | ● | | | | | Function pointers → `std::function`; `CRITICAL_SECTION` typedef |
| `net_mutex*.h/cpp` | | | ● | | | | | `CRITICAL_SECTION` → `std::mutex` |
| `net_socket*.h/cpp` | ● | ● | ● | | | | | C casts; magic numbers; `memcpy` overflow risk; `(FILE*)0` |
| `net_thread_win32.cpp` | | | ● | | | | | `CreateThread` → `std::thread`; 6× `NULL` |
| `net_udp_packet.h/cpp` | | | | | | | | Fixed `char[512]` buffer; `MIN` macro |

### NeuronClient

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `2d_array.h` | | ● | | | | | | `new T[]` |
| `auto_vector.h` | | | | | | ● | | Legacy `#if !defined` guard |
| `bitmap.cpp` | | ● | | | ● | | | `new RGBAColour[]`; temp allocs in filters |
| `bounded_array.h` | | ● | | | | | | `new T[]` |
| `clienttoserver.cpp` | ● | ● | | ● | | | | `strcpy`; raw `new`; dead gethostbyname |
| `control_bindings.cpp` | | | | ● | | | | Disabled unique_ptr logic |
| `control_types.h` | | | ● | | | | | Plain enum |
| `eclbutton.h/cpp` | | ●○ | | | | | | **`delete`/`delete[]` mismatch (UB)** §-1.1; `char*` members |
| `eclipse.cpp` | ● | | | | | | | 50+ `strcpy` |
| `file_paths.cpp` | | | ● | | | | | `const_cast` workaround §1.9 |
| `hash_table.h` | | ● | | ● | | | | Raw arrays; dead alt hash; `memset` on generic `T` |
| `inputdriver.h` | | | ● | | | | | 2 plain enums |
| `input_types.h` | | | ● | | | | | 2 plain enums |
| `matrix33.cpp` | | | ● | | | | | `memset(this, 0, …)` — verify trivially copyable §2.4 |
| `matrix34.cpp` | | | ● | | | | | `memset(this, 0, …)` — verify trivially copyable §2.4 |
| `networkupdate.h` | | | ● | | | | | Plain enum |
| `NeuronClient.h` | | | | ● | | ● | ● | Mega-header mixes modern + legacy; `SAFE_*` macros use `NULL`; dead target `#define`s; duplicate `PROFILER_ENABLED` |
| `opengl_directx.h` | | | | | | ● | | Legacy `#if !defined` guard |
| `opengl_directx.cpp` | | | | ● | | | | 3 TODO stubs |
| `preferences.h` | | | ● | | | | | `GetString` takes `char*` not `const char*` §1.9 |
| `preferences.cpp` | | ● | | | | | | Heavy `strdup` usage (paired with `free` — OK) |
| `sound_filter.cpp` | | | | ● | | | | Disabled audio blocks |
| `text_renderer.cpp` | | ● | | | | | | `m_filename = strdup(…)` never freed §2.3 |

### GameLogic

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `building.h` | | | ● | | | | | Anonymous enum (60+ values) — defer §3.1 |
| `building.cpp` | | ● | | | | | ● | 169-line factory; 60+ `new` — defer §5.4 |
| `darwinia_window.cpp` | ● | | ● | | | | ● | `void*` casts; `sprintf`; repeated GL |
| `drop_down_menu.cpp` | ● | | | ● | | | | `strcpy`/`sprintf`; dead code |
| `entity.h` | | | ● | | | | | Anonymous enum — defer §3.1 |
| `entity.cpp` | | ● | | | | | | `m_names[] = strdup(…)` — never freed §2.3 |
| `entity_leg.cpp` | | | | ● | | | | FIXME (cosTheta bounds) |
| `generichub.cpp` | ● | | | ● | | | | `sprintf`; dead power-link |
| `input_field.cpp` | ● | | | | | | | 6 unsafe string ops |
| `constructionyard.cpp` | ● | | | | | | | 3× `sprintf` into `char[64]` |
| 30+ entity files | | ● | | ● | | | ● | Commented code; raw ptrs; repeated advance/render |

### Starstrike

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `3d_sierpinski_gasket.cpp` | | ● | | ● | | | | `new T[]`; 2× `DELETEME` includes §4.5 |
| `attract.cpp` | ● | | | | | | | `strcpy` |
| `camera.cpp` | | | | ● | | | | Many commented blocks §4.1; 3× `fixMeUp` vars §4.3; `JAMES_TODO` |
| `entity_grid.cpp` | ● | ● | | | ● | | | `strcat`; raw arrays; `NULL`; manual realloc |
| `explosion.cpp` | | | | | | | | ✅ Migrated to `std::vector`; Rule-of-Five applied |
| `GameApp.cpp` | ● | ● | | ● | | | | `sprintf`; dead platform branches §4.2; `sprintf` overlapping UB §1.6; many raw `new` |
| `gamecursor.cpp` | ● | | | | | | | `sprintf`; `NULL`; `strdup` (paired OK) |
| `global_world.cpp` | ● | ● | | ● | | | | `JAMES_FIX` §4.2; raw ptrs; `strdup`+`strupr` |
| `landscape.cpp` | | ● | | | | | | 10× `NULL` usage §0.2; raw `new`/`delete` |
| `level_file.cpp` | | ● | | | | | | `strdup` for `m_mountName` (freed in header dtor — OK) |
| `location.cpp` | | | | | ● | | | Slow recalc §6.2 (2 TODOs) |
| `main.h` | | | | ● | | | | Dead `TARGET_OS_VISTA` extern block §4.2 |
| `particle_system.h` | | | ● | | | | | `RGBAColour col = NULL` (type error) §0.2 |
| `startsequence.cpp` | ● | ● | | | | | | **Format string injection** §-1.2; `strdup` never freed §2.3 |
| `water.cpp` | | ● | | ● | | | | JAK HACK §4.4; 7× raw `new`/`delete` |
| `team.cpp` | | | ● | | | | | `memset(this, 0, sizeof(*this))` — UB §2.4 |

---

*End of plan.  Each phase should be committed separately with a descriptive
message listing exactly what changed and the supporting evidence it was safe
to remove or modify.*
