# String Modernization Plan

Migrate the Starstrike codebase from C-style string handling (`snprintf`, `strncpy`, `strlen`, `char[]` buffers) to modern C++ (`std::format`, `std::string`, `std::string_view`).

## 📊 Current State

| Metric | Count |
|--------|-------|
| Unsafe C string function calls | 349 |
| `const char*` function parameters in headers | 186 |
| `char m_field[N]` member declarations (struct/class) | 26 |
| `char buf[N]` stack buffer declarations | 119 |
| `NewStr()` heap allocations (raw `new char[]`) | 10 |
| Unique files with unsafe string calls | 73 |
| Existing `std::string` usage | 51 |
| Existing `std::string_view` usage | 66 |
| Existing `std::format` usage | 4 |
| `_CRT_SECURE_NO_WARNINGS` | Global in `NeuronCore.h` |

### Call-Site Inventory by Function

| Function | Total | GameLogic | GameRender | NeuronClient | NeuronCore | Starstrike |
|----------|-------|-----------|------------|--------------|------------|------------|
| `snprintf` | 148 | 32 | 30 | 32 | 0 | 54 |
| `strncpy` | 106 | 9 | 9 | 54 | 0 | 34 |
| `strlen` | 70 | 2 | 11 | 46 | 1 | 11 |
| `vsprintf` | 9 | 0 | 0 | 9 | 0 | 0 |
| `strncat` | 8 | 0 | 2 | 3 | 0 | 3 |
| `strcpy` | 7 | 0 | 5 | 0 | 0 | 2 |
| `sscanf` | 1 | 0 | 0 | 1 | 0 | 0 |
| **Total** | **349** | **43** | **57** | **145** | **1** | **104** |

### Call-Site Inventory by File

#### GameLogic (43 calls, 21 files)

| File | Calls | Functions |
|------|-------|-----------|
| `scripttrigger.cpp` | 8 | `snprintf`, `strncpy` |
| `rocket.cpp` | 3 | `snprintf` |
| `switch.cpp` | 3 | `snprintf`, `strncpy` |
| `constructionyard.cpp` | 3 | `snprintf` |
| `generator.cpp` | 3 | `snprintf` |
| `generichub.cpp` | 3 | `snprintf`, `strncpy` |
| `controltower.cpp` | 2 | `snprintf` |
| `staticshape.cpp` | 2 | `snprintf`, `strncpy` |
| `gunturret.cpp` | 2 | `snprintf` |
| `spiritreceiver.cpp` | 2 | `snprintf` |
| `spawnpoint.cpp` | 2 | `snprintf` |
| Others (10 files) | 1 each | Various |

#### GameRender (57 calls, 14 files)

| File | Calls | Functions |
|------|-------|-----------|
| `input_field.cpp` | 20 | `snprintf`, `strlen`, `strncpy` |
| `scrollbar.cpp` | 9 | `snprintf` |
| `darwinia_window.cpp` | 6 | `snprintf`, `strncpy` |
| `prefs_other_window.cpp` | 5 | `snprintf`, `strncpy` |
| `TrunkPortBuildingRenderer.cpp` | 3 | `snprintf` |
| `drop_down_menu.cpp` | 2 | `snprintf`, `strncpy` |
| Others (8 files) | 1–2 each | Various |

#### NeuronClient (145 calls, 23 files)

| File | Calls | Functions |
|------|-------|-----------|
| `eclipse.cpp` | 21 | `strlen`, `strncpy` |
| `text_renderer.cpp` | 18 | `strlen`, `vsprintf` |
| `language_table.cpp` | 17 | `strlen`, `strncpy`, `snprintf` |
| `preferences.cpp` | 11 | `strlen`, `strncpy`, `snprintf` |
| `soundsystem.cpp` | 9 | `strncpy`, `snprintf` |
| `sound_instance.cpp` | 9 | `strncpy`, `snprintf` |
| `resource.cpp` | 8 | `strlen`, `strncpy` |
| `text_stream_readers.cpp` | 7 | `strlen`, `strncpy` |
| `eclwindow.cpp` | 7 | `strlen`, `strncpy` |
| `btree.h` | 5 | `strncpy` |
| `eclbutton.cpp` | 5 | `strlen`, `strncpy` |
| `filesys_utils.cpp` | 5 | `strlen` |
| `server.cpp` | 5 | `snprintf`, `strlen` |
| Others (10 files) | 1–4 each | Various |

#### Starstrike (104 calls, 15 files)

| File | Calls | Functions |
|------|-------|-----------|
| `taskmanager_interface_icons.cpp` | 24 | `snprintf`, `strncpy`, `strlen` |
| `GameApp.cpp` | 16 | `snprintf`, `strncpy` |
| `level_file.cpp` | 16 | `snprintf`, `strncpy` |
| `sepulveda_strings.cpp` | 9 | `snprintf` |
| `script.cpp` | 6 | `snprintf`, `strlen` |
| `entity_grid.cpp` | 4 | `snprintf` |
| `global_world.cpp` | 4 | `snprintf` |
| `taskmanager_interface.cpp` | 4 | `snprintf`, `strncpy` |
| `water.cpp` | 4 | `snprintf` |
| Others (6 files) | 2–3 each | Various |

### `snprintf` Format Specifier Distribution

| Specifier | Occurrences | Notes |
|-----------|-------------|-------|
| `%s` | 79 | String interpolation — direct `std::format("{}", str)` replacement |
| `%d` | 49 | Integer formatting — direct `std::format("{}", n)` replacement |
| `%f` / `%.Nf` | 9 | Float formatting — `std::format("{:.Nf}", f)` |
| `%x` | 0 | — |

### Stack Buffer Size Distribution

| Size | Count | Notes |
|------|-------|-------|
| `char[256]` | 62 | Most common — path buffers, name buffers |
| `char[64]` | 19 | Short labels, UI text |
| `char[512]` | 16 | Longer format strings |
| `char[128]` | 5 | — |
| `char[1024]` | 4 | Large text blocks |
| Other | 13 | Various sizes (16, 32, 10240, etc.) |

### Member `char[]` Fields (26 declarations)

These are the hardest to migrate — they are part of struct layouts, may be serialized, and are copied with `strncpy`.

