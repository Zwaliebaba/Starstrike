# Starstrike — Codebase Improvement Plan

> Generated from a full static analysis of every project in the solution.
> Findings are grouped by severity, then by project.  Work items within each
> phase are ordered so that earlier items never depend on later ones.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Phase 0 — Quick Wins (< 1 day each)](#phase-0--quick-wins--1-day-each)
- [Phase 1 — Security & Stability](#phase-1--security--stability)
- [Phase 2 — Memory Safety](#phase-2--memory-safety)
- [Phase 3 — Type Safety & Modern Idioms](#phase-3--type-safety--modern-idioms)
- [Phase 4 — Dead Code & Preprocessor Hygiene](#phase-4--dead-code--preprocessor-hygiene)
- [Phase 5 — Code Duplication](#phase-5--code-duplication)
- [Phase 6 — Performance](#phase-6--performance)
- [Phase 7 — Header & Build Hygiene](#phase-7--header--build-hygiene)
- [Appendix A — Full TODO/FIXME/HACK Inventory](#appendix-a--full-todofixmehack-inventory)
- [Appendix B — Per-File Issue Matrix](#appendix-b--per-file-issue-matrix)

---

## Architecture Overview

```
Starstrike (Main App — MSIX, WinUI 3, DirectX 12)
 ├─► NeuronClient  (rendering, input, UI, audio — static lib)
 │    ├─► NeuronCore   (core utilities, math, networking — static lib)
 │    └─► GameLogic    (entities, buildings, AI, menus — static lib)
 ├─► NeuronCore
 └─► GameLogic

NeuronServer (network server — static lib)
 └─► NeuronCore
```

| Setting              | Value                                  |
|----------------------|----------------------------------------|
| Platform             | x64 only                               |
| Configurations       | Debug / Release                        |
| Toolset              | MSVC v145 (v143 fallback)              |
| C++ Standard         | `stdcpplatest` (Debug), `stdcpp20` (Release) |
| Windows SDK          | 10.0.26100.7705                        |
| Minimum OS           | Windows 10 (10.0.17763.0)             |
| Warning Level        | W4 (Starstrike), W3 (libraries)        |
| Conformance Mode     | Mixed (true for NeuronCore/Release, false elsewhere in Debug) |
| Header Guard Style   | 15 % `#pragma once`, 85 % legacy `#ifndef` |

---

## Phase 0 — Quick Wins (< 1 day each)

Small, safe fixes that improve correctness or silence noise immediately.

### 0.1  Remove redundant `volatile` on atomics

**Project:** NeuronCore &nbsp;|&nbsp; **Risk:** SAFE

`volatile std::atomic_bool` is redundant — `std::atomic` already provides the
necessary memory ordering.  `volatile` adds nothing and misleads readers.

| File | Lines |
|------|-------|
| `NeuronCore/ASyncLoader.h` | 30, 31, 59, 60 |

**Action:** Delete the `volatile` qualifier on all four declarations.

---

### 0.2  Fix unused `errorText` in `Fatal()`

**Project:** NeuronCore &nbsp;|&nbsp; **Risk:** SAFE

`Debug.h` builds the formatted error message (`errorText`) but never
passes it to `__debugbreak()` or the exception.

| File | Lines |
|------|-------|
| `NeuronCore/Debug.h` | ~36, ~50 |

**Action:** Include `errorText` in the exception message and/or output it
via `OutputDebugStringW` before `__debugbreak()`.

---

### 0.3  `NULL` → `nullptr`

**Project:** all &nbsp;|&nbsp; **Risk:** SAFE

30+ sites still use the `NULL` macro.  Key locations:

| File | Lines (examples) |
|------|------------------|
| `NeuronClient/2d_array.h`          | 50 |
| `NeuronCore/net_socket_listener.cpp` | 43 |
| `Starstrike/landscape.cpp`          | 39-77 |
| `Starstrike/particle_system.h`      | 92 (`RGBAColour col = NULL` — type error) |

**Action:** Replace with `nullptr`.  Fix `particle_system.h` to use the
default constructor instead.

---

## Phase 1 — Security & Stability

### 1.1  Eliminate unsafe C string functions

**Severity:** 🔴 CRITICAL &nbsp;|&nbsp; **Count:** 75+ call sites

`sprintf`, `strcpy`, `strcat` with no bounds checking are buffer-overflow
vectors.

| Project / File | Lines (sample) | Pattern |
|----------------|----------------|---------|
| `GameLogic/input_field.cpp` | 111, 140, 158, 172, 177, 187, 192, 209 | `strcpy`/`strcat`/`sprintf` into `m_buf[256]` |
| `GameLogic/darwinia_window.cpp` | 317, 325, 339, 343 | `sprintf` into `char[64]` |
| `GameLogic/drop_down_menu.cpp` | 37, 253 | `strcpy`/`sprintf` |
| `GameLogic/generichub.cpp` | 21, 70 | `strcpy` into `m_shapeName` |
| `GameLogic/constructionyard.cpp` | 35 | `sprintf` |
| `Starstrike/attract.cpp` | 12, 36, 64 | `strcpy` |
| `Starstrike/entity_grid.cpp` | 87, 92, 93, 95 | `strcat`/`sprintf` |
| `Starstrike/global_world.cpp` | 37-39, 193-204, 1537-1540 | `sprintf` |
| `Starstrike/GameApp.cpp` | 104, 183, 185, 196, 200, 214 | `sprintf`/`strcpy` |
| `Starstrike/gamecursor.cpp` | 72, 81, 958, 968 | `sprintf` |
| `NeuronCore/net_socket.cpp` | 132 | raw `memcpy(m_hostname, …)` — overflow risk |

**Action:**
- In **NeuronCore** (modern layer): use `std::format` / `std::string`.
- In **legacy projects** (GameLogic, NeuronClient, Starstrike): replace with
  `snprintf` / `strncpy` as a first pass; convert to `std::string` where the
  function boundary allows.

---

### 1.3  ~~Fix potential logic error in `net_socket_listener.cpp`~~

**Severity:** 🟢 FALSE POSITIVE &nbsp;|&nbsp; **Project:** NeuronCore

**Verified:** The `(!NetIsAddrInUse)` conditional is correct.
`NetIsAddrInUse` expands to `(WSAGetLastError() == WSAEADDRINUSE)`.
The logic retries bind only when the error is "address in use" and fails
immediately for any other error (e.g. permission denied).  No change needed.

---

### 1.4  ~~Fix memory leak in `net_socket_listener.cpp`~~ ✅ DONE

**Severity:** 🔴 HIGH &nbsp;|&nbsp; **Project:** NeuronCore

**Fixed:** Two changes applied:
1. Added `datasize <= 0` guard after `recvfrom()` — previously a failed
   `recvfrom` (returning -1) would pass a negative length to `NetUdpPacket`,
   causing undefined behaviour in `memcpy`.
2. Wrapped the allocation in `std::make_unique` with `.release()` into the
   callback, so the packet cannot leak between allocation and callback
   invocation.

---

### 1.5  Fix unsafe `void*` dispatching in `darwinia_window.cpp`

**Severity:** 🔴 HIGH &nbsp;|&nbsp; **Project:** GameLogic

`CreateValueControl()` casts a `void* value` to `unsigned char*`, `int*`,
`float*`, `char*` based on a runtime `dataType` parameter with no compile-
time safety.

| File | Lines |
|------|-------|
| `GameLogic/darwinia_window.cpp` | 300-309 |

**Action:** Replace with a type-safe variant (`std::variant` or tagged
union) or template the function.

---

## Phase 2 — Memory Safety

### 2.1  Convert raw `new`/`delete` to RAII containers

**Severity:** 🟠 HIGH &nbsp;|&nbsp; **Count:** 100+ allocation sites

The codebase relies on manual `new`/`delete` across every project.  Missed
`delete` paths cause leaks; double-`delete` paths cause crashes.

**Key areas:**

| Project / File | Lines (sample) | Pattern |
|----------------|----------------|---------|
| `Starstrike/3d_sierpinski_gasket.cpp` | 16, 65 | `new T[]` / `delete[] m_points` |
| `Starstrike/entity_grid.cpp` | 70-80, 137-138, 245, 262 | Dynamic arrays |
| `Starstrike/explosion.cpp` | 143, 147, 303 | Geometry buffers |
| `Starstrike/water.cpp` | 65, 79-80, 204, 227, 343-344 | Multiple heap arrays |
| `Starstrike/global_world.cpp` | 77-89, 348, 740, 1252-1254 | Global structures |
| `NeuronClient/bitmap.cpp` | 366-378 | `m_pixels = new RGBAColour[…]` |
| `NeuronClient/clienttoserver.cpp` | 34, 46, 56-64, 309-425 | Network letters |
| `NeuronClient/2d_array.h` | 61, 80 | Template container |
| `NeuronClient/bounded_array.h` | 56, 65, 87 | Template container |
| `NeuronClient/hash_table.h` | 135-137, 194-195 | Hash map backing arrays |
| `NeuronClient/eclbutton.h` | 21-22 | `char *m_caption`, `char *m_tooltip` |
| `GameLogic/building.cpp` | 627-795 | 60+ factory `new` calls |
| `GameLogic/engineer.cpp` | 360 | `delete position` |
| `GameLogic/entity.cpp` | 71 | `delete theFile` |
| `GameLogic/spam.cpp` | 476 | `delete target` |
| `GameLogic/spawnpoint.cpp` | 244 | `delete availableLinks` |

**Action:**
1. Template containers (`2d_array.h`, `bounded_array.h`, `hash_table.h`):
   replace internal `new T[]` with `std::vector<T>`.
2. Owned pointers in class members: convert to `std::unique_ptr<T>` or
   `std::unique_ptr<T[]>`.
3. Factory allocations (`Building::CreateBuilding`): return
   `std::unique_ptr<Building>`.
4. Network packet allocation: pass `std::unique_ptr` through callback chain.

---

### 2.2  Add move semantics to resource-owning classes

**Severity:** 🟡 MEDIUM

Classes that own heap resources lack move constructors / assignment:

| Class | File |
|-------|------|
| `BitmapRGBA` | `NeuronClient/bitmap.h` |
| `EclButton` | `NeuronClient/eclbutton.h` |
| `EclWindow` | `NeuronClient/eclwindow.h` |

**Action:** Implement Rule-of-Five or convert to `std::vector` / smart-
pointer members to get implicit moves.

---

## Phase 3 — Type Safety & Modern Idioms

### 3.1  Convert old-style enums to `enum class`

**Severity:** 🟠 HIGH &nbsp;|&nbsp; **Count:** 9 enums

| File | Enum | Current |
|------|------|---------|
| `GameLogic/entity.h` | Entity types | Anonymous `enum { TypeInvalid, … }` |
| `GameLogic/building.h` | Building types (60+ values) | Anonymous `enum { TypeInvalid, … }` |
| `NeuronClient/control_types.h` | `ControlType` | Plain `enum` |
| `NeuronClient/inputdriver.h` | `InputCondition` | Plain `enum` |
| `NeuronClient/inputdriver.h` | `InputParserState` | Plain `enum` |
| `NeuronClient/input_types.h` | `InputType` | Plain `enum` |
| `NeuronClient/input_types.h` | `InputMode` | Plain `enum` |
| `NeuronClient/networkupdate.h` | Network update types | Plain `enum` |
| `NeuronCore/net_lib.h` | `NetRetCode` | Plain `enum` |

**Action:** Convert each to `enum class` with an explicit underlying type.
Update all switch/comparison sites.

---

### 3.2  Add `[[nodiscard]]`, `noexcept`, `constexpr` where appropriate

**Severity:** 🟡 MEDIUM

| Idiom | Where to Add |
|-------|-------------|
| `[[nodiscard]]` | Getter functions returning allocated memory, `Hash.h` `HashState()`, network socket operations |
| `noexcept` | All `TimerCore.h` static getters, trivial accessors across NeuronCore |
| `constexpr` | Math constants, magic numbers used as array sizes |

**Action:** Audit NeuronCore first (modern C++ project), then extend to
Starstrike headers.  Do **not** retrofit into GameLogic/NeuronClient legacy
code unless editing those functions for other reasons.

---

### 3.3  Replace `const char*` with `std::string_view` in NeuronCore APIs

**Severity:** 🟡 MEDIUM

NeuronCore is the modern layer but still exposes some `const char*`
parameters (e.g. `FileSys.h`, `net_socket.h`).

**Action:** Where the callee does not need to own the string, change to
`std::string_view`.

---

### 3.4  Modernise NeuronCore networking layer

**Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Files:** `net_*.h/cpp` (8 files)

| Legacy Pattern | Modern Replacement |
|----------------|--------------------|
| `CRITICAL_SECTION` (`net_mutex.h`) | `std::mutex` / `std::lock_guard` |
| `CreateThread()` (`net_thread_win32.cpp`) | `std::thread` / `std::jthread` |
| Function-pointer callbacks (`net_lib_win32.h`) | `std::function<>` or `std::move_only_function` |
| `#ifndef` header guards | `#pragma once` |
| `memset()` on structs in constructors | Aggregate / in-class init |
| `MIN()` macro | `std::min()` |
| C-style casts | `static_cast<>`/`reinterpret_cast<>` |
| Hardcoded magic numbers (`100`, `10000`) | Named constants |

**Action:** Modernise one file at a time, build after each, starting with
`net_mutex` → `net_thread` → `net_socket` → `net_socket_listener`.

---

### 3.5  Template DataWriter boilerplate

**Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **File:** `NeuronCore/DataWriter.h`

7+ identical `WriteInt16`/`WriteUInt16`/`WriteInt32`/… methods differ only
in the type parameter.

**Action:** Replace with a single `Write<T>(const T& value)` template.

---

## Phase 4 — Dead Code & Preprocessor Hygiene

### 4.1  Remove commented-out code blocks

**Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 30+ blocks

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
| `GameLogic/armour.cpp` | 475-477, 416-487 | Debug render / target prediction |
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
| `Starstrike/camera.cpp` | 70, 72, 277, 667, 1004, 1010, 1333-1336, 1449-1452, 1517-1523 | Camera input |

**Action:** Delete all.  Commit with a message listing what was removed.

---

### 4.2  Remove stale preprocessor branches

**Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **Count:** 6 defines

| Define | Files | Status |
|--------|-------|--------|
| `DEMO2` | `GameLogic/ai.cpp`, `GameLogic/mainmenus.cpp` | Dead demo build |
| `JAMES_FIX` | `Starstrike/global_world.cpp`, `GameLogic/gunturret.cpp` | Developer-specific |
| `DEBUG_RENDER_ENABLED` | `GameLogic/airstrike.cpp`, `anthill.cpp`, `armour.cpp` + more | Dead debug rendering |
| `TARGET_OS_LINUX` | `Starstrike/GameApp.cpp` | Unsupported platform |
| `TARGET_OS_MACOSX` | `Starstrike/GameApp.cpp` | Unsupported platform |
| `TARGET_VISTA_DEMO2` | `Starstrike/GameApp.cpp` | Dead build variant |

**Action:** Verify each define is never set in any active configuration
(check `.vcxproj` files and build scripts).  Collapse each branch, keeping
the active path.

---

### 4.3  Rename/document `fixMeUp` static variables

**Severity:** 🟡 MEDIUM &nbsp;|&nbsp; **File:** `Starstrike/camera.cpp`

Three statics named `fixMeUp` (lines 282, 318, 373) indicate incomplete
work.

**Action:** Rename to descriptive names (`s_cameraDefaultFov`,
`s_cameraInitialised`, etc.) and document intent.

---

### 4.4  Remove `JAK HACK` in `water.cpp`

**Severity:** 🟢 LOW &nbsp;|&nbsp; **File:** `Starstrike/water.cpp` line 452

Disabled hack comment — if the hack is no longer active, remove the dead
code block entirely.

---

## Phase 5 — Code Duplication

### 5.1  Consolidate `DataWriter` write methods

(See Phase 3.5 above — listed here for cross-reference.)

---

### 5.3  Extract repeated OpenGL quad-rendering helpers

**Projects:** GameLogic, Starstrike

`glBegin(GL_QUADS) / glVertex3fv / glEnd` sequences appear 20+ times
across `darwinia_window.cpp`, `blueprintstore.cpp`, `water.cpp`,
`clouds.cpp` and others.

**Action:** Extract into a shared `RenderQuad()` / `RenderTexturedQuad()`
utility, or migrate the draw calls to `SpriteBatch` where appropriate.

---

### 5.4  Refactor `Building::CreateBuilding` factory

**File:** `GameLogic/building.cpp` lines 627-795

169 lines of `if (_type == TypeX) building = new X();` chains.

**Action:** Replace with a static registration map:

```cpp
using BuildingFactory = std::unique_ptr<Building>(*)();
static const std::unordered_map<int, BuildingFactory> s_factories = { … };
```

---

### 5.5  Consolidate entity `Advance()`/`Render()` boilerplate

**Projects:** GameLogic (30+ entity files)

Many entity types repeat identical health-check, movement, and render
preamble code.

**Action:** Factor common logic into `Entity::AdvanceBase()` /
`Entity::RenderBase()` called by each override.  Alternatively, investigate
a component/composition approach for shared behaviours.

---

## Phase 6 — Performance

### 6.1  Replace busy-wait in `ASyncLoader`

**File:** `NeuronCore/ASyncLoader.h` lines 9-10

```cpp
while (m_isLoading) std::this_thread::yield();  // Spins CPU
```

**Action:** Replace with a `std::condition_variable` or `std::binary_semaphore`
(C++20) to sleep the thread until the load completes.

---

### 6.2  Fix slow area-recalculation in `location.cpp`

**File:** `Starstrike/location.cpp` lines 606, 641

Existing TODO: *"This is WAY too slow, should only recalculate the areas
affected."*

**Action:** Implement dirty-region tracking so only modified cells are
recalculated.

---

### 6.3  Pre-allocate entity grid arrays

**File:** `Starstrike/entity_grid.cpp`

Dynamic array resizing without `.reserve()` or pre-allocation hints.

**Action:** Profile and add `reserve()` calls based on expected entity
counts.

---

## Phase 7 — Header & Build Hygiene

### 7.1  Convert `#ifndef` guards to `#pragma once`

**Count:** 212+ headers (85 % of codebase)

All projects except NeuronServer have significant non-compliance.  `#pragma
once` is supported by MSVC, GCC, and Clang and avoids typo-induced ODR
violations.

| Project | `#pragma once` | `#ifndef` | Compliance |
|---------|----------------|-----------|------------|
| NeuronCore    | 13 | 8  | 62 % |
| NeuronServer  | 2  | 0  | 100 % |
| NeuronClient  | 24 | 80+| 19 % |
| GameLogic     | 1  | 81+| 1 % |
| Starstrike    | 1  | 40+| 2 % |

**Action:** Automate with a script or regex find-and-replace.  Build all
configurations after the batch to verify no breakage.

---

### 7.2  Raise warning level on library projects

Currently GameLogic, NeuronClient, NeuronCore, and NeuronServer compile at
`/W3`.  Starstrike compiles at `/W4`.

**Action:** Raise all libraries to `/W4` and address new warnings.
Consider enabling `/we4505` (unreferenced local function) and `/we4189`
(unreferenced local variable) to catch dead code automatically.

---

### 7.3  Enable Conformance Mode consistently

GameLogic and NeuronClient disable conformance mode
(`<ConformanceMode>false</ConformanceMode>`) in Debug.

**Action:** Enable `ConformanceMode=true` for all projects in all
configurations.  Fix any resulting compilation errors — these indicate
non-conformant code that may break with future compiler updates.

---

### 7.4  Audit legacy `#pragma warning(disable:…)` in `NeuronCore.h`

Warnings 4201, 4238, 4244, 4324 are globally suppressed.  Some may no
longer fire with the current code.

**Action:** Re-enable one at a time, build, fix or locally suppress where
truly needed, and remove the global suppression.

---

## Appendix A — Full TODO/FIXME/HACK Inventory

| # | File | Line | Text |
|---|------|------|------|
| 1 | `Starstrike/location.cpp` | 606 | TODO: This is WAY too slow, should only recalculate the areas affected |
| 2 | `Starstrike/location.cpp` | 641 | TODO: This is WAY too slow, should only recalculate the areas affected |
| 3 | `Starstrike/location.cpp` | 1344 | TODO: This is sent by the server. ACK!!! |
| 4 | `Starstrike/location.cpp` | 1363 | TODO: When do we want to create a marker? |
| 5 | `Starstrike/camera.cpp` | 71 | TODO: Support mouse/joystick |
| 6 | `Starstrike/camera.cpp` | 85 | TODO: Really? |
| 7 | `Starstrike/camera.cpp` | 566 | TODO: Support mouse/joystick |
| 8 | `Starstrike/location_input.cpp` | 245 | TODO: Really? |
| 9 | `Starstrike/GameApp.cpp` | 125 | (todo) or is running in media center and tenFootMode == -1 |
| 10 | `Starstrike/water.cpp` | 452 | JAK HACK (DISABLED) |
| 12 | `GameLogic/entity_leg.cpp` | 104 | FIXME — cosTheta should never be greater than one, yet sometimes it is |
| 13 | `GameLogic/prefs_other_window.cpp` | 69 | (todo) or is running in media center… |
| 14 | `GameLogic/input_field.cpp` | 15 | bit of a hack |
| 15 | `NeuronClient/inputdriver_alias.cpp` | 72 | TODO: This isn't quite right |
| 16 | `NeuronClient/inputdriver_idle.cpp` | 136 | TODO: This |
| 17 | `NeuronClient/keydefs.h` | 104 | TODO: Any key not implemented yet |
| 18 | `NeuronClient/opengl_directx.cpp` | 961 | TODO |
| 19 | `NeuronClient/opengl_directx.cpp` | 1015 | TODO |
| 20 | `NeuronClient/opengl_directx.cpp` | 2073 | TODO: implement via constant buffer clip planes in shader |
| 21 | `NeuronClient/d3dx12.h` | 5063 | TODO: SDK Version check should include new subobject types |
| 22 | `NeuronClient/GraphicsCommon.h` | 38 | XXX |
| 23 | `NeuronClient/soundsystem.cpp` | 13 | FIXME |
| 24 | `NeuronClient/sound_parameter.cpp` | 142 | TODO: version compatibility check |

---

## Appendix B — Per-File Issue Matrix

Legend: **S** = Security, **M** = Memory, **T** = Type Safety, **D** = Dead Code,
**P** = Performance, **H** = Header Hygiene, **DUP** = Duplication

### NeuronCore

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `ASyncLoader.h` | | | | | ● | | ● | busy-wait; `volatile` on atomics; duplicated `Static` variant |
| `DataReader.h` | | | ● | | | | | Raw ptr param → use `std::span` |
| `DataWriter.h` | | | | | | | ● | 7+ identical write methods |
| `Debug.h` | | ● | | | | | | `errorText` unused in `Fatal()` |
| `FileSys.h` | | | | | | | | Legacy A/W suffix |
| `Hash.h` | | | ● | | | | | C-style pointer casts |
| `MathCommon.h` | | | | | ● | | | `_BitScan*` intrinsics → `std::bit_width` |
| `NeuronCore.h` | | | | | | ● | | Global warning suppressions |
| `TimerCore.h` | | | ● | | | | | Missing `noexcept` on getters |
| `net_lib.h` | | | ● | | | ● | | Plain enum; `#ifndef` guard; `MIN` macro |
| `net_lib_win32.h` | | | ● | | | ● | | Function pointers → `std::function` |
| `net_mutex*.h/cpp` | | | ● | | | ● | | `CRITICAL_SECTION` → `std::mutex` |
| `net_socket*.h/cpp` | ● | ● | ● | ● | | ● | | C casts, magic numbers, dead code, `memcpy` overflow |
| `net_thread_win32.cpp` | | | ● | | | | | `CreateThread` → `std::thread` |
| `net_udp_packet.h/cpp` | | | | | | ● | | Fixed `char[]` buffer; `MIN` macro |

### NeuronClient

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `2d_array.h` | | ● | | | | ● | | `new T[]`; `NULL`; `#ifndef` |
| `bitmap.cpp` | | ● | | | ● | | | `new RGBAColour[]`; temp allocs in filters |
| `bounded_array.h` | | ● | | | | ● | | `new T[]` |
| `clienttoserver.cpp` | ● | ● | | ● | | | | `strcpy`; raw `new`; dead gethostbyname |
| `control_bindings.cpp` | | | | ● | | | | Disabled unique_ptr logic |
| `control_types.h` | | | ● | | | | | Plain enum |
| `eclbutton.h/cpp` | | ● | | | | | | `char*` m_caption/m_tooltip |
| `eclipse.cpp` | ● | | | | | | | 50+ `strcpy` |
| `hash_table.h` | | ● | | ● | | | | Raw arrays; dead alt hash |
| `inputdriver.h` | | | ● | | | | | 2 plain enums |
| `input_types.h` | | | ● | | | | | 2 plain enums |
| `networkupdate.h` | | | ● | | | | | Plain enum |
| `opengl_directx.cpp` | | | | ● | | | | 3 TODO stubs |
| `sound_filter.cpp` | | | | ● | | | | Disabled audio blocks |
| `CompiledShaders/*.h` | | | | ● | | | | `#if 0` around debug info |

### GameLogic

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `building.h` | | | ● | | | ● | | Anonymous enum (60+ values) |
| `building.cpp` | | ● | | | | | ● | 169-line factory; 60+ `new` |
| `darwinia_window.cpp` | ● | | ● | | | | ● | `void*` casts; `sprintf`; repeated GL |
| `drop_down_menu.cpp` | ● | | | ● | | | | `strcpy`/`sprintf`; dead code |
| `entity.h` | | | ● | | | ● | | Anonymous enum |
| `entity_leg.cpp` | | | | ● | | | | FIXME (cosTheta bounds) |
| `generichub.cpp` | ● | | | ● | | | | `strcpy`; dead power-link |
| `input_field.cpp` | ● | | | | | | | 8 unsafe string ops |
| `constructionyard.cpp` | ● | | | | | | | `sprintf` |
| 30+ entity files | | ● | | ● | | ● | ● | Commented code; raw ptrs; repeated advance/render |

### Starstrike

| File | S | M | T | D | P | H | DUP | Notes |
|------|---|---|---|---|---|---|-----|-------|
| `3d_sierpinski_gasket.cpp` | | ● | | | | | | `new T[]` |
| `attract.cpp` | ● | | | | | | | `strcpy` |
| `camera.cpp` | | | | ● | | | | Many commented blocks; `fixMeUp` vars |
| `entity_grid.cpp` | ● | ● | | | ● | | | `strcat`; raw arrays; no reserve |
| `explosion.cpp` | | ● | | | | | | Dynamic geometry |
| `GameApp.cpp` | ● | | | ● | | | | `sprintf`; dead platform branches |
| `gamecursor.cpp` | ● | | | | | | | `sprintf` |
| `global_world.cpp` | ● | ● | | ● | | | | Crash workaround; `JAMES_FIX`; raw ptrs |
| `landscape.cpp` | | | | | | | | `NULL` usage |
| `location.cpp` | | | | | ● | | | Slow recalc (2 TODOs) |
| `particle_system.h` | | | ● | | | | | `RGBAColour col = NULL` (type error) |
| `water.cpp` | | ● | | ● | | | | JAK HACK; raw arrays |

---

*End of plan.  Each phase should be committed separately with a descriptive
message listing exactly what changed and the supporting evidence it was safe
to remove or modify.*
