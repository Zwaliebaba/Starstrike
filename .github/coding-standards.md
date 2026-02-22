# Coding Standards

These conventions reflect the established patterns in the Starstrike codebase. Follow existing file-local style when a file deviates.

## Language and Toolchain

- Target platform: Windows 10+ (x64), MSIX packaging.
- Toolset: MSVC v145 (v143 fallback).
- Standard: `stdcpplatest` (Debug), `stdcpp20` (Release).
- Precompiled headers: every project uses `pch.h` / `pch.cpp`.

## Naming

- Files: `PascalCase.cpp` / `PascalCase.h`.
- Classes/structs: `PascalCase`.
- Methods/functions: `PascalCase`.
- Member variables: `m_` prefix with `camelCase` (e.g., `m_homeDir`).
- Global pointers: `g_` prefix (e.g., `g_app`).
- Constants/macros: `UPPER_SNAKE_CASE`.

## Formatting and Layout

- Indentation: 2 spaces in app-level code; some library code uses tabs. Match the file.
- Braces: control-flow opening brace on the same line; function/class definitions vary by file. Match surrounding style.
- Include guards: use `#pragma once`.

## Includes and Headers

- Do not add includes already covered by `pch.h`.
- When adding new source files, configure PCH usage (`Use` for `.cpp`, `Create` for `pch.cpp`).

## Assertions and Diagnostics

- Use `ASSERT_TEXT(condition, msg)` and `DEBUG_ASSERT(x)` (avoid standard `assert`).
- Debug logging: `Neuron::DebugTrace(fmt, args...)` with `std::format` syntax.
- Fatal errors: `Neuron::Fatal(fmt, args...)`.

## Pointers and Ownership

- Legacy code uses raw pointers and manual `new` / `delete`. Refactor to smart pointers unless requested.

## NeuronCore Modern C++ Usage

When working inside NeuronCore:

- Prefer `std::string_view`, `std::format`, `constexpr`, `[[nodiscard]]`, `noexcept`.
- Prefer `std::vector`, `std::array`, `std::ranges`, `std::mdspan` where appropriate.
- Use C++/WinRT for Windows Runtime API access.

## Legacy Code (NeuronClient, GameLogic)

- Keep C-style strings (`char*`) consistent with surrounding code.
- Use `strdup`, `new[]` / `delete[]` as the file already does.

## Build Verification

- Run a build after changes to confirm compilation succeeds.