| Header | Field | Size | Alloc | Used By |
|--------|-------|------|-------|---------|
| `GameContext.h` | `m_userProfileName`, `m_requestedMission`, `m_requestedMap`, `m_gameDataFile` | 256 each | Fixed | Starstrike (app context) |
| `sound_instance.h` | `m_soundName` | 256 | Fixed | NeuronClient (sound system) |
| `soundsystem.h` | `m_name` (×3 different structs: `DspParameterBlueprint`, `DspBlueprint`, `SampleGroup`) | 256 each | Fixed | NeuronClient (sound blueprints) |
| `binary_stream_readers.h` | `m_filename` | 256 | Fixed | NeuronClient (file I/O) |
| `text_stream_readers.h` | `m_filename`, `m_seperatorChars` | 256, 16 | Fixed | NeuronClient (file I/O) |
| `input_field.h` | `m_buf` | 256 | Fixed | GameRender (UI) |
| `drop_down_menu.h` | `m_parentName` | 256 | Fixed | GameRender (UI) |
| `scripttrigger.h` | `m_scriptFilename` | 256 | Fixed | GameLogic |
| `generichub.h` | `m_shapeName` | 256 | Fixed | GameLogic |
| `staticshape.h` | `m_shapeName` | 256 | Fixed | GameLogic |
| `switch.h` | `m_script` | 256 | Fixed | GameLogic |
| `attract.h` | `m_userProfile` | 256 | Fixed | Starstrike |
| `gamecursor_2d.h` | `m_selectionArrowFilename`, `m_selectionArrowShadowFilename` | 256 each | Fixed | Starstrike |
| `taskmanager_interface.h` | `m_name`, `m_toolTip` | 256, 1024 | Fixed | Starstrike |
| `networkupdate.h` | `m_clientIp` | 16 | Fixed | NeuronClient (networking) |
| `servertoclient.h` | `m_ip` | 16 | Fixed | NeuronClient (networking) |
| `tree.h` | `m_leafColourArray`, `m_branchColourArray` | 4 each | Fixed | GameLogic (byte arrays, not strings) |

### Heap-Allocated `char*` Member Fields (5 declarations)

These are `char*` members allocated with `_strdup`/`malloc`/`NewStr` and freed with `free`/`delete[]`. Easier to migrate than `char[N]` (no struct layout concerns) but missing from the original inventory.

| Header | Field | Alloc | Free | Used By | Notes |
|--------|-------|-------|------|---------|-------|
| `text_renderer.h` | `m_filename` | `_strdup` | `free` | NeuronClient — font file path, used in `Initialise` and `BuildOpenGlState` | 5 references across 2 files |
| `sound_instance.h` | `m_eventName` | `malloc` (manual concat) / `_strdup` (Copy) | `free` | NeuronClient — compound "EntityName EventName" key, used in `_stricmp` comparisons, `BTree` lookups, and profiler tags | 22 references across 2 files |
| `soundsystem.h` | `m_eventName` (`SoundEventBlueprint`) | `NewStr` | `delete[]` | NeuronClient — event blueprint name, used in `_stricmp` matching during trigger dispatch | 16 references across 2 files |
| `eclbutton.h` | `m_caption`, `m_tooltip` | `NewStr` | `delete[]` | NeuronClient — Eclipse UI button text, set via `SetCaption`/`SetTooltip` | Widespread (Eclipse UI) |
| `eclwindow.h` | `m_name[256]`, `m_title[256]`, `m_currentTextEdit[256]` | Fixed | — | NeuronClient — Eclipse UI window identity (used as `HashTable`-style key in `EclGetWindow`) | Widespread |

> **Note:** `SoundEventBlueprint::m_eventName` uses `NewStr` (which calls `new char[]`) but `SetEventName` frees with `delete[]` — this is correct. However `SoundInstance::m_eventName` uses `_strdup`/`malloc` and frees with `free` — also correct internally. But the Copy path uses `_strdup` (correct). The manual concat path in `SetEventName` uses `malloc` + `memcpy` (correct). All paths are consistent within each class.

### Legacy Container Inventory

These legacy containers use raw `char*` keys internally and are a **blocking dependency** for Phases 4, 6, and 7 of the string migration.

#### `HashTable<T>` — Open-Addressing Hash Map with `char*` Keys

| Property | Detail |
|----------|--------|
| Header | `NeuronClient/hash_table.h` |
| Key type | `char*` — allocated with `_strdup` (`malloc`), freed with `free` (in `Empty`) or `delete[]` (in `RemoveData` — **allocator mismatch bug**) |
| Comparison | Case-insensitive (`_stricmp`) |
| Collision | Linear probing (open addressing) |
| Growth | Doubles when >50% full, max 65536 slots |
| Index API | `Size()` = total slots (not element count), `NumUsed()` = element count, `ValidIndex(i)`, `operator[](i)`, `GetName(i)` |
| Iteration pattern | `for (unsigned int i = 0; i < table.Size(); ++i) { if (table.ValidIndex(i)) { ... table[i] ... table.GetName(i) ... } }` |
| `T` constraint | `memset(m_data, 0, ...)` — assumes `T` is trivially zero-initializable |

**⚠️ Pre-existing bug:** `PutData` allocates keys with `_strdup` (→ `malloc`), but `RemoveData(unsigned int)` frees with `delete[]`. This is undefined behavior. The migration to `std::unordered_map` (which owns `std::string` keys) eliminates this bug.

**Declarations in the codebase (6 sites):**

| Header | Member | Type | Notes |
|--------|--------|------|-------|
| `resource.h` | `m_bitmaps` | `HashTable<BitmapRGBA*>` | Static member, bitmap registry |
| `resource.h` | `m_displayLists` | `HashTable<int>` | Static member, display list cache |
| `resource.h` | `m_textures` | `HashTable<int>` | Static member, texture ID cache |
| `resource.h` | `m_shapes` | `HashTable<ShapeStatic*>` | Static member, shape cache |
| `preferences.h` | `m_items` | `HashTable<PrefsItem*>` | Preferences key→value store |
| `sample_cache.h` | `m_cache` | `HashTable<CachedSample*>` | Sound sample cache |

**Pointer-to-HashTable declarations (3 sites):**

