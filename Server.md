# Server Separation Plan

## Goal

Extract the game server — currently embedded in the client as `m_server` (`GameApp.h:44`) —
into a standalone Windows executable that runs inside a **Windows container**.

---

## Current State

| Location | Role |
|----------|------|
| `NeuronClient/server.h/.cpp` | `Server` class — inbox/outbox, tick loop, client tracking |
| `NeuronClient/servertoclient.h/.cpp` | Per-client connection wrapper |
| `NeuronClient/servertoclientletter.h/.cpp` | Server → Client UDP message format |
| `NeuronClient/networkupdate.h/.cpp` | Client → Server UDP message format |
| `NeuronCore/net_*.h/.cpp` | Cross-platform UDP socket + thread primitives |
| `Starstrike/GameApp.h:44` | `Server* m_server;` — null when in client-only mode |
| `Starstrike/main.cpp:541-548` | `new Server()` + `Initialise()` when `IAmAServer` pref is set |
| `Starstrike/main.cpp:229-237` | `m_server->Advance()` called at 10 Hz in the game loop |
| `NeuronClient/clienttoserver.cpp:101-102` | Bypass mode: client calls `m_server->ReceiveLetter()` directly |

The game supports three modes today:

1. **Pure client** — `IAmAServer=0`, connects to a remote server via `ServerAddress` pref.
2. **Integrated server + client** — `IAmAServer=1`, `BypassNetwork=1`, single process.
3. **Dedicated server (manual)** — `IAmAServer=1`, `BypassNetwork=0`, listens on UDP 4000.

Mode 3 already works at the protocol level; we just need a proper standalone executable
and container image for it.

---

## Step 1 — Move Server Files to `NeuronServer`

The `NeuronServer` static-lib project already exists in the solution (`NeuronServer/NeuronServer.vcxproj`).
Move the following files **out of** `NeuronClient` **into** `NeuronServer`:

```
NeuronClient/server.h             →  NeuronServer/server.h
NeuronClient/server.cpp           →  NeuronServer/server.cpp
NeuronClient/servertoclient.h     →  NeuronServer/servertoclient.h
NeuronClient/servertoclient.cpp   →  NeuronServer/servertoclient.cpp
```

Keep the protocol/message files in `NeuronClient` because they are shared with the client side:

```
NeuronClient/networkupdate.h/.cpp          (shared — keep in NeuronClient)
NeuronClient/servertoclientletter.h/.cpp   (shared — keep in NeuronClient)
```

Update `NeuronServer.vcxproj`:
- Add the two moved `.cpp` files as `<ClCompile>` items.
- Add `NeuronClient` and `NeuronCore` to `<AdditionalIncludeDirectories>`.
- Add project references to `NeuronClient` and `NeuronCore`.

Update `NeuronClient.vcxproj`:
- Remove the four moved files.
- Any remaining includes of `server.h` or `servertoclient.h` inside `NeuronClient`
  (currently only `clienttoserver.cpp`) must use the new path or the include directories
  must be updated.

---

## Step 2 — Create `StarstrikeServer` Executable Project

Add a new project `StarstrikeServer/StarstrikeServer.vcxproj` to the solution:

| Setting | Value |
|---------|-------|
| Output type | Application (`.exe`) |
| Platform toolset | v145 |
| C++ standard | C++20 |
| Target platform | x64, Windows 10+ |
| Subsystem | Console (`/SUBSYSTEM:CONSOLE`) |
| No MSIX packaging | This is a headless server |

**Dependencies (project references):**
- `NeuronServer`
- `NeuronClient` (for shared protocol types: `NetworkUpdate`, `ServerToClientLetter`)
- `NeuronCore` (for `net_*`, `DArray`, `LList`, math utilities)

**Files to create:**

### `StarstrikeServer/main.cpp`

```cpp
// StarstrikeServer/main.cpp
//
// Standalone dedicated server entry point.
// Initialises the Server, then ticks it at 10 Hz until the process is killed
// or receives a console CTRL+C / CTRL+BREAK signal.

#include <windows.h>
#include <csignal>
#include <chrono>
#include <thread>
#include "server.h"          // NeuronServer
#include "preferences.h"     // NeuronClient

PrefsManager* g_prefsManager = nullptr;

static volatile bool g_running = true;

static BOOL WINAPI ConsoleCtrlHandler(DWORD)
{
    g_running = false;
    return TRUE;
}

int main(int argc, char* argv[])
{
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    // Minimal preferences needed by the server
    g_prefsManager = new PrefsManager("prefs.txt");

    Server server;
    server.Initialise();   // binds UDP port 4000, starts listener thread

    using Clock = std::chrono::steady_clock;
    constexpr auto TICK = std::chrono::milliseconds(100);   // 10 Hz
    auto next = Clock::now() + TICK;

    while (g_running)
    {
        server.Advance();
        std::this_thread::sleep_until(next);
        next += TICK;
    }

    delete g_prefsManager;
    return 0;
}
```

Key points:
- No `GameApp`, no rendering, no audio.
- `g_prefsManager` is still created because `Server` and `PrefsManager` are used across the
  existing codebase — it reads `prefs.txt` in the working directory.
- `Server::Advance()` handles all inbox draining, outbox flushing, and client management
  exactly as it does today.
- `Server::AdvanceSender()` is called inside `Advance()` (verify in `server.cpp`) or call it
  explicitly after `Advance()` if it is a separate step.

### `StarstrikeServer/prefs.txt` (default server configuration)

```
IAmAServer 1
BypassNetwork 0
ServerPort 4000
```

---

## Step 3 — Client Changes

### Remove `m_server` from the client

With a dedicated server, the client always runs in **pure client** mode.

**`Starstrike/GameApp.h`**
- Remove `Server* m_server;` (line 44).
- Remove the `#include "server.h"` if it is only there for `m_server`.

