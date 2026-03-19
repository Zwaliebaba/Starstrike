# Starstrike

Starstrike is a Windows-native real-time strategy game built in C++ with Direct3D 12, WinUI 3, and Windows App SDK 1.8. It targets Windows 10+ (x64) and ships as an MSIX-packaged desktop application.

## Documentation

| Document | Description |
|---|---|
| [Architecture](docs/architecture.md) | Module structure, dependency graph, and subsystem design |
| [Development Guide](docs/development-guide.md) | Prerequisites, build instructions, and development workflow |
| [Contributing](docs/contributing.md) | Pull request process, code review standards, and PR guidelines |
| [Coding Standards](.github/coding-standards.md) | Naming conventions, formatting, and language usage |
| [Codebase Improvement Plan](CI.md) | Phased roadmap for modernising the codebase |
| [Math Migration Plan](MathPlan.md) | Plan for migrating legacy math types to DirectXMath |
| [Matrix Convention Plan](MatrixConv.md) | Plan for standardising matrix conventions across the engine |
| [Server Separation Plan](Server.md) | Plan to produce a headless `StarstrikeServer.exe` for Windows Server Core |
| [GameLogic Decoupling](docs/gamelogic.md) | Plan to split `GameLogic` into pure-simulation and render-companion libraries |
| [TreeRenderer Decoupling](docs/tree.md) | Plan to decouple `tree_renderer.cpp` from the OpenGL translation layer |

## Project Structure

```
Starstrike/
├── Starstrike/       # Main application — WinUI 3, MSIX packaging, DirectX 12 renderer
├── GameLogic/        # Game entities, buildings, AI, menus (static lib)
├── NeuronClient/     # Rendering, input, audio, UI framework (static lib)
├── NeuronCore/       # Modern C++ utilities, math, networking (static lib)
├── NeuronServer/     # Network server (static lib)
├── docs/             # Project documentation
└── .github/          # Repository configuration and AI agent definitions
```

**Dependency graph:**

```
Starstrike ──► NeuronClient ──► NeuronCore
          ──► GameLogic
          ──► NeuronCore
NeuronServer ──► NeuronCore
```

## Quick Start

### Prerequisites

- Visual Studio 2022 (v17.x+) with the **Desktop development with C++** and **Windows App SDK** workloads
- Windows SDK 10.0.22621 or later
- NuGet package manager (bundled with Visual Studio)

### Build

```powershell
# Restore NuGet packages
nuget restore Starstrike.slnx

# Build (command line)
msbuild Starstrike.slnx /p:Configuration=Release /p:Platform=x64

# Or open in Visual Studio
# File > Open > Project/Solution > Starstrike.slnx
# Select x64 + Debug, then Build > Build Solution (Ctrl+Shift+B)
```

### Run

Launch the `Starstrike` project from Visual Studio (F5) or run the built MSIX package.

## Technology Stack

| Component | Technology |
|---|---|
| Language | C++20 (Release), C++latest (Debug) |
| Compiler | MSVC v145 (v143 fallback) |
| Graphics API | Direct3D 12 |
| UI Framework | WinUI 3 / Windows App SDK 1.8 |
| Packaging | MSIX |
| Build system | MSBuild / Visual Studio solution (.slnx) |
| Dependencies | NuGet |

## Key Subsystems

- **Rendering** — Direct3D 12 pipeline with a legacy OpenGL compatibility shim (`NeuronClient/opengl_directx`)
- **Game Logic** — Entity and building hierarchy with 18+ entity types and 57+ building types (`GameLogic`)
- **Input** — Pluggable input driver stack with binding and filtering layers (`NeuronClient`)
- **Audio** — 3D spatial audio with DirectSound backend (`NeuronClient`)
- **Networking** — UDP client-server with sequence-ID delivery tracking (`NeuronCore`, `NeuronServer`)
- **Math** — Transitioning from legacy `LegacyVector3`/`Matrix34` to DirectXMath-backed types (see [MathPlan.md](MathPlan.md))