| Header | Member | Type | Notes |
|--------|--------|------|-------|
| `language_table.h` | `m_phrasesKbd` | `HashTable<int>*` | Keyboard-mode phrase index offsets |
| `language_table.h` | `m_phrasesXin` | `HashTable<int>*` | Controller-mode phrase index offsets |
| `language_table.h` | (parameter) | `HashTable<int>*` | `RebuildTable` and `GetCurrentTable` return type |

#### `SortingHashTable<T>` — Ordered Extension of `HashTable<T>`

| Property | Detail |
|----------|--------|
| Header | `NeuronClient/sorting_hash_table.h` |
| Inherits | `HashTable<T>` |
| Extra | Maintains `short* m_orderedIndices` chain for alphabetical iteration via `StartOrderedWalk` / `GetNextOrderedIndex` |
| Iteration pattern | `for (short i = table.StartOrderedWalk(); i != -1; i = table.GetNextOrderedIndex()) { ... }` |

**Declarations in the codebase:** None found as member variables — the class exists but no current members use it. If any are discovered during migration, they map to `std::map`.

#### `BTree<T>` — Unbalanced Binary Search Tree with `char*` Keys

| Property | Detail |
|----------|--------|
| Header | `NeuronClient/btree.h` |
| Key type | `char*` — allocated with `new char[]`, freed with `delete[]` |
| Comparison | Case-insensitive (`_stricmp`) |
| Balance | **Unbalanced** — worst case O(n) lookup |
| Node ownership | Each `BTree<T>` node owns its `ltree`/`rtree` children (recursive delete) |
| Extra API | `ConvertToDArray()`, `ConvertIndexToDArray()` — return heap-allocated `DArray` |

**Declarations in the codebase (2 sites):**

| Location | Variable | Type | Notes |
|----------|----------|------|-------|
| `language_table.h` | `m_phrasesRaw` | `BTree<LangPhrase*>` | Raw phrase tree — master lookup before mode-specific HashTables |
| `soundsystem.cpp:1070` | `existingInstances` | `BTree<float>` (local) | Temporary per-frame tree for priority reduction of duplicate sound events |

### Existing String Infrastructure

| File | Purpose | Notes |
|------|---------|-------|
| `NeuronClient/string_utils.h` | `NewStr()` (heap-allocate copy), `StrToLower()` | Legacy — `NewStr` should become `std::string` copy |
| `NeuronClient/Strings.h` | `Strings::Get()` — localization lookup | Already returns `std::wstring`/`std::string` |
| `NeuronCore` | Uses `std::string`, `std::string_view`, `std::format` | Modern baseline to follow |

---

## 🎯 Migration Strategy

### Guiding Principles

1. **`std::string` is the default storage type.** Replace `char buf[N]` locals with `std::string`. Replace `char m_field[N]` members with `std::string`.
2. **`std::string_view` for read-only parameters.** Replace `const char*` function parameters with `std::string_view` where the function does not store the string.
3. **`std::format` replaces `snprintf`.** Every `snprintf(buf, size, fmt, ...)` becomes `auto str = std::format(fmt, ...)`.
4. **Preserve `const char*` at API boundaries** where callers pass string literals or C-library interop requires null-terminated strings. Use `.c_str()` or `.data()` at the boundary.
5. **Member `char[]` fields are migrated last** — they may affect serialization, network protocols, or struct layout assumptions.
6. **Legacy code (NeuronClient, GameLogic) patterns are respected** per `copilot-instructions.md`: keep C-style strings when editing small sections of legacy files. Full-file migration happens in dedicated steps below.

### Format String Translation Reference

| C format | `std::format` equivalent | Example |
|----------|-------------------------|---------|
| `%s` | `{}` | `std::format("Hello {}", name)` |
| `%d` / `%i` | `{}` | `std::format("Count: {}", n)` |
| `%u` | `{}` | `std::format("Size: {}", sz)` |
| `%f` | `{}` | `std::format("Value: {}", f)` (default 6 digits) |
| `%.2f` | `{:.2f}` | `std::format("Price: {:.2f}", p)` |
| `%04d` | `{:04d}` | `std::format("ID: {:04d}", id)` |
| `%x` / `%X` | `{:x}` / `{:X}` | `std::format("Hex: {:x}", val)` |
| `%-20s` | `{:<20}` | `std::format("{:<20}", label)` |
| `%c` | `{:c}` | `std::format("{:c}", ch)` |

### `vsprintf` → `std::vformat` Translation

The 9 `vsprintf` calls in `text_renderer.cpp` and `file_writer.cpp` use variadic `va_list` parameters. These are the highest-risk calls (unbounded writes).

**Pattern:**
```cpp
// Before (unsafe):
void TextRenderer::Print(const char* _text, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, _text);
    vsprintf(buf, _text, ap);  // ← buffer overflow risk
    va_end(ap);
    // ... use buf ...
}

// After (safe):
void TextRenderer::Print(const char* _text, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, _text);
    vsnprintf(buf, sizeof(buf), _text, ap);  // ← bounded (Phase 1 quick fix)
    va_end(ap);
    // ... use buf ...
}
```

**Full modernization** (Phase 4+) would replace the variadic C function with a template or `std::format_string`, but that changes the function signature and all call sites. Defer to when `text_renderer` is fully migrated.

---

## 📋 Implementation Phases

### Phase 1: Critical Safety Fixes (Effort: Small, ~0.5 day)

Fix the most dangerous patterns without changing any APIs.

| Step | Action | Files | Risk |
|------|--------|-------|------|
| 1.1 | Replace all 9 `vsprintf` → `vsnprintf` (add buffer size parameter) | `NeuronClient/file_writer.cpp`, `NeuronClient/text_renderer.cpp` | Low — same output, just bounded |
| 1.2 | Replace all 7 `strcpy` → `strncpy` + null-terminate | 5 GameRender + 2 Starstrike files | Low — same output, just bounded |
| 1.3 | Audit all 106 `strncpy` call sites for missing null-termination (`buf[N-1] = '\0'`) | 35 files | Low — adds safety, no behavior change |

**Verification:** Build Debug x64 + Release x64. Run the game, verify text rendering and file I/O.

---

