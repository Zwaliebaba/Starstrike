# Copilot Instructions — Starstrike

## Project Overview

Starstrike is a Windows-native C++ game built with Visual Studio using the `.slnx` solution format. It targets **x64 Windows 10+** (minimum 10.0.17763.0) and is packaged as an **MSIX** application using the **Windows App SDK 1.8** and **WinUI 3**.

## Solution Structure

| Project | Type | Role |
|---|---|---|
| **Starstrike** | Application (.exe) | Main WinUI 3 entry point, game loop, rendering, camera, scripting, task management |
| **NeuronClient** | Static Library | Client-side engine: rendering primitives, sound (DirectSound), UI (Eclipse widget system), input, resource management, text rendering, window management |
| **NeuronCore** | Static Library | Core engine: networking (WinSock2), file I/O, math, threading, C++/WinRT interop, debug utilities |
| **NeuronServer** | Static Library | Server-side networking |
| **GameLogic** | Static Library | Game entities, buildings, AI, weapons, UI windows, preferences |

### Dependency Graph

```
Starstrike → NeuronClient, NeuronCore, GameLogic
NeuronClient → NeuronCore, GameLogic
NeuronServer → NeuronCore
GameLogic → (standalone)
```

## Build & Toolchain

- **Platform toolset**: v145 (VS 2022 17.x+) with v143 fallback
- **C++ standard**: `stdcpplatest` (Debug), `stdcpp20` (Release)
- **Precompiled headers**: Every project uses `pch.h` / `pch.cpp`
- **Package management**: NuGet via `packages.config`
- **Character set**: Unicode
- **Key packages**: Microsoft.WindowsAppSDK 1.8, Microsoft.Windows.CppWinRT 2.0, Microsoft.Windows.SDK.BuildTools

## Coding Conventions

### Naming

- **Files**: `snake_case.cpp` / `snake_case.h`
- **Classes**: `PascalCase` (e.g., `SystemInfo`, `SoundLibrary3dDSound`)
- **Methods**: `PascalCase` (e.g., `GetLocaleDetails`, `ReadFile`)
- **Member variables**: `m_` prefix with `camelCase` (e.g., `m_preferredDevice`, `m_homeDir`)
- **Global pointers**: `g_` prefix (e.g., `g_app`, `g_systemInfo`, `g_prefsManager`)
- **Constants / macros**: `UPPER_SNAKE_CASE`

### Style

- Indentation: **2 spaces** in Starstrike/app-level code; some library code uses **tabs** — match the existing file's style
- Include guards: Legacy code uses `#ifndef` / `#define` / `#endif`; newer NeuronCore headers use `#pragma once`
- Braces: Opening brace on the **same line** as the declaration for control flow; **next line** for function/class definitions in some files — match surrounding code

### Patterns

- Raw pointers with manual `new` / `delete` are common in legacy code (NeuronClient, GameLogic). Do not refactor existing code to smart pointers unless specifically asked.
- `Neuron` namespace is used for all NeuronCore types and utilities.
- Custom assert macros: `DarwiniaReleaseAssert(condition, msg)` and `DarwiniaDebugAssert(x)` — use these instead of standard `assert`.
- Debug output: `Neuron::DebugTrace(fmt, args...)` using `std::format` syntax.
- Fatal errors: `Neuron::Fatal(fmt, args...)` — `[[noreturn]]`, triggers `__debugbreak()`.
- Globals: Singleton-style global pointers are the norm (e.g., `g_app`, `g_systemInfo`). Do not introduce dependency injection or service locators.

### Modern C++ Usage (NeuronCore)

NeuronCore uses modern C++20/23 features. When writing code in NeuronCore:
- Prefer `std::string_view`, `std::format`, `constexpr`, `[[nodiscard]]`, `noexcept`
- Use `std::vector`, `std::array`, `std::ranges`, `std::mdspan` where appropriate
- Use C++/WinRT for Windows Runtime API access

### Legacy Code (NeuronClient, GameLogic)

These projects contain ported legacy code. When modifying:
- Keep C-style string usage (`char*`) consistent with surrounding code
- Use `strdup`, `new[]` / `delete[]` as existing code does
- Use the existing `DarwiniaReleaseAssert` / `DarwiniaDebugAssert` macros

## Audio

