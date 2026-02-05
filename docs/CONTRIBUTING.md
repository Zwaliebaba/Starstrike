# Contributing to Starstrike

Thank you for your interest in contributing to Starstrike! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

This project adheres to a code of conduct that all contributors are expected to follow. Please be respectful, inclusive, and professional in all interactions.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Starstrike.git
   cd Starstrike
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/Zwaliebaba/Starstrike.git
   ```
4. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Workflow

### 1. Before You Start

- Check existing issues and pull requests to avoid duplicate work
- For major changes, open an issue first to discuss your proposal
- Ensure you understand the project architecture (see [ARCHITECTURE.md](ARCHITECTURE.md))

### 2. Making Changes

**Coding Standards:**

- Follow modern C++ best practices (C++17/20)
- Use RAII for resource management
- Prefer `std::unique_ptr`/`std::shared_ptr` over raw pointers
- Use `const` and `constexpr` where appropriate
- Avoid unnecessary allocations in performance-critical code
- Write self-documenting code; add comments for complex logic

**Code Style:**

```cpp
// Namespace usage
namespace Starstrike::Engine {

// Class naming: PascalCase
class EntityManager {
public:
    // Public methods: PascalCase
    Entity CreateEntity();
    void DestroyEntity(Entity entity);
    
    // Getters/Setters: Get/Set prefix
    World* GetWorld() const { return m_world.get(); }
    
private:
    // Private members: m_ prefix, camelCase
    std::unique_ptr<World> m_world;
    std::vector<Entity> m_entities;
    
    // Private methods: camelCase
    void updateSystems(float deltaTime);
};

// Constants: UPPER_SNAKE_CASE
constexpr uint32_t MAX_ENTITIES = 10000;

// Enums: PascalCase for enum, PascalCase for values
enum class ComponentType {
    Transform,
    MeshRenderer,
    Rigidbody
};

} // namespace Starstrike::Engine
```

**Formatting:**
- Indentation: 4 spaces (no tabs)
- Braces: Allman style (braces on new line)
- Line length: Aim for 100 characters, hard limit at 120
- File encoding: UTF-8

### 3. Writing Tests

All new features and bug fixes should include tests:

- **Unit tests** for individual components/functions
- **Integration tests** for system interactions
- **Regression tests** for bug fixes

Example test structure:
```cpp
#include <gtest/gtest.h>
#include <starstrike/Engine.h>

TEST(EntityManagerTest, CreateEntity) {
    Starstrike::Engine::World world;
    auto entity = world.CreateEntity();
    EXPECT_TRUE(entity.IsValid());
}

TEST(EntityManagerTest, DestroyEntity) {
    Starstrike::Engine::World world;
    auto entity = world.CreateEntity();
    world.DestroyEntity(entity);
    EXPECT_FALSE(world.HasEntity(entity));
}
```

### 4. Building and Testing

Before submitting your changes:

```bash
# Build in Debug mode
cmake --build build --config Debug

# Run tests
ctest --test-dir build -C Debug

# Build in Release mode
cmake --build build --config Release

# Run tests again
ctest --test-dir build -C Release
```

Ensure all tests pass and there are no compiler warnings.

### 5. Committing Changes

**Commit Message Format:**

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `perf`: Performance improvements
- `chore`: Build system, dependencies, etc.

**Examples:**

```
feat(renderer): Add skybox rendering support

Implements cubemap rendering for skybox backgrounds.
Uses pre-filtered environment maps for reflections.

Closes #123
```

```
fix(network): Fix packet deserialization bug

Corrects byte order issues when reading int64 values
from network packets on big-endian systems.