### Phase 2: Stack Buffer Elimination — GameLogic (Effort: Small, ~1 day)

GameLogic has the fewest calls (43) and is a standalone project with no rendering dependencies. Good pilot for establishing patterns.

| Step | Action | Files | Calls |
|------|--------|-------|-------|
| 2.1 | Replace `snprintf` → `std::format` in all GameLogic `.cpp` files | 21 files | 32 `snprintf` |
| 2.2 | Replace `strncpy` into local buffers → `std::string` assignment | Same files | 9 `strncpy` |
| 2.3 | Replace `strlen` → `.size()` / `.length()` | Same files | 2 `strlen` |

**Pattern (before/after):**
```cpp
// Before:
char buf[256];
snprintf(buf, sizeof(buf), "scripts/%s", m_scriptFilename);

// After:
auto buf = std::format("scripts/{}", m_scriptFilename);
```

**Verification:** Build Debug x64. Run the game, load levels that use scripted triggers, generators, spawn points, gun turrets.

---

### Phase 3: Stack Buffer Elimination — GameRender (Effort: Medium, ~1–2 days)

| Step | Action | Files | Calls |
|------|--------|-------|-------|
| 3.1 | Replace `snprintf` → `std::format` in GameRender `.cpp` files | 14 files | 30 `snprintf` |
| 3.2 | Replace `strncpy` / `strlen` in GameRender | Same files | 9 `strncpy`, 11 `strlen` |
| 3.3 | Replace `strncat` / `strcpy` in GameRender | Subset | 2 `strncat`, 5 `strcpy` |
| 3.4 | Replace `NewStr()` calls in GameRender with `std::string` | 3 files | 3 calls |

**⚠️ Coordination:** `input_field.cpp` (20 calls) has a `char m_buf[256]` member in `input_field.h`. This member is used for interactive text input (keystroke buffer). Migrating it to `std::string` requires careful handling of character insertion/deletion logic. Consider deferring `m_buf` to Phase 6 (member fields) and only migrating the local stack buffers in Phase 3.

**⚠️ DX12 coordination:** Check `DX12Migration.md` — some GameRender files may be scheduled for rewrite. Skip those files here.

**Verification:** Build Debug x64. Run the game, test UI: scrollbars, dropdown menus, preferences dialogs, text input fields.

---

### Phase 3.5: Legacy Container Migration (Effort: Large, ~3–5 days)

Replace `HashTable<T>`, `SortingHashTable<T>`, and `BTree<T>` with standard library containers. These legacy containers use raw `char*` keys internally and are a **blocking dependency** for Phases 4, 6, and 7 — any file that builds a `char buf[N]` with `snprintf` just to pass it as a `HashTable` key cannot be fully modernized until the container accepts `std::string`/`std::string_view`.

#### Replacement Mapping

| Legacy Container | Replacement | Rationale |
|-----------------|-------------|-----------|
| `HashTable<T>` | `std::unordered_map<std::string, T, CaseInsensitiveHash, CaseInsensitiveEqual>` | Same O(1) average lookup semantics. Case-insensitive to match `_stricmp` behavior. |
| `SortingHashTable<T>` | `std::map<std::string, T, CaseInsensitiveCompare>` | Provides ordered iteration (replaces `StartOrderedWalk`/`GetNextOrderedIndex`). Case-insensitive. |
| `BTree<T>` | `std::map<std::string, T, CaseInsensitiveCompare>` | Sorted + case-insensitive. Balanced (O(log n) guaranteed vs. O(n) worst-case for unbalanced `BTree`). |

#### Step 3.5.0: Case-Insensitive String Utilities

Create a utility header in NeuronClient (or NeuronCore if dependency allows) with case-insensitive comparators and hashers:

```cpp
// NeuronClient/CaseInsensitiveString.h
#pragma once

#include <string>
#include <string_view>
#include <algorithm>
#include <functional>

struct CaseInsensitiveHash
{
    size_t operator()(std::string_view sv) const noexcept
    {
        size_t h = 0;
        for (char c : sv)
            h = h * 31 + static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(c)));
        return h;
    }
};

struct CaseInsensitiveEqual
{
    bool operator()(std::string_view a, std::string_view b) const noexcept
    {
        return std::ranges::equal(a, b, [](char l, char r) {
            return std::tolower(static_cast<unsigned char>(l)) ==
                   std::tolower(static_cast<unsigned char>(r));
        });
    }
};

struct CaseInsensitiveCompare
{
    bool operator()(std::string_view a, std::string_view b) const noexcept
    {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](char l, char r) {
                return std::tolower(static_cast<unsigned char>(l)) <
                       std::tolower(static_cast<unsigned char>(r));
            });
    }
};

// Convenience aliases
template <typename T>
using CaseInsensitiveUnorderedMap = std::unordered_map<std::string, T, CaseInsensitiveHash, CaseInsensitiveEqual>;

template <typename T>
using CaseInsensitiveMap = std::map<std::string, T, CaseInsensitiveCompare>;
```

#### Step 3.5.1: Migrate `HashTable<T>` (6 member declarations + 3 pointer declarations)

| Sub-step | Target | Replacement | Files Affected |
|----------|--------|-------------|----------------|
| 3.5.1a | `Resource::m_bitmaps` | `CaseInsensitiveUnorderedMap<BitmapRGBA*>` | `resource.h`, `resource.cpp` |
| 3.5.1b | `Resource::m_displayLists` | `CaseInsensitiveUnorderedMap<int>` | `resource.h`, `resource.cpp` |
| 3.5.1c | `Resource::m_textures` | `CaseInsensitiveUnorderedMap<int>` | `resource.h`, `resource.cpp` |
| 3.5.1d | `Resource::m_shapes` | `CaseInsensitiveUnorderedMap<ShapeStatic*>` | `resource.h`, `resource.cpp` |
| 3.5.1e | `PrefsManager::m_items` | `CaseInsensitiveUnorderedMap<PrefsItem*>` | `preferences.h`, `preferences.cpp` |
| 3.5.1f | `CachedSampleManager::m_cache` | `CaseInsensitiveUnorderedMap<CachedSample*>` | `sample_cache.h`, `sample_cache.cpp` |
| 3.5.1g | `LangTable::m_phrasesKbd`, `m_phrasesXin` | `CaseInsensitiveUnorderedMap<int>*` | `language_table.h`, `language_table.cpp` |

