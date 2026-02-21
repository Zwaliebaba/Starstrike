# Agents — Starstrike

This file describes available Copilot agent modes and when to use each for the Starstrike project.

## Code Agent (default)

Use the default code agent for everyday tasks:

- Adding or modifying gameplay entities in **GameLogic** (buildings, AI, weapons, units)
- Implementing rendering features in **Starstrike** or **NeuronClient**
- Writing or updating UI code using the **Eclipse** widget system (`eclbutton.h`, `eclwindow.h`)
- Adding or adjusting sound effects via **DirectSound** (`SoundLibrary3dDSound`)
- Networking code changes using the **WinSock2** abstractions in **NeuronCore**
- Adding new source files (remember to configure precompiled headers: `Use` for `.cpp`, `Create` for `pch.cpp`)
- Bug fixes, feature additions, and general C++ coding tasks across all five projects

### Key project conventions to follow

- **NeuronCore**: Modern C++20/23 — use `std::format`, `std::string_view`, `constexpr`, `[[nodiscard]]`, `noexcept`, C++/WinRT
- **NeuronClient / GameLogic**: Legacy C++ — use `char*`, `new`/`delete`, `strdup`, `DarwiniaReleaseAssert`/`DarwiniaDebugAssert`
- **Starstrike**: App-level glue — 2-space indent, PascalCase methods, `g_` globals
- Match the existing file's indentation style (spaces vs tabs) when editing

## Modernization Agent

Use `/modernize` or ask to upgrade/migrate when you need to:

- Upgrade the Windows App SDK or CppWinRT NuGet packages
- Move from `packages.config` to `PackageReference`
- Target a newer Windows SDK version or platform toolset
- Migrate legacy APIs to modern equivalents (e.g., DirectSound → XAudio2, WinSock2 → Windows::Networking)
- Convert project files to SDK-style or update the `.slnx`

## Profiler Agent

Use `/profile` or ask about performance when you need to:

- Diagnose slow game loop frames, rendering bottlenecks, or audio latency
- Profile memory usage in legacy code with manual allocations
- Benchmark specific subsystems (e.g., entity grid, particle system, networking)
- Measure the impact of code changes on frame time or throughput