Fixes #456
```

**Commit Guidelines:**
- Keep commits focused and atomic
- Write clear, descriptive commit messages
- Reference issue numbers when applicable
- Sign-off your commits if required by project policy

### 6. Submitting a Pull Request

1. **Update your fork**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create Pull Request** on GitHub with:
   - Clear title describing the change
   - Detailed description of what and why
   - Reference to related issues
   - Screenshots/videos for visual changes

4. **PR Checklist**:
   - [ ] Code follows project style guidelines
   - [ ] All tests pass
   - [ ] New tests added for new features
   - [ ] Documentation updated (if applicable)
   - [ ] No compiler warnings
   - [ ] Commit messages follow guidelines
   - [ ] Branch is up-to-date with main

### 7. Code Review Process

- Maintainers will review your PR
- Address feedback and requested changes
- Keep the PR updated with main branch
- Once approved, your PR will be merged

## Areas for Contribution

### High Priority

- Core engine systems implementation
- DirectX 12 renderer implementation
- Networking layer implementation
- Input system
- Resource management
- Math library optimizations

### Medium Priority

- Unit tests and integration tests
- Documentation improvements
- Example projects
- Build system improvements
- Performance profiling tools

### Low Priority (Nice to Have)

- Editor tools
- Asset pipeline tools
- Additional platform support
- Third-party integrations

## Reporting Bugs

When reporting bugs, please include:

1. **Description**: Clear description of the issue
2. **Steps to Reproduce**: Minimal steps to reproduce the bug
3. **Expected Behavior**: What you expected to happen
4. **Actual Behavior**: What actually happened
5. **Environment**:
   - OS version
   - GPU model and driver version
   - Build configuration (Debug/Release)
   - Commit hash or version
6. **Logs**: Relevant error messages or logs
7. **Screenshots**: If applicable

Use the GitHub issue template when available.

## Suggesting Enhancements

For feature requests:

1. **Check existing issues** to avoid duplicates
2. **Describe the feature** clearly
3. **Explain the use case** and benefits
4. **Provide examples** of similar implementations (if any)
5. **Consider implications**: Performance, API changes, compatibility

## Performance Considerations

When contributing performance-critical code:

- Profile before and after changes
- Document performance characteristics
- Consider cache-friendliness
- Minimize allocations in hot paths
- Use appropriate data structures
- Consider multi-threading implications

## Documentation

Documentation improvements are always welcome:

- Fix typos and unclear explanations
- Add missing API documentation
- Provide usage examples
- Update outdated information
- Improve code comments

## Platform-Specific Contributions

Currently targeting Windows/DirectX 12. When contributing:

- Use platform abstractions where appropriate
- Document platform-specific behavior
- Consider future cross-platform support
- Test on different Windows versions if possible

## Security

If you discover a security vulnerability:

- **Do NOT** open a public issue
- Email the maintainers directly (contact info to be provided)
- Provide detailed information about the vulnerability
- Allow time for a fix before public disclosure

## Legal

By contributing, you agree that:

- Your contributions are your original work
- You have the right to submit the contributions
- Your contributions may be distributed under the project's license
- You grant the project maintainers a perpetual, worldwide, non-exclusive license to use your contributions

## Getting Help

- **Documentation**: Check [docs/](../docs/) directory
- **Issues**: Search existing issues or create a new one
- **Discussions**: Use GitHub Discussions for questions
- **Discord**: (To be set up - for real-time chat)

## Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes
- Project documentation

Significant contributions may result in commit access to the repository.

## Style Guides

### C++ Style Guide

Follow the C++ Core Guidelines with these specifics:

**Headers:**
```cpp
// myheader.h
#pragma once

#include <standard_library_headers>

#include "project_headers.h"
#include "third_party/library.h"

namespace Starstrike::Module {

class MyClass {
    // ... 
};

} // namespace Starstrike::Module
```

**Source Files:**
```cpp
// myclass.cpp
#include "myheader.h"

#include <standard_library_headers>

#include "other_project_headers.h"

namespace Starstrike::Module {

void MyClass::MyMethod() {
    // Implementation
}

} // namespace Starstrike::Module
```

### Documentation Style

Use Doxygen-style comments for public APIs:

```cpp
/**
 * @brief Creates a new entity in the world.
 * 
 * Entities are lightweight handles to game objects. Components can be
 * attached to entities to give them behavior and data.
 * 
 * @return A valid entity handle
 * @throws std::bad_alloc if entity pool is exhausted
 * 
 * @see DestroyEntity
 * @see AddComponent
 */
Entity CreateEntity();
```

## Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [DirectX 12 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [Game Programming Patterns](https://gameprogrammingpatterns.com/)
- [DirectX 12 Best Practices](https://developer.nvidia.com/dx12-dos-and-donts)

## License

(License information to be determined)

---

Thank you for contributing to Starstrike! Your efforts help make this project better for everyone.
