# Build Instructions

## Prerequisites

### Required Software

- **Operating System**: Windows 10 (version 1909 or later) or Windows 11
- **Visual Studio**: Visual Studio 2019 or 2022 with the following workloads:
  - Desktop development with C++
  - Game development with C++
- **Windows SDK**: Version 10.0.19041.0 or later (for DirectX 12)
- **Git**: For version control

### Recommended Software

- **CMake**: Version 3.20 or later (if using CMake build system)
- **Visual Studio Code**: For lightweight editing (optional)
- **RenderDoc** or **PIX**: For graphics debugging
- **Python**: Version 3.8+ for build scripts and tools

### Hardware Requirements

**Minimum:**
- DirectX 12 compatible GPU
- 8 GB RAM
- 10 GB free disk space

**Recommended:**
- Modern DirectX 12 GPU (NVIDIA GTX 1060 / AMD RX 580 or better)
- 16 GB RAM
- SSD storage for faster builds

## Getting the Source Code

```bash
git clone https://github.com/Zwaliebaba/Starstrike.git
cd Starstrike
```

## Build System

(To be determined - instructions will be added once build system is established)

### Option 1: Visual Studio Solution (Recommended for Windows)

**Steps:**

1. Open `Starstrike.sln` in Visual Studio
2. Select build configuration:
   - **Debug**: For development with full debugging symbols
   - **Release**: Optimized build for testing
   - **Shipping**: Final optimized build with minimal debug info
3. Build the solution (Ctrl+Shift+B)
4. Run the game (F5 or Ctrl+F5)

### Option 2: CMake

**Steps:**

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Debug

# Or build Release
cmake --build . --config Release
```

## Build Configurations

### Debug

- Full debug information
- No optimizations
- DirectX Debug Layer enabled
- Memory leak detection enabled
- Assertions enabled

**Use for**: Day-to-day development, debugging

### Release

- Optimizations enabled (-O2)
- Debug information included
- DirectX Debug Layer disabled
- Assertions enabled
- Suitable for profiling

**Use for**: Performance testing, profiling

### Shipping

- Maximum optimizations (-O3, Link-Time Optimization)
- Minimal debug information
- All debug layers disabled
- Assertions disabled
- Stripped binaries

**Use for**: Final distribution

## Project Structure

```
Starstrike/
├── src/
│   ├── engine/           # Core engine code
│   │   ├── CMakeLists.txt
│   │   └── ...
│   ├── renderer/         # DirectX 12 renderer
│   │   ├── CMakeLists.txt
│   │   └── ...
│   ├── network/          # Networking layer
│   │   ├── CMakeLists.txt
│   │   └── ...
│   └── game/             # Game-specific code
│       ├── CMakeLists.txt
│       └── ...
├── include/              # Public headers
│   └── starstrike/
├── third_party/          # External dependencies
├── assets/               # Game assets
├── tests/                # Unit tests
├── shaders/              # HLSL shader source
├── tools/                # Build tools and scripts
├── CMakeLists.txt        # Root CMake configuration
└── Starstrike.sln        # Visual Studio solution
```

## Shader Compilation

Shaders are written in HLSL and compiled as part of the build process.

### Manual Compilation

```bash
# Compile vertex shader
dxc -T vs_6_0 -E VSMain shader.hlsl -Fo shader_vs.bin

# Compile pixel shader
dxc -T ps_6_0 -E PSMain shader.hlsl -Fo shader_ps.bin
```

### Build Integration

Shaders are automatically compiled during the build process. Compiled shaders are placed in the output directory alongside the executable.

## Dependencies

Dependencies will be managed via one of the following methods:

1. **vcpkg**: Microsoft's C++ package manager (recommended)
2. **Git submodules**: For specific libraries
3. **Manual installation**: For DirectX 12 Agility SDK and other platform-specific SDKs

### Expected Dependencies

- DirectX 12 Agility SDK (for latest D3D12 features)
- DirectX Math library (DirectXMath)
- (Additional dependencies to be determined based on final design)

## Running the Game

### From Visual Studio

Press F5 to run with debugger attached, or Ctrl+F5 to run without debugger.

### From Command Line

```bash
# Debug build
./build/Debug/Starstrike.exe

# Release build
./build/Release/Starstrike.exe
```

### Command Line Arguments

(To be documented once command line interface is implemented)

Example:
```bash
Starstrike.exe --windowed --resolution 1920x1080 --server
```

## Testing

### Running Unit Tests

```bash
# Using CTest (if CMake)
cd build
ctest -C Debug

# Using Visual Studio Test Explorer
# Open Test Explorer (Test > Test Explorer)
# Click "Run All"
```

### Running Integration Tests

(To be documented)

## Debugging

### Visual Studio Debugging

1. Set breakpoints in source code
2. Press F5 to start debugging
3. Use standard Visual Studio debugging tools:
   - Watch windows
   - Call stack
   - Memory viewer
   - CPU profiler

### Graphics Debugging

**Using PIX:**

1. Open PIX for Windows
2. Select "Starstrike.exe" as target
3. Start capture
4. Analyze GPU commands and resources

**Using RenderDoc:**

1. Launch RenderDoc
2. Set executable path to Starstrike.exe
3. Press F12 in-game to capture frame
4. Analyze in RenderDoc UI

### Network Debugging

- Enable verbose network logging via command line flag (to be documented)
- Use Wireshark to capture network traffic
- Enable network statistics overlay (to be implemented)

## Troubleshooting

### Common Build Errors

**Error: Cannot find DirectX 12 headers**
- Solution: Install Windows SDK 10.0.19041.0 or later via Visual Studio Installer

**Error: Missing vcpkg dependencies**
- Solution: Run `vcpkg install` in the project root (once dependencies are defined)

**Error: Shader compilation failed**
- Solution: Check shader syntax, ensure DXC compiler is in PATH

### Common Runtime Errors

**Error: Application failed to create DirectX 12 device**
- Solution: Update GPU drivers, verify GPU supports DirectX 12

**Error: Missing DLL**
- Solution: Ensure all dependencies are in the same directory as the executable

## Development Workflow

### Recommended Workflow

1. Pull latest changes from main branch
2. Create feature branch
3. Build in Debug configuration
4. Run unit tests
5. Implement feature
6. Test locally
7. Build in Release configuration
8. Run full test suite
9. Submit pull request

### Hot Reloading

Hot reloading support is planned for:
- Shaders (automatic recompilation and reload)
- Assets (textures, models)
- (Code hot-reloading is not planned)

## Performance Profiling

### CPU Profiling

**Visual Studio Profiler:**
1. Debug > Performance Profiler
2. Select "CPU Usage"
3. Start profiling
4. Analyze hotspots

**External Profilers:**
- Intel VTune
- AMD μProf
- Superluminal

### GPU Profiling

- Use PIX for GPU timing queries
- Enable GPU markers in code for detailed profiling
- Use vendor-specific tools (NVIDIA Nsight, AMD Radeon GPU Profiler)

## Continuous Integration

CI configuration will be added to automatically:
- Build all configurations
- Run unit tests
- Run static analysis
- Generate documentation

## Additional Resources

- [DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [CMake Documentation](https://cmake.org/documentation/)
- [Visual Studio Documentation](https://docs.microsoft.com/en-us/visualstudio/)

---

**Note**: This document will be updated as the build system and project structure are finalized.