- Audio uses **DirectSound** (`dsound.h`, `mmsystem.h`)
- Sound device enumeration via `waveOutGetDevCaps` / `waveOutGetNumDevs`
- Sound library abstraction: `SoundLibrary3dDSound`

## Networking

- Uses **WinSock2** (`winsock2.h`, `ws2tcpip.h`)
- Client-server architecture with `ClientToServer`, `ServerToClient`, `ServerToClientLetter`
- Custom `NetSocket`, `NetSocketListener`, `NetMutex`, `NetThread` abstractions

## File I/O

- `Neuron::FileSys` base with `Neuron::BinaryFile` and `Neuron::TextFile` for asset loading
- Home directory set to `Assets\` subdirectory
- Legacy file readers: `TextStreamReaders`, `BinaryStreamReaders`

## Rendering Architecture

### Current State

The rendering pipeline is built on **Direct3D 11**. Key components:

| Component | File(s) | Role |
|---|---|---|
| `RenderDevice` | `render_device.h/.cpp` | D3D11 device, swap chain, back buffer management |
| `RenderStates` | `render_states.h/.cpp` | Pre-built blend, depth, and rasteriser states |
| `TextureManager` | `texture_manager.h/.cpp` | D3D11 texture creation, SRV lookup, sampler states |
| `ImRenderer` | `im_renderer.h/.cpp` | **Legacy** — OpenGL-style immediate-mode emulation over D3D11 |
| `TextRenderer` | `text_renderer.h/.cpp` | Bitmap font rendering (currently uses `ImRenderer`) |
| `SpriteBatch` | `sprite_batch.h/.cpp` | **Planned** — batched 2D/3D sprite and text rendering |

### `ImRenderer` — Legacy (Do Not Extend)

`ImRenderer` (`g_imRenderer`) is an **OpenGL immediate-mode emulation layer** over D3D11. It was created as a transitional bridge during the OpenGL → D3D11 port. **Do not add new functionality to `ImRenderer` or write new code that depends on it.** Specifically:

- **Do not** add new `Begin(PRIM_QUADS)` / `End()` sequences for textured quads or sprites — use `SpriteBatch` instead (once available) or note the code as pending migration.
- **Do not** extend the `ImRenderer` constant buffer, vertex format, or shader pair.
- **Do not** use `ImRenderer` matrix state (`SetProjectionMatrix`, `SetViewMatrix`, `SetWorldMatrix`) for new rendering paths.
- Existing `ImRenderer` usage for **line rendering**, **debug wireframes**, and **lit 3D mesh rendering** is acceptable until those subsystems are individually migrated.

The full migration plan is documented in [`DX11Text.md`](../DX11Text.md).

### `SpriteBatch` — Replacement for Sprite/Text Rendering

All new 2D/3D textured-quad rendering (text glyphs, UI sprites, cursor, logos) should target the `SpriteBatch` API:
- `g_spriteBatch->Begin2D()` / `End2D()` for screen-space quads
- `g_spriteBatch->Begin3D()` / `End3D()` for world-space billboards
- `g_spriteBatch->AddQuad2D()` / `AddQuad3D()` to batch geometry
- Uses a dedicated lightweight shader pair (no lighting/fog), pre-built index buffer, and single `DrawIndexed()` per batch.

### Shaders

HLSL shaders live in `NeuronClient/Shaders/` and are compiled offline into byte arrays in `NeuronClient/CompiledShaders/`.

| Shader | Purpose |
|---|---|
| `im_defaultVS.hlsl` | Vertex shader for `ImRenderer` (full WVP + lighting + fog) |
| `im_texturedPS.hlsl` | Pixel shader for `ImRenderer` textured draws |
| `im_coloredPS.hlsl` | Pixel shader for `ImRenderer` untextured draws |
| `sprite_vs.hlsl` | **Planned** — Vertex shader for `SpriteBatch` (ViewProj only) |
| `sprite_ps.hlsl` | **Planned** — Pixel shader for `SpriteBatch` (tex × color) |

## Important Notes

- Do not add `#include` directives that are already covered by `pch.h`.
- When adding new source files, create a corresponding precompiled header entry (`Use` for `.cpp`, `Create` for `pch.cpp`).
- The Starstrike project includes headers from NeuronCore, NeuronClient, and GameLogic via `AdditionalIncludeDirectories`.
- Build verification: always run a build after making changes to confirm compilation succeeds.