**Call-site migration patterns:**

```cpp
// BEFORE: Index-based iteration
for (unsigned int i = 0; i < table.Size(); ++i) {
    if (table.ValidIndex(i)) {
        auto data = table[i];
        auto name = table.GetName(i);
    }
}

// AFTER: Range-based iteration
for (auto& [name, data] : table) {
    // name is const std::string&, data is T&
}
```

```cpp
// BEFORE: Key lookup
int index = table.GetIndex("key");
if (index >= 0) { T val = table.GetData(index); }
// or
T val = table.GetData("key");
// or
T* ptr = table.GetPointer("key");

// AFTER: Key lookup
auto it = table.find("key");
if (it != table.end()) { T val = it->second; }
// or
T val = table.contains("key") ? table.at("key") : defaultVal;
```

```cpp
// BEFORE: Insert
table.PutData("key", value);

// AFTER: Insert
table.emplace("key", value);
// or
table["key"] = value;
```

```cpp
// BEFORE: Remove
table.RemoveData("key");

// AFTER: Remove
table.erase("key");
```

```cpp
// BEFORE: Empty / EmptyAndDelete
table.Empty();
table.EmptyAndDelete(); // deletes pointed-to objects

// AFTER: Empty
table.clear();
// For EmptyAndDelete:
for (auto& [k, v] : table) { delete v; }
table.clear();
```

**⚠️ `EmptyAndDelete` ownership pattern:** Many `HashTable` instances store raw owning pointers (`HashTable<SomeType*>`). After migration, consider wrapping values in `std::unique_ptr<SomeType>` where ownership is clear. This is optional for this phase but recommended as a follow-up.

**⚠️ `NumUsed()` / `Size()` semantics:** `HashTable::Size()` returns total slots (including empty), `HashTable::NumUsed()` returns occupied slots. `std::unordered_map::size()` returns element count (equivalent to `NumUsed()`). Audit all `Size()` call sites — most actually want element count and should map to `.size()`.

**⚠️ Pre-existing bug fix:** `HashTable::RemoveData(unsigned int)` uses `delete[]` to free keys allocated with `_strdup` (`malloc`). This is undefined behavior. The migration to `std::unordered_map` (which owns `std::string` keys) eliminates this bug automatically.

#### Step 3.5.2: Migrate `BTree<T>` (1 member + 1 local)

| Sub-step | Target | Replacement | Files Affected |
|----------|--------|-------------|----------------|
| 3.5.2a | `LangTable::m_phrasesRaw` | `CaseInsensitiveMap<LangPhrase*>` | `language_table.h`, `language_table.cpp` |
| 3.5.2b | `existingInstances` local in `SoundSystem::Advance` | `CaseInsensitiveMap<float>` | `soundsystem.cpp` (~line 1070) |

**Call-site migration patterns:**

```cpp
// BEFORE: BTree lookup
BTree<float>* node = tree.LookupTree(key);
if (node) { float val = node->data; node->data = newVal; }

// AFTER: std::map lookup
auto it = tree.find(key);
if (it != tree.end()) { float val = it->second; it->second = newVal; }
```

```cpp
// BEFORE: BTree insert
tree.PutData(key, value);

// AFTER: std::map insert
tree.emplace(key, value);
```

```cpp
// BEFORE: ConvertToDArray / ConvertIndexToDArray
DArray<T>* arr = tree.ConvertToDArray();
DArray<char*>* keys = tree.ConvertIndexToDArray();

// AFTER: Direct iteration or collect into std::vector
std::vector<T> values;
values.reserve(tree.size());
for (auto& [k, v] : tree) values.push_back(v);
```

#### Step 3.5.3: Migrate `SortingHashTable<T>` (0 members found)

No member declarations found in the codebase. The class definition and implementation in `sorting_hash_table.h` should be deleted in Phase 8 cleanup (or in this phase if no usages exist). Verify with a full-text search before deletion.

#### Step 3.5.4: Delete Legacy Container Headers

After all call sites are migrated:

| Action | File |
|--------|------|
| Delete | `NeuronClient/hash_table.h` |
| Delete | `NeuronClient/sorting_hash_table.h` |
| Delete | `NeuronClient/btree.h` |
| Remove `#include` | From `pch.h`, `resource.h`, `language_table.h`, `preferences.h`, `sample_cache.h`, and any other includers |

**⚠️ `LangPhrase` migration adjacency:** `LangPhrase` has `char* m_key` and `char* m_string` (both `malloc`/`free`). When migrating `BTree<LangPhrase*>` to `std::map`, the `m_key` field becomes redundant (the map key replaces it). Consider migrating `LangPhrase` at the same time: remove `m_key`, change `m_string` to `std::string`. This simplifies `language_table.cpp` significantly.

**⚠️ `PrefsItem` migration adjacency:** `PrefsItem` has `char* m_key` and `char* m_str` (both heap-allocated). When migrating `HashTable<PrefsItem*>` to `std::unordered_map`, `m_key` becomes redundant. Consider migrating `PrefsItem` at the same time.

**Verification:** Build Debug x64 + Release x64. Run the game, test: resource loading (textures, shapes, bitmaps), preferences save/load, language table switching, sound priority system. Verify that case-insensitive lookups produce identical results to the legacy containers.

---

### Phase 4: Stack Buffer Elimination — NeuronClient (Effort: Medium, ~2–3 days)

NeuronClient has the most calls (145) and the most complex patterns (variadic `vsprintf` in text renderer, heavy `strncpy` in Eclipse UI toolkit). With Phase 3.5 complete, `HashTable`/`BTree` key-building patterns using `char buf[N]` + `snprintf` can be replaced directly with `std::string`/`std::format`.