**`Starstrike/main.cpp`**
- Remove the `IAmAServer` preference check block (lines 541–548).
- Remove the `m_server->Advance()` call block (lines 229–237).
- Remove cleanup `delete g_app->m_server` (lines 403–404).

**`NeuronClient/clienttoserver.cpp`**
- Remove the bypass-mode code path (lines 101–102):
  ```cpp
  // DELETE:
  if (g_app->m_bypassNetworking)
      g_app->m_server->ReceiveLetter(letter, g_app->m_clientToServer->GetOurIP_String());
  ```
  Bypass mode is no longer needed once the server is a separate process.
- Remove any remaining `#include "server.h"` from client files.

### Update client preferences default

`ServerAddress` must now point to the container's hostname or IP. Document this in
`README.md`:

```
ServerAddress <container-hostname-or-ip>
```

---

## Step 4 — Windows Container Setup

### Base image choice

Use **Windows Server Core** (not Nano Server) because the MSVC C++ runtime is not available
on Nano Server without manual extraction.

Recommended tag: `mcr.microsoft.com/windows/servercore:ltsc2022`

### `StarstrikeServer/Dockerfile`

```dockerfile
# ── Build stage ──────────────────────────────────────────────────────────────
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS build

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop';"]

# Install Build Tools for Visual Studio 2022 (C++ workload, no IDE)
RUN Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vs_buildtools.exe `
        -OutFile vs_buildtools.exe ; `
    Start-Process -FilePath vs_buildtools.exe -Wait -ArgumentList `
        '--quiet', '--wait', '--norestart', `
        '--add', 'Microsoft.VisualStudio.Workload.VCTools', `
        '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', `
        '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.22621' ; `
    Remove-Item vs_buildtools.exe

# Install NuGet CLI
RUN Invoke-WebRequest -Uri https://dist.nuget.org/win-x86-commandline/latest/nuget.exe `
        -OutFile C:\Windows\nuget.exe

WORKDIR C:\src
COPY . .

RUN nuget restore Starstrike.slnx
RUN & 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' `
        StarstrikeServer\StarstrikeServer.vcxproj `
        /p:Configuration=Release /p:Platform=x64

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM mcr.microsoft.com/windows/servercore:ltsc2022

WORKDIR C:\server

# Copy the MSVC runtime DLLs alongside the binary
COPY --from=build C:\src\StarstrikeServer\x64\Release\StarstrikeServer.exe .
COPY --from=build `
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.*\x64\Microsoft.VC143.CRT\*.dll" .

COPY StarstrikeServer\prefs.txt .

EXPOSE 4000/udp

ENTRYPOINT ["StarstrikeServer.exe"]
```

> **Note:** The exact MSVC redist path depends on the installed toolset version.
> Use a glob or hard-code the specific version found during the build stage.

### `docker-compose.yml` (optional, for local testing)

```yaml
version: "3.9"
services:
  starstrike-server:
    build:
      context: .
      dockerfile: StarstrikeServer/Dockerfile
    ports:
      - "4000:4000/udp"
    restart: unless-stopped
```

Run with:
```powershell
docker compose up --build
```

Requires Docker Desktop with **Windows containers** mode enabled.

---

## Step 5 — Solution File Update

Add `StarstrikeServer` to `Starstrike.slnx`:

```xml
<Project Path="StarstrikeServer\StarstrikeServer.vcxproj" />
```

Ensure build order dependencies are declared so `NeuronCore`, `NeuronClient`, and
`NeuronServer` are built before `StarstrikeServer`.

---

## Step 6 — CI / Build Pipeline

Add a build step that:
1. Builds `StarstrikeServer` in Release/x64.
2. Runs `docker build` to produce the container image.
3. (Optional) Pushes the image to a container registry.

Example GitHub Actions addition to the existing workflow:

```yaml
- name: Build StarstrikeServer
  run: msbuild StarstrikeServer\StarstrikeServer.vcxproj /p:Configuration=Release /p:Platform=x64

- name: Build Docker image
  run: docker build -t starstrike-server:${{ github.sha }} -f StarstrikeServer\Dockerfile .
```

---

## Dependency Graph (after separation)

```
StarstrikeServer.exe
  └── NeuronServer (static lib)  ← server.h/.cpp, servertoclient.h/.cpp
        └── NeuronClient (static lib)  ← networkupdate, servertoclientletter, preferences
              └── NeuronCore (static lib)  ← net_*, DArray, LList, math

Starstrike.exe (client, no m_server)
  └── GameLogic (static lib)
  └── NeuronClient (static lib)
        └── NeuronCore (static lib)
```

---

## File Change Summary

| Action | File |
|--------|------|
| Move | `NeuronClient/server.h/.cpp` → `NeuronServer/` |
| Move | `NeuronClient/servertoclient.h/.cpp` → `NeuronServer/` |
| Edit | `NeuronServer/NeuronServer.vcxproj` — add moved files, add include dirs |
| Edit | `NeuronClient/NeuronClient.vcxproj` — remove moved files |
| Edit | `Starstrike/GameApp.h` — remove `Server* m_server` |
| Edit | `Starstrike/main.cpp` — remove server init, advance, and cleanup |
| Edit | `NeuronClient/clienttoserver.cpp` — remove bypass-mode server call |
| Create | `StarstrikeServer/main.cpp` |
| Create | `StarstrikeServer/StarstrikeServer.vcxproj` |
| Create | `StarstrikeServer/prefs.txt` |
| Create | `StarstrikeServer/Dockerfile` |
| Create | `docker-compose.yml` |
| Edit | `Starstrike.slnx` — add `StarstrikeServer` project |
| Edit | `README.md` — document `ServerAddress` configuration |
