# AGENTS.md

## Project Overview

Starstrike is a Windows-native C++ game built with Visual Studio (.slnx). It targets Windows 10+ (x64) and ships as an MSIX app using Windows App SDK 1.8 and WinUI 3. Rendering is Direct3D 12. The solution contains multiple static libraries (NeuronClient, NeuronCore, NeuronServer, GameLogic) and the Starstrike app.

## Setup Commands

- Install dependencies: `nuget restore Starstrike.slnx`
- Start development server: N/A (desktop app)
- Build for production: `msbuild Starstrike.slnx /p:Configuration=Release /p:Platform=x64`

## Development Workflow

- Open `Starstrike.slnx` in Visual Studio 2022 (v17.x+), select `x64` + `Debug`, and build/run the `Starstrike` project.
- No hot reload/watch mode is configured.
- No required environment variables are defined.

## Testing Instructions

- Run all tests: N/A (no automated test suite configured)
- Run unit tests: N/A
- Run integration tests: N/A
- Test coverage: N/A
- Manual validation is expected after rendering or gameplay changes.

## Code Style

- Naming: files `PascalCase.cpp/.h`, classes `PascalCase`, methods `PascalCase`, members `m_` + `camelCase`, globals `g_` prefix, constants `UPPER_SNAKE_CASE`.
- Indentation: 2 spaces in app-level code; some libraries use tabs. Match the file.
- Assertions/logging: `DarwiniaReleaseAssert`, `DarwiniaDebugAssert`, `Neuron::DebugTrace`, `Neuron::Fatal`.
- PCH: all projects use `pch.h` / `pch.cpp`.
- Rendering: do not add new functionality to `ImRenderer`; use `SpriteBatch` for new textured-quad work.

## Build and Deployment

- Build the `Starstrike` project for MSIX packaging (x64).
- Use `Debug` for local dev and `Release` for shipping builds.
- Deployment is via MSIX produced by the Starstrike app project.

## Pull Request Guidelines

- Title format: [component] Brief description
- Required checks: build `x64` in `Debug` and `Release`.
- Include a short summary of gameplay/graphics impact when applicable.

## Additional Notes

- Avoid adding includes already covered by `pch.h`.
- NeuronCore favors modern C++20/23 patterns; NeuronClient/GameLogic contain legacy code patterns (raw pointers, C-style strings).
- Build after changes to confirm compilation succeeds.