| Step | Action | Files | Calls |
|------|--------|-------|-------|
| 4.1 | Migrate `eclipse.cpp` / `eclwindow.cpp` / `eclbutton.cpp` — Eclipse UI toolkit string handling | 3 files | 33 |
| 4.2 | Migrate `text_renderer.cpp` — replace local `char buf[512]` with `std::string`, keep `va_list` API for now | 1 file | 18 |
| 4.3 | Migrate `language_table.cpp` — string lookup and table building | 1 file | 17 |
| 4.4 | Migrate `preferences.cpp` — config file read/write | 1 file | 11 |
| 4.5 | Migrate `soundsystem.cpp` / `sound_instance.cpp` — sound name handling | 2 files | 18 |
| 4.6 | Migrate `resource.cpp` — resource path construction | 1 file | 8 |
| 4.7 | Migrate `text_stream_readers.cpp` / `binary_stream_readers.cpp` — file I/O | 2 files | 11 |
| 4.8 | Migrate remaining NeuronClient files | 12 files | 29 |
| 4.9 | Delete `NewStr()` from `string_utils.h` (replace 3 NeuronClient calls + 4 Starstrike + 3 GameRender calls with `std::string`) | `string_utils.h` + 10 call sites | 10 |

**Verification:** Build Debug x64 + Release x64. Run the game, test: language switching, preferences save/load, sound playback, resource loading, text rendering, UI interactions.

---

### Phase 5: Stack Buffer Elimination — Starstrike (Effort: Medium, ~2–3 days)

| Step | Action | Files | Calls |
|------|--------|-------|-------|
| 5.1 | Migrate `taskmanager_interface_icons.cpp` — task bar / HUD text | 1 file | 24 |
| 5.2 | Migrate `GameApp.cpp` — application startup paths | 1 file | 16 |
| 5.3 | Migrate `level_file.cpp` — level loading path construction | 1 file | 16 |
| 5.4 | Migrate `sepulveda_strings.cpp` — narrative text formatting | 1 file | 9 |
| 5.5 | Migrate remaining Starstrike files | 11 files | 39 |

**Verification:** Build Debug x64. Run the game, load every level type, verify HUD text, task manager, level transitions, attract mode.

---

### Phase 6: Member Field Migration (Effort: Large, ~1–2 weeks)

Replace `char m_field[N]` member declarations with `std::string`. This is the highest-risk phase because it changes struct layouts and may affect serialization.

**Pre-requisite:** Phases 2–5 complete (all stack buffers eliminated), Phase 3.5 complete (legacy containers migrated). This ensures that by the time we change member types, all the surrounding code already uses `std::string` locally and container keys are `std::string`.

#### Priority order (by impact and isolation):

| Step | Fields | Header | Risk | Notes |
|------|--------|--------|------|-------|
| 6.1 | `m_scriptFilename` | `scripttrigger.h` | Low | Single field, GameLogic only |
| 6.2 | `m_shapeName` | `generichub.h`, `staticshape.h` | Low | Single field each, GameLogic only |
| 6.3 | `m_script` | `switch.h` | Low | Single field, GameLogic only |
| 6.4 | `m_name`, `m_toolTip` | `taskmanager_interface.h` | Medium | Starstrike UI |
| 6.5 | `m_selectionArrowFilename`, `m_selectionArrowShadowFilename` | `gamecursor_2d.h` | Low | Starstrike, path strings |
| 6.6 | `m_userProfile` | `attract.h` | Low | Starstrike |
| 6.7 | `m_parentName` | `drop_down_menu.h` | Low | GameRender UI |
| 6.8 | `m_buf` | `input_field.h` | Medium | GameRender — interactive text input buffer |
| 6.9 | `m_soundName` | `sound_instance.h` | Medium | NeuronClient — used in sound matching/lookup |
| 6.10 | `m_name` (×3 structs) | `soundsystem.h` | Medium | NeuronClient — blueprint names, used in `HashTable` keys |
| 6.11 | `m_filename`, `m_seperatorChars` | `text_stream_readers.h`, `binary_stream_readers.h` | Medium | NeuronClient — file I/O |
| 6.12 | `m_filename` (`TextRenderer`) | `text_renderer.h` | Low | Heap `char*` (`_strdup`/`free`). Replace with `std::string`. See details below. |
| 6.13 | `m_eventName` (`SoundInstance`) | `sound_instance.h` | Medium | Heap `char*` (`malloc`+manual concat / `_strdup` / `free`). Replace with `std::string`. See details below. |
| 6.14 | `m_eventName` (`SoundEventBlueprint`) | `soundsystem.h` | Medium | Heap `char*` (`NewStr`/`delete[]`). Replace with `std::string`. Tightly coupled with 6.13. |
| 6.15 | `m_userProfileName`, `m_requestedMission`, `m_requestedMap`, `m_gameDataFile` | `GameContext.h` | High | Starstrike — central app context, widely referenced |
| 6.16 | `m_clientIp`, `m_ip` | `networkupdate.h`, `servertoclient.h` | **Permanent exclusion** | IPv4 address buffers — fixed 16-byte wire format. Keep as `char[16]`. See note below. |

> **Note:** `tree.h` `m_leafColourArray[4]` and `m_branchColourArray[4]` are **byte arrays** (unsigned char), not strings. Skip these.

**⚠️ Serialization risk:** Some member fields (`GameContext`) may be serialized with `fwrite`/`fread`. Changing `char[N]` to `std::string` breaks binary layout. Audit each field's serialization before migrating.

**⚠️ Network wire format (step 6.16):** `NetworkUpdate::m_clientIp[16]` and `ServerToClient::m_ip[16]` are **IPv4 address buffers** that exactly fit `"xxx.xxx.xxx.xxx\0"`. These are part of network wire format and **must remain `char[16]`**. They are permanently excluded from this migration. If IPv6 support is ever needed, that is a separate networking task.

#### Step 6.12 Detail: `TextRenderer::m_filename` → `std::string`

**Current state:**
```cpp
// text_renderer.h
class TextRenderer {
protected:
    char *m_filename;       // allocated with _strdup in Initialise
    // ...
public:
    ~TextRenderer() { free(m_filename); }
    void Initialise(char const *_filename);
};
```

**References (5 across 2 files):**
| File | Line | Usage |
|------|------|-------|
| `text_renderer.h:22` | Declaration | `char *m_filename;` |
| `text_renderer.h:33` | Destructor | `free(m_filename);` |
| `text_renderer.cpp:39` | Initialise | `m_filename = _strdup(_filename);` |
| `text_renderer.cpp:48` | BuildOpenGlState | `Resource::GetBinaryReader(m_filename)` |
| `text_renderer.cpp:49` | BuildOpenGlState | `GetExtensionPart(m_filename)` |

