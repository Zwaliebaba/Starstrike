# Development Guide

This guide covers everything needed to set up a development environment, build the project, and follow the established development workflow for Starstrike.

## Prerequisites

| Requirement | Version |
|---|---|
| Windows | 10 or later (x64) |
| Visual Studio | 2022 (v17.x+) |
| VS workload: Desktop development with C++ | Required |
| VS workload: Windows application development | Required (for WinUI 3 / Windows App SDK) |
| Windows SDK | 10.0.22621 or later |
| NuGet | Bundled with Visual Studio |

## Initial Setup

1. Clone the repository:
   ```powershell
   git clone <repo-url>
   cd Starstrike
   ```

2. Restore NuGet packages:
   ```powershell
   nuget restore Starstrike.slnx
   ```
   Or open `Starstrike.slnx` in Visual Studio — it will prompt to restore packages automatically.

3. Open the solution:
   ```
   File > Open > Project/Solution > Starstrike.slnx
   ```

## Build Configurations

| Configuration | C++ Standard | Warning Level | Use For |
|---|---|---|---|
| Debug x64 | `stdcpplatest` | W4 (Starstrike), W3 (libs) | Local development and debugging |
| Release x64 | `stdcpp20` | Default | Shipping builds and performance testing |

**Always build both configurations before submitting a pull request.**

### Command-Line Builds

```powershell
# Debug
msbuild Starstrike.slnx /p:Configuration=Debug /p:Platform=x64

# Release
msbuild Starstrike.slnx /p:Configuration=Release /p:Platform=x64

# Parallel build (faster)
msbuild Starstrike.slnx /p:Configuration=Debug /p:Platform=x64 /m
```

### Visual Studio Build

Select the target in the toolbar (`x64` + `Debug` or `Release`), then:
- **Build Solution**: `Ctrl+Shift+B`
- **Rebuild Solution**: `Build > Rebuild Solution`
- **Build a single project**: Right-click project → `Build`

## Running the Game

- Press **F5** in Visual Studio to build and launch the `Starstrike` project with the debugger attached.
- Press **Ctrl+F5** to launch without the debugger (faster startup).
- The application runs as a WinUI 3 desktop window. No server process is required for local single-player.

## Project Layout

```
Starstrike/
├── Starstrike/           # Main app — add new gameplay-facing files here
│   ├── pch.h/cpp         # Precompiled header
│   └── ...
├── GameLogic/            # Game rules, entities, buildings, AI
│   ├── pch.h/cpp
│   └── ...
├── NeuronClient/         # Legacy rendering, input, audio, math
│   ├── pch.h/cpp
│   └── ...
├── NeuronCore/           # Modern utilities, math, networking
│   ├── pch.h/cpp
│   └── ...
├── NeuronServer/         # Network server
│   ├── pch.h/cpp
│   └── ...
├── docs/                 # Project documentation
│   ├── architecture.md   # Module structure, dependency graph, subsystem design
│   ├── development-guide.md
│   ├── contributing.md
│   ├── gamelogic.md      # Simulation–rendering decoupling plan
│   └── tree.md           # TreeRenderer dependency reduction plan
├── .github/              # Repo configuration and AI agent definitions
├── Starstrike.slnx       # Visual Studio solution
├── CI.md                 # Codebase improvement roadmap (index of all initiatives)
├── MathPlan.md           # Math type migration plan
├── MatrixConv.md         # Matrix convention standardisation plan
└── Server.md             # Server separation plan (headless StarstrikeServer.exe)
```

## Adding New Files

1. Create `NewFeature.h` and `NewFeature.cpp` in the appropriate project directory using `PascalCase` names.
2. Add both files to the `.vcxproj`:
   - Right-click the project in Solution Explorer → `Add > Existing Item`
3. In the `.vcxproj`, configure PCH usage for the new `.cpp`:
   - Set **Precompiled Header** = `Use` and **Precompiled Header File** = `pch.h`
4. Ensure `#include "pch.h"` is the first line of the new `.cpp`.
5. Use `#pragma once` at the top of the new `.h` (not `#ifndef` guards).

## Debugging

### Debug Output

Use `Neuron::DebugTrace` for debug logging (uses `std::format` syntax):
```cpp
Neuron::DebugTrace("Unit {} health: {}", m_id, m_health);
```

For fatal errors that should terminate the process:
```cpp
Neuron::Fatal("Failed to load asset: {}", path);
```

### Assertions

```cpp
ASSERT_TEXT(ptr != nullptr, "Expected non-null pointer");
DEBUG_ASSERT(index < m_count);
```

Use `ASSERT_TEXT` and `DEBUG_ASSERT` — do not use the standard `assert()` macro.

### GPU Debugging

- **PIX for Windows**: Attach to the Starstrike process to capture GPU frames.
- **RenderDoc**: Supported for DX12 frame capture.
- HLSL shader source lives in `NeuronClient/Shaders/` (`VertexShader.hlsl`, `PixelShader.hlsl`, `TreeVertexShader.hlsl`, `TreePixelShader.hlsl`, `PerFrameConstants.hlsli`).
- Pre-compiled DXIL bytecode is checked in as C++ headers under `NeuronClient/CompiledShaders/`. Rebuild the `NeuronClient` project to regenerate them after shader edits.

## Testing

There is no automated test suite. Manual validation is expected after every change:

1. Launch the game and verify the affected feature works correctly.
2. Check that no visual regressions are introduced in rendering or UI.
3. For networking changes, test with a local client and server.
4. Build in both `Debug` and `Release` configurations.

If you introduce a new utility function in `NeuronCore`, consider writing a Visual Studio Native Unit Test to validate its behaviour independently of the full game.

## Dependency Management

Dependencies are managed via NuGet:
- Package metadata is in the `.vcxproj` files.
- Run `nuget restore` after pulling changes that modify package references.
- Do not check in `packages/` directories — they are restored on build.

Key packages:
- **Microsoft.WindowsAppSDK** (1.8.x) — WinUI 3, packaging, Windows Runtime
- **Microsoft.Direct3D.D3D12** — DirectX 12 headers and libs
- **DirectXMath** — SIMD math used by `NeuronCore/GameMath.h`

## Environment Variables

No environment variables are required. All configuration is embedded in the Visual Studio project files and the application's MSIX manifest.

## Common Issues

| Problem | Fix |
|---|---|
| Build fails with "Cannot open include file: pch.h" | Ensure PCH usage is configured in the `.vcxproj` for the new `.cpp` file |
| NuGet package not found | Run `nuget restore Starstrike.slnx` |
| MSIX deployment fails | Ensure the Windows App SDK workload is installed and the developer mode is enabled in Windows Settings |
| Shader compilation errors | Rebuild the `NeuronClient` project; DXIL headers are pre-compiled and checked in |
| Linker errors after adding a file | Verify the new `.cpp` is listed in the correct `.vcxproj` and included in both Debug and Release configurations |
