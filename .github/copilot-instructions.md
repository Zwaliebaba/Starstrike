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

## Important Notes

- Do not add `#include` directives that are already covered by `pch.h`.
- When adding new source files, create a corresponding precompiled header entry (`Use` for `.cpp`, `Create` for `pch.cpp`).
- The Starstrike project includes headers from NeuronCore, NeuronClient, and GameLogic via `AdditionalIncludeDirectories`.
- Build verification: always run a build after making changes to confirm compilation succeeds.