**Migration:**
```cpp
// text_renderer.h — AFTER
class TextRenderer {
protected:
    std::string m_filename;
    // ...
public:
    ~TextRenderer() = default;  // std::string handles cleanup
    void Initialise(const char* _filename);
};

// text_renderer.cpp — AFTER
void TextRenderer::Initialise(const char* _filename) {
    m_filename = _filename;
    BuildOpenGlState();
    m_renderShadow = false;
    m_renderOutline = false;
}

void TextRenderer::BuildOpenGlState() {
    BinaryReader* reader = Resource::GetBinaryReader(m_filename.c_str());
    const char* extension = GetExtensionPart(m_filename.c_str());
    // ... rest unchanged ...
}
```

**Risk:** Low. `m_filename` is set once in `Initialise` and only read thereafter. No serialization, no cross-thread access. The `.c_str()` calls at `Resource`/`GetExtensionPart` boundaries are safe because the string outlives both calls.

#### Step 6.13 Detail: `SoundInstance::m_eventName` → `std::string`

**Current state:**
```cpp
// sound_instance.h
class SoundInstance {
    // ...
    char *m_eventName;  // malloc'd compound string "EntityName EventName"
};
```

**References (22 across 2 files):**
| File | Lines | Usage Pattern |
|------|-------|---------------|
| `sound_instance.cpp:115` | Constructor | `m_eventName(nullptr)` |
| `sound_instance.cpp:142` | Destructor | `free(m_eventName)` |
| `sound_instance.cpp:156–167` | `SetEventName` | `malloc` + manual `memcpy` concat of `"entity event"` |
| `sound_instance.cpp:220–221` | `Copy` | `m_eventName = _strdup(_copyMe->m_eventName)` |
| `soundsystem.cpp:225,276,294` | Profiler | `m_eventProfiler->StartProfile(instance->m_eventName)` / `EndProfile` |
| `soundsystem.cpp:345` | Profiler | `EndProfile(instance->m_eventName)` |
| `soundsystem.cpp:633` | Monophonic check | `_stricmp(thisInstance->m_eventName, _instance->m_eventName)` |
| `soundsystem.cpp:869,904,922` | StopAllSounds / NumInstances | `_stricmp(instance->m_eventName, _eventName)` |
| `soundsystem.cpp:1088,1095` | Priority system | `existingInstances.LookupTree(instance->m_eventName)` / `PutData(instance->m_eventName, ...)` |

**Migration:**
```cpp
// sound_instance.h — AFTER
class SoundInstance {
    // ...
    std::string m_eventName;
};

// sound_instance.cpp — AFTER
SoundInstance::SoundInstance()
    : // ... other initializers unchanged ...
{
    // m_eventName default-constructs to empty string
    SetSoundName("[???]");
    // ...
}

SoundInstance::~SoundInstance() {
    // remove free(m_eventName) — std::string handles cleanup
    g_deletingCachedSampleHandle = true;
    delete m_cachedSampleHandle;
    m_cachedSampleHandle = nullptr;
    g_deletingCachedSampleHandle = false;
    m_dspFX.EmptyAndDelete();
    m_objIds.EmptyAndDelete();
}

void SoundInstance::SetEventName(const char* _entityName, const char* _eventName) {
    DEBUG_ASSERT(m_eventName.empty());
    DEBUG_ASSERT(_entityName && _eventName);
    DEBUG_ASSERT(g_context->m_soundSystem);
    m_eventName = std::format("{} {}", _entityName, _eventName);
}

void SoundInstance::Copy(SoundInstance* _copyMe) {
    SetSoundName(_copyMe->m_soundName);
    // ...
    DEBUG_ASSERT(!_copyMe->m_eventName.empty());
    m_eventName = _copyMe->m_eventName;
    // ...
}
```

**Call-site changes in `soundsystem.cpp`:**
```cpp
// BEFORE: _stricmp(instance->m_eventName, _eventName)
// AFTER:  _stricmp(instance->m_eventName.c_str(), _eventName)
//    (or after Phase 7: case-insensitive std::string comparison)

// BEFORE: m_eventProfiler->StartProfile(instance->m_eventName)
// AFTER:  m_eventProfiler->StartProfile(instance->m_eventName.c_str())
//    (depends on Profiler::StartProfile signature)

// BEFORE (post Phase 3.5): existingInstances.find(instance->m_eventName)
// AFTER:  existingInstances.find(instance->m_eventName)  — works directly with std::map<std::string, ...>
```

**⚠️ Dependency:** Step 6.13 depends on Phase 3.5.2b (`BTree<float>` → `std::map<std::string, float>`) being complete, because `m_eventName` is used as a `BTree`/`std::map` key in the sound priority system.

**⚠️ `SetEventName` assert change:** The current code checks `m_eventName == NULL`. After migration, check `m_eventName.empty()`.

**Risk:** Medium. `m_eventName` is used in audio callbacks which run on a separate thread. Verify that all writes happen before the instance is added to the active sound list, and that no concurrent mutation occurs during playback.

#### Step 6.14 Detail: `SoundEventBlueprint::m_eventName` → `std::string`

**Current state:**
```cpp
// soundsystem.h
class SoundEventBlueprint {
public:
    char* m_eventName;  // allocated with NewStr, freed with delete[]
    SoundInstance* m_instance;
    void SetEventName(char* _name);
};
```

**Migration:**
```cpp
// soundsystem.h — AFTER
class SoundEventBlueprint {
public:
    std::string m_eventName;
    SoundInstance* m_instance;
    void SetEventName(const char* _name);
};

// soundsystem.cpp — AFTER
SoundEventBlueprint::SoundEventBlueprint()
    : m_instance(nullptr) {}

void SoundEventBlueprint::SetEventName(const char* _name) {
    if (_name)
        m_eventName = _name;
    else
        m_eventName.clear();
}
```

**Call-site changes:** All `_stricmp(seb->m_eventName, ...)` become `_stricmp(seb->m_eventName.c_str(), ...)` (or case-insensitive `std::string` comparison after Phase 7). `seb->m_instance->SetEventName(_entityName, seb->m_eventName)` needs `.c_str()` if `SetEventName` still takes `const char*`, or works directly if the signature is updated.

