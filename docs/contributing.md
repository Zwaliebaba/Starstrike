# Contributing

This document describes the process for contributing code changes to Starstrike.

## Workflow

1. Create a feature branch from `main`:
   ```powershell
   git checkout main
   git pull origin main
   git checkout -b feature/short-description
   ```
2. Make your changes following the [Coding Standards](.github/../.github/coding-standards.md).
3. Build and validate locally (see [Development Guide](development-guide.md)).
4. Commit with a clear message (see [Commit Messages](#commit-messages) below).
5. Push and open a pull request.

## Pull Request Guidelines

### Title Format

```
[component] Brief description of change
```

Examples:
- `[renderer] Fix shadow mapping artefacts on terrain`
- `[gamelogic] Add turret cooldown after overheat`
- `[neuroncore] Replace busy-wait in ASyncLoader with condition variable`
- `[docs] Update architecture diagram`

Valid component values: `renderer`, `gamelogic`, `neuronclient`, `neuroncore`, `neuronserver`, `input`, `audio`, `networking`, `ui`, `build`, `docs`.

### Required Checks

Every pull request must pass:

- [ ] **Debug x64 build** — `msbuild Starstrike.slnx /p:Configuration=Debug /p:Platform=x64`
- [ ] **Release x64 build** — `msbuild Starstrike.slnx /p:Configuration=Release /p:Platform=x64`
- [ ] Manual gameplay validation (launch the game and exercise the affected feature)

### PR Description

Include the following sections in your PR description:

```markdown
## Summary
- What changed and why

## Testing
- Steps to validate the change manually
- Any regression risks and how they were checked

## Notes (optional)
- Performance impact, known limitations, follow-up work
```

For changes that affect gameplay or rendering, include a brief description of the visual or mechanical impact.

## Commit Messages

Use the imperative mood, keep the subject line under 72 characters, and reference the affected component:

```
[component] Short description of what the commit does

Optional body explaining the motivation and any non-obvious
implementation details. Wrap lines at 72 characters.
```

Examples:
```
[neuroncore] Replace busy-wait in ASyncLoader with condition variable

The previous implementation spun the CPU at 100% while waiting for
assets to load. Replacing with std::condition_variable reduces CPU
usage and improves responsiveness during level transitions.
```

```
[gamelogic] Fix null deref in Building::Create for unknown type

Building::Create returned an uninitialised pointer when given an
unrecognised type enum value. Added a nullptr guard and a fatal-
error log call to catch unregistered types early.
```

## Code Review Standards

Reviewers will check:

1. **Correctness** — Does the change do what it claims? Are edge cases handled?
2. **Standards** — Does the code follow [Coding Standards](.github/../.github/coding-standards.md)?
3. **Safety** — No new unsafe string operations (`sprintf`, `strcpy`, `strcat`). Prefer safe alternatives or document why unsafe usage is unavoidable.
4. **Memory** — New allocations in NeuronCore use smart pointers. Legacy code in GameLogic/NeuronClient may use raw pointers if consistent with surrounding code.
5. **Build impact** — Changes to widely-included headers are justified and minimised.
6. **Documentation** — New public APIs, subsystems, or non-obvious behaviour are documented.
7. **No regressions** — Existing gameplay and rendering remain unaffected.

## Documentation Requirements

When your change introduces or modifies a public API, subsystem, or architectural pattern:

- Update or add documentation in `docs/` if the change affects architecture, the development workflow, or the contribution process.
- Update `.github/coding-standards.md` if a new convention is established.
- Add inline comments for non-obvious logic.
- For significant architectural decisions, consider adding an ADR entry (see [Architecture Decision Records](architecture.md#architecture-decision-records) format in `docs/architecture.md`).

## Adding New Subsystems

If your change adds a new subsystem or library:

1. Determine which existing project it belongs to, or propose a new project with justification.
2. Follow the dependency rules:
   - `GameLogic` must not depend on `NeuronClient` or `Starstrike`.
   - `NeuronCore` must not depend on any other project in this solution.
   - `NeuronServer` must not depend on `GameLogic` or `NeuronClient`.
3. Create paired `.h`/`.cpp` files with `PascalCase` names.
4. Configure precompiled header usage in the `.vcxproj`.
5. Update `docs/architecture.md` to describe the new subsystem.

## Style Checklist

Before opening a PR, verify:

- [ ] File names are `PascalCase.cpp` / `PascalCase.h`
- [ ] Classes and methods use `PascalCase`
- [ ] Member variables use `m_camelCase`
- [ ] Global pointers use `g_` prefix
- [ ] Constants use `UPPER_SNAKE_CASE`
- [ ] Header uses `#pragma once` (not `#ifndef` guard)
- [ ] New `.cpp` includes `pch.h` as the first line
- [ ] No new includes already covered by `pch.h`
- [ ] Assertions use `ASSERT_TEXT` / `DEBUG_ASSERT` (not `assert`)
- [ ] Debug logging uses `Neuron::DebugTrace` (not `printf` or `OutputDebugString` directly)
- [ ] COM objects use `winrt::com_ptr<T>` (not `Microsoft::WRL::ComPtr<T>`)
- [ ] New NeuronCore code uses C++20 idioms (`constexpr`, `[[nodiscard]]`, `std::format`, etc.)