**Risk:** Low. `SoundEventBlueprint` is a data holder created during sound loading and read during event dispatch. No threading concerns.

**Verification per step:** Build + run. Test the specific subsystem affected (sound, UI, file I/O, networking, level loading).

---

### Phase 7: Function Signature Modernization (Effort: Medium, ~2–3 days)

Replace `const char*` function parameters with `std::string_view` where the function does not store the string.

**Scope:** 186 function declarations across headers (GameLogic: 26, GameRender: 5, NeuronClient: 114, Starstrike: 37).

| Step | Action | Files | Notes |
|------|--------|-------|-------|
| 7.1 | Change `const char*` → `std::string_view` in NeuronClient headers where the parameter is read-only (not stored) | ~40 headers | Largest batch. Callers passing `std::string` or `const char*` work automatically. |
| 7.2 | Change `const char*` → `std::string_view` in GameLogic headers | ~15 headers | |
| 7.3 | Change `const char*` → `std::string_view` in Starstrike headers | ~20 headers | |
| 7.4 | Change `const char*` → `std::string_view` in GameRender headers | ~5 headers | |

**⚠️ Where NOT to change:**
- Parameters that are stored into a `char[]` member (use `std::string` instead after Phase 6)
- Parameters passed to C APIs (`fopen`, `LoadLibrary`, etc.) — keep `const char*` or add `.data()` at call site
- Parameters used with `HashTable<T>` keys (until HashTable is migrated per `modernize.md` Phase 7.2)
- `string_utils.h` functions (`StrToLower`, `NewStr`) — these should be removed entirely, not signature-changed

**Verification:** Build Debug x64 + Release x64. Full test suite.

---

### Phase 8: Cleanup & Lockdown (Effort: Small, ~0.5 day)

| Step | Action | Files |
|------|--------|-------|
| 8.1 | Delete `string_utils.h` / `string_utils.cpp` (if `NewStr` and `StrToLower` have been fully replaced) | 2 files |
| 8.2 | Remove `_CRT_SECURE_NO_WARNINGS` from `NeuronCore/NeuronCore.h`. Fix any remaining warnings. | 1 file |
| 8.3 | Remove `#include <string.h>` / `#include <cstring>` where no longer needed | Various |
| 8.4 | Verify zero `snprintf`/`sprintf`/`strncpy`/`strcpy`/`vsprintf` calls remain (excluding third-party code in `wil`/`winml`) | — |

**Verification:** Full rebuild Debug x64 + Release x64. Full test suite. Run the game end-to-end.

---

## 📊 Phase Summary

| Phase | Target | Files | Effort | Risk |
|-------|--------|-------|--------|------|
| 1 | Critical safety: `vsprintf` → `vsnprintf`, `strcpy` → `strncpy`, null-termination audit | 37 | ~0.5 day | Low |
| 2 | Stack buffers — GameLogic | 21 | ~1 day | Low |
| 3 | Stack buffers — GameRender | 14 | ~1–2 days | Medium |
| 3.5 | Legacy containers — `HashTable`/`BTree`/`SortingHashTable` → `std::unordered_map`/`std::map` | ~15 | ~3–5 days | Medium–High |
| 4 | Stack buffers — NeuronClient | 23 | ~2–3 days | Medium |
| 5 | Stack buffers — Starstrike | 15 | ~2–3 days | Medium |
| 6 | Member `char[]` / `char*` fields → `std::string` (incl. `TextRenderer::m_filename`, `SoundInstance::m_eventName`, `SoundEventBlueprint::m_eventName`) | 15 headers + impls | ~1–2 weeks | High |
| 7 | `const char*` params → `std::string_view` | ~80 headers | ~2–3 days | Medium |
| 8 | Cleanup: remove `_CRT_SECURE_NO_WARNINGS`, delete legacy utils + legacy container headers | ~8 | ~0.5 day | Low |

**Total estimated effort:** ~5–7 weeks

**Recommended execution order:** 1 → 2 → 3 → 3.5 → 4 → 5 → 6 → 7 → 8 (strictly sequential — each phase builds on the previous)

---

## ✅ Success Criteria

- [ ] Zero `vsprintf` calls (unbounded writes eliminated)
- [ ] Zero `strcpy` calls (unbounded copies eliminated)
- [ ] Zero `sprintf` calls (already at 0 — maintain)
- [ ] Zero `snprintf` calls in project code (all migrated to `std::format`)
- [ ] Zero `strncpy` calls (all migrated to `std::string` assignment)
- [ ] Zero `strncat` calls
- [ ] `NewStr()` deleted — no heap `char[]` allocations for strings
- [ ] `HashTable<T>` deleted — replaced with `std::unordered_map`
- [ ] `SortingHashTable<T>` deleted — replaced with `std::map`
- [ ] `BTree<T>` deleted — replaced with `std::map`
- [ ] `HashTable::RemoveData` allocator mismatch bug (`_strdup`→`delete[]`) eliminated
- [ ] `TextRenderer::m_filename` migrated from `char*` (`_strdup`/`free`) to `std::string`
- [ ] `SoundInstance::m_eventName` migrated from `char*` (`malloc`/`free`) to `std::string`
- [ ] `SoundEventBlueprint::m_eventName` migrated from `char*` (`NewStr`/`delete[]`) to `std::string`
- [ ] Zero `_strdup` / `_stricmp` calls in migrated code (replaced by `std::string` copies and case-insensitive comparators)
- [ ] `_CRT_SECURE_NO_WARNINGS` removed from `NeuronCore.h`
- [ ] `string_utils.h` / `string_utils.cpp` deleted
- [ ] `hash_table.h` / `sorting_hash_table.h` / `btree.h` deleted
- [ ] All 26 member `char[]` fields migrated to `std::string` (except `m_clientIp[16]` and `m_ip[16]` — permanent IPv4 wire-format exclusions)
- [ ] All 5 heap `char*` string members migrated to `std::string`
- [ ] `const char*` parameters changed to `std::string_view` where appropriate
- [ ] Game builds and runs correctly in both Debug x64 and Release x64
- [ ] All 55 existing unit tests pass
