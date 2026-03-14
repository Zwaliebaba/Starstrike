# Server Separation Plan

## Executive Summary

The original plan assumed the server could link against `NeuronClient`.
This is not viable: Windows Server Core does not support the WinRT runtime,
and `NeuronClient` (and even the existing `NeuronServer`) both pull in WinRT
through their aggregate umbrella headers.

The contamination must be eliminated at two independent sources:

| Source | File | Symbol |
|--------|------|--------|
| Aggregate PCH chain | `NeuronServer.h` → `NeuronCore.h` | `#include <winrt/…>` + `using namespace winrt` |
| Aggregate PCH chain | `NeuronClient.h` → `NeuronCore.h` | same |
| Direct GameApp include | `server.cpp`, `servertoclient.cpp`, `preferences.cpp` | `#include "GameApp.h"` → `GameMain` → WinRT interfaces |

Both sources must be cut before the server can compile on Server Core.

---

## 1. WinRT Contamination — Exact Root Causes

### 1.1 The NeuronCore.h aggregate header (the primary poison)

`NeuronCore/NeuronCore.h` lines 77–84:

```cpp
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Threading.Core.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.System.h>
using namespace winrt;
```

Every project in the solution routes its `pch.h` through one of the
aggregate umbrella headers that include `NeuronCore.h`:

```
NeuronServer/pch.h
  → NeuronServer/NeuronServer.h
      → NeuronCore/NeuronCore.h          ← WinRT enters here

NeuronClient/pch.h
  → NeuronClient/NeuronClient.h
      → NeuronCore/NeuronCore.h          ← WinRT enters here
      → <winrt/Windows.Devices.Enumeration.h>
      → <winrt/Windows.UI.Core.h>
      → <winrt/Windows.UI.Popups.h>
```

Because every `.cpp` file begins with `#include "pch.h"`, every translation
unit in both libraries is already WinRT-contaminated before a single
project-specific header is reached.

### 1.2 Direct GameApp.h includes

Three server-related source files explicitly `#include "GameApp.h"`:

| File | Line | Symbol used |
|------|------|-------------|
| `NeuronClient/server.cpp` | 3 | `g_app->m_server`, `g_app->m_bypassNetworking`, `g_app->m_clientToServer`, `g_app->m_profiler` |
| `NeuronClient/servertoclient.cpp` | 4 | `g_app->m_bypassNetworking` |
| `NeuronClient/preferences.cpp` | 2 | `g_app->m_resource` |

`GameApp` inherits from `GameMain`, and `GameMain` implements
`Windows::Foundation::IInspectable` — a WinRT interface.  Even if the PCH
were clean, including `GameApp.h` reintroduces WinRT.

---

## 2. What the Server Actually Uses from GameApp

The five `g_app->` references inside the server files, and what they do:

### `NeuronClient/server.cpp`

| Location | Expression | Purpose | Server-build replacement |
|----------|-----------|---------|--------------------------|
| `ListenCallback` (line 40) | `g_app->m_server` | Route incoming UDP packet to Server inbox | Module-static `static Server* s_server` set in `Initialise()` |
| `Initialise()` (line 86) | `g_app->m_bypassNetworking` | Skip real UDP setup in bypass mode | Remove check; dedicated server always uses real sockets |
| `AdvanceSender()` (line 288) | `g_app->m_bypassNetworking` | Route outbound letter in-process | Remove entire bypass branch |
| `AdvanceSender()` (line 289) | `g_app->m_clientToServer->ReceiveLetter()` | In-process delivery to local client | Remove (bypass mode only) |
| `Advance()` (lines 316, 436) | `g_app->m_profiler` | Perf profiling | Already no-ops: `START_PROFILE`/`END_PROFILE` are empty macros when `PROFILER_ENABLED` is undefined |

Note: `g_prefsManager->GetInt("RecordDemo")` (line 405) is **not** a
GameApp reference — `g_prefsManager` is an independent global — it stays
unchanged.

### `NeuronClient/servertoclient.cpp`

| Location | Expression | Purpose | Server-build replacement |
|----------|-----------|---------|--------------------------|
| constructor (line 13) | `g_app->m_bypassNetworking` | Skip socket creation in bypass mode | Remove; always create socket |

### `NeuronClient/preferences.cpp`

| Location | Expression | Purpose | Server-build replacement |
|----------|-----------|---------|--------------------------|
| `CreateDefaultValues()` (line 308) | `g_app->m_resource` | Read `default_preferences.txt` from resource system | Remove; server reads plain prefs file |

---

## 3. Transitive Type Dependencies from networkupdate.h

`networkupdate.h` includes `worldobject.h`, `entity.h`, and `team.h`.
The full chain:

```
networkupdate.h
  ├── worldobject.h   → rgb_colour.h (clean), LegacyVector3.h (clean)
  ├── entity.h        → llist.h (clean), texture_uv.h (clean), worldobject.h
  └── team.h          → fast_darray.h (clean), slice_darray.h (clean),
                         rgb_colour.h, worldobject.h, entity.h
```

**Critical finding:** None of these headers contain WinRT themselves.
`rgb_colour.h`, `LegacyVector3.h`, `texture_uv.h`, `worldobject.h`,
`entity.h`, and `team.h` are plain C++ data-class headers.  The WinRT
exposure is 100% a PCH artefact.

However, `worldobject.h` and `entity.h` appear to be legacy dead-includes
in `networkupdate.h` — the `NetworkUpdate` struct has no `WorldObject` or
`Entity` fields.  Only `TeamControls` (from `team.h`) and `LegacyVector3`
(from NeuronCore) are actually used in the struct.

The implementation of `TeamControls`, `WorldObjectId`, and `WorldObject`
methods are defined in `.cpp` files that the server does not need to link
against, because the server never calls those methods — it treats
`NetworkUpdate` as an opaque byte container and uses only its scalar fields
and serialisation functions.

---

## 4. Fix Strategy

Two orthogonal fixes, applied together:

### Fix A — Replace the NeuronServer umbrella header with a WinRT-free version

`NeuronServer/NeuronServer.h` currently chains through `NeuronCore.h`.
Replace it with a hand-curated list of individual NeuronCore headers
that are each WinRT-free:

```cpp
// NeuronServer/NeuronServer.h  —  NEW CONTENT (WinRT-free)
#pragma once

#define TARGET_MSVC
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define SERVER_BUILD                // enables conditional compilation guards

#pragma warning(disable:4201)
#pragma warning(disable:4238)
#pragma warning(disable:4244)
#pragma warning(disable:4324)

// Standard C++ subset needed by server
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <format>
#include <memory>
#include <functional>

#define NOMINMAX
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP

// Win32 networking — NOT WinRT
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
// NOTE: No <hstring.h>, <unknwn.h>, <winrt/…> here

// Individual NeuronCore utility headers (all WinRT-free):
#include "Debug.h"      // DEBUG_ASSERT, DebugTrace, ASSERT_TEXT
#include "GameMath.h"   // basic math helpers
// LegacyVector3.h, llist.h, darray.h etc. included on-demand by server headers

#pragma comment(lib, "Ws2_32.lib")
```

`NeuronServer/pch.h` is unchanged (`#include "NeuronServer.h"`) — it
inherits the new clean content automatically.

### Fix B — Introduce `#ifdef SERVER_BUILD` guards in server source files

The `SERVER_BUILD` macro defined in `NeuronServer.h` is picked up by the
server PCH before any server `.cpp` file is processed.  Each of the
offending references is then wrapped so the client build is unaffected.

---

## 5. Exact Code Changes Required

### 5.1 `NeuronClient/server.cpp`

**Lines 3–4** — Remove GameApp and ClientToServer includes for server build:
```cpp
// BEFORE:
#include "GameApp.h"
#include "clienttoserver.h"

// AFTER:
#ifndef SERVER_BUILD
#include "GameApp.h"
#include "clienttoserver.h"
#endif
```

**Lines 32–51** — Replace `g_app->m_server` in `ListenCallback` with a
module-level static pointer that the server registers itself:

```cpp
// Add before ListenCallback:
#ifdef SERVER_BUILD
static Server* s_server = nullptr;
#endif

static NetCallBackRetType ListenCallback(NetUdpPacket* udpdata)
{
  if (udpdata)
  {
    NetIpAddress* fromAddr = &udpdata->m_clientAddress;
    char newip[16];
    IpToString(fromAddr->sin_addr, newip);

#ifdef SERVER_BUILD
    if (s_server)
    {
      auto letter = new NetworkUpdate(udpdata->m_data);
      s_server->ReceiveLetter(letter, newip);
    }
#else
    if (g_app->m_server)
    {
      auto letter = new NetworkUpdate(udpdata->m_data);
      g_app->m_server->ReceiveLetter(letter, newip);
    }
#endif

    delete udpdata;
  }
  return 0;
}
```

**Lines 81–93** — Remove bypass-mode guard in `Initialise()`:
```cpp
void Server::Initialise()
{
  m_inboxMutex  = new NetMutex();
  m_outboxMutex = new NetMutex();

#ifdef SERVER_BUILD
  // Dedicated server always uses real networking
  s_server = this;
  m_netLib = new NetLib();
  m_netLib->Initialise();
  NetStartThread(ListenThread);
#else
  if (!g_app->m_bypassNetworking)
  {
    m_netLib = new NetLib();
    m_netLib->Initialise();
    NetStartThread(ListenThread);
  }
#endif
}
```

**Lines 276–312** — Remove bypass-mode branch in `AdvanceSender()`:
```cpp
// Inside the while (m_outbox.Size()) loop, replace:
//   if (g_app->m_bypassNetworking)
//       g_app->m_clientToServer->ReceiveLetter(letter);
//   else { ... real send ... }
// With:
#ifdef SERVER_BUILD
      // Dedicated server: always send over UDP
      int linearSize = 0;
      ServerToClient* client = m_clients[letter->GetClientId()];
      NetSocket* socket = client->GetSocket();
      char* linearisedLetter = letter->GetByteStream(&linearSize);
      socket->WriteData(linearisedLetter, linearSize);
      bytesSentThisFrame += linearSize;
      delete letter;
#else
      if (g_app->m_bypassNetworking)
        g_app->m_clientToServer->ReceiveLetter(letter);
      else
      {
        int linearSize = 0;
        ServerToClient* client = m_clients[letter->GetClientId()];
        NetSocket* socket = client->GetSocket();
        char* linearisedLetter = letter->GetByteStream(&linearSize);
        socket->WriteData(linearisedLetter, linearSize);
        bytesSentThisFrame += linearSize;
        delete letter;
      }
#endif
```

**Lines 314–436** — `Advance()` profiler calls:
No change required.  `START_PROFILE` and `END_PROFILE` are already defined
as empty macros when `PROFILER_ENABLED` is not set (which it is not in
Release builds and will not be in the server build).

### 5.2 `NeuronClient/servertoclient.cpp`

**Lines 1–6** — Remove GameApp include:
```cpp
// BEFORE:
#include "pch.h"
#include "net_lib.h"
#include "net_socket.h"
#include "GameApp.h"
#include "servertoclient.h"

// AFTER:
#include "pch.h"
#include "net_lib.h"
#include "net_socket.h"
#ifndef SERVER_BUILD
#include "GameApp.h"
#endif
#include "servertoclient.h"
```

**Lines 8–21** — Remove bypass check in constructor:
```cpp
ServerToClient::ServerToClient(char* _ip)
  : m_socket(nullptr)
{
  strcpy(m_ip, _ip);

#ifdef SERVER_BUILD
  // Dedicated server always creates a real socket to reach the client
  m_socket = new NetSocket();
  NetRetCode retCode = m_socket->Connect(_ip, 4001);
  DEBUG_ASSERT(retCode == NetOk);
#else
  if (!g_app->m_bypassNetworking)
  {
    m_socket = new NetSocket();
    NetRetCode retCode = m_socket->Connect(_ip, 4001);
    DEBUG_ASSERT(retCode == NetOk);
  }
#endif

  m_lastKnownSequenceId = -1;
}
```

### 5.3 `NeuronClient/preferences.cpp`

**Lines 2–8** — Remove GameApp and UI-layer includes:
```cpp
// BEFORE:
#include "pch.h"
#include "GameApp.h"
#include "preferences.h"
#include "resource.h"
#include "text_stream_readers.h"
#include "prefs_other_window.h"

// AFTER:
#include "pch.h"
#ifndef SERVER_BUILD
#include "GameApp.h"
#include "resource.h"
#include "text_stream_readers.h"
#include "prefs_other_window.h"
#endif
#include "preferences.h"
```

**Lines 307–312** — Remove resource-file default loading:
```cpp
// BEFORE:
  if (g_app && g_app->m_resource)
  {
    TextReader* reader = g_app->m_resource->GetTextReader("default_preferences.txt");
    if (reader && reader->IsOpen()) { while (reader->ReadLine()) { AddLine(reader->GetRestOfLine(), true); } }
  }

// AFTER:
#ifndef SERVER_BUILD
  if (g_app && g_app->m_resource)
  {
    TextReader* reader = g_app->m_resource->GetTextReader("default_preferences.txt");
    if (reader && reader->IsOpen()) { while (reader->ReadLine()) { AddLine(reader->GetRestOfLine(), true); } }
  }
#endif
```

### 5.4 `NeuronClient/networkupdate.h`

The includes of `worldobject.h` and `entity.h` are legacy dead-includes
(no field in `NetworkUpdate` is of those types).  Guard them to shrink the
server's dependency surface, eliminating any future risk if those headers
ever gain indirect WinRT deps:

```cpp
// BEFORE:
#include "worldobject.h"
#include "entity.h"
#include "team.h"

// AFTER:
#ifndef SERVER_BUILD
#include "worldobject.h"
#include "entity.h"
#endif
#include "team.h"     // TeamControls is an actual field — required
```

`team.h` transitively includes `worldobject.h` and `entity.h` anyway, but
those headers are WinRT-free so this is safe; guarding the direct includes
eliminates one link-time symbol set that the server will never call.

---

## 6. Files to Add to `NeuronServer.vcxproj`

The server static lib must compile all the networking and protocol files.
Add the following as `<ClCompile>` items with their relative paths:

```xml
<!-- Protocol layer — currently in NeuronClient, compiled here with server PCH -->
<ClCompile Include="..\NeuronClient\server.cpp" />
<ClCompile Include="..\NeuronClient\servertoclient.cpp" />
<ClCompile Include="..\NeuronClient\servertoclientletter.cpp" />
<ClCompile Include="..\NeuronClient\networkupdate.cpp" />
<ClCompile Include="..\NeuronClient\preferences.cpp" />
```

Corresponding header references (for IDE navigation):
```xml
<ClInclude Include="..\NeuronClient\server.h" />
<ClInclude Include="..\NeuronClient\servertoclient.h" />
<ClInclude Include="..\NeuronClient\servertoclientletter.h" />
<ClInclude Include="..\NeuronClient\networkupdate.h" />
<ClInclude Include="..\NeuronClient\preferences.h" />
```

**`<AdditionalIncludeDirectories>`** for `NeuronServer.vcxproj` must include:

```
$(SolutionDir)NeuronCore;
$(SolutionDir)NeuronClient;
$(SolutionDir)GameLogic;
$(SolutionDir)Starstrike;
```

The GameLogic and Starstrike include paths are needed for `team.h`,
`worldobject.h`, and `entity.h` which are reachable through `networkupdate.h`.

**Important:** Do NOT add a project reference from `NeuronServer` to
`NeuronClient`.  Adding those files as direct `<ClCompile>` items in
`NeuronServer.vcxproj` means they compile with `NeuronServer`'s clean PCH
(picked up via `/Yu"pch.h"` from the NeuronServer project) instead of
NeuronClient's WinRT-contaminated one.

---

## 7. Create `StarstrikeServer` Executable Project

### `StarstrikeServer/StarstrikeServer.vcxproj`

| Setting | Value |
|---------|-------|
| Output type | Application (`.exe`) |
| Platform toolset | v145 |
| C++ standard | C++20 |
| Subsystem | `/SUBSYSTEM:CONSOLE` |
| No packaging | headless server — no MSIX |
| Configuration | Release / x64 only needed |

**Project references (libs to link):**
- `NeuronServer` (contains server logic + protocol, compiled WinRT-free)
- `NeuronCore` (socket, thread, mutex, math primitives)

**Do NOT reference:**
- `NeuronClient` — WinRT-contaminated aggregate header
- `GameLogic` — game rendering / entity AI, not needed
- `Starstrike` — packaged app host, not needed

### `StarstrikeServer/pch.h`

```cpp
#pragma once
#include "NeuronServer.h"   // inherits SERVER_BUILD + clean Win32 includes
```

### `StarstrikeServer/pch.cpp`

```cpp
#include "pch.h"
```

### `StarstrikeServer/main.cpp`

```cpp
#include "pch.h"
#include "server.h"
#include "preferences.h"

PrefsManager* g_prefsManager = nullptr;

static volatile LONG g_running = 1;

static BOOL WINAPI ConsoleCtrlHandler(DWORD)
{
    InterlockedExchange(&g_running, 0);
    return TRUE;
}

int main()
{
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    g_prefsManager = new PrefsManager("prefs.txt");

    Server server;
    server.Initialise();   // binds UDP port 4000, starts listener thread

    constexpr DWORD TICK_MS = 100;   // 10 Hz
    DWORD next = GetTickCount() + TICK_MS;

    while (InterlockedCompareExchange(&g_running, 1, 1))
    {
        server.Advance();
        DWORD now = GetTickCount();
        if (next > now)
            Sleep(next - now);
        next += TICK_MS;
    }

    delete g_prefsManager;
    return 0;
}
```

Note: `g_prefsManager` is declared as an `extern` in `preferences.h` with
the definition here in `main.cpp`.  The client build defines it in
`preferences.cpp` / GameApp initialisation; the server defines it here.
Wrap the definition in `preferences.cpp` with `#ifndef SERVER_BUILD` to
avoid a duplicate-symbol linker error:

```cpp
// preferences.cpp, line 9:
#ifndef SERVER_BUILD
PrefsManager* g_prefsManager = nullptr;
#endif
```

### `StarstrikeServer/prefs.txt`

```
IAmAServer 1
BypassNetwork 0
RecordDemo 0
```

---

## 8. Client-Side Cleanup (Starstrike.exe)

Remove the now-dead in-process server code from the client:

**`Starstrike/GameApp.h`**
- Remove `Server* m_server;` (line 44)
- Remove `#include "server.h"` or the forward declaration `class Server;` if it was added there

**`Starstrike/main.cpp`**
- Remove `IAmAServer` init block (lines 541–548): `new Server()` + `Initialise()`
- Remove server advance block (lines 229–237): `m_server->Advance()`
- Remove cleanup block (lines 403–404): `delete g_app->m_server`

**`NeuronClient/clienttoserver.cpp`**
- Remove bypass branch (lines 101–102):
  `g_app->m_server->ReceiveLetter(letter, …)` — dead once `m_server` is gone

---

## 9. Windows Container

### Why Server Core is correct

Windows Server Core (`mcr.microsoft.com/windows/servercore:ltsc2022`):
- Supports Win32 API, WinSock2, and kernel32 — all that the server needs
- Ships with `Ws2_32.dll`, `kernel32.dll`, `ntdll.dll`
- **Does not ship the WinRT runtime** (`Windows.Foundation.dll`, etc.)
- Does not ship DXGI, Direct3D, or any graphics stack

After the fixes in sections 5–7, the server binary has zero WinRT imports
in its import table, making it fully compatible with Server Core.

The MSVC C++ runtime (`vcruntime140.dll`, `msvcp140.dll`) is not present on
Server Core by default and must be distributed alongside the binary.

### `StarstrikeServer/Dockerfile`

```dockerfile
# ── Build stage ──────────────────────────────────────────────────────────────
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS build

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop';"]

# Install Build Tools for Visual Studio 2022 (C++ workload only, no IDE)
RUN Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vs_buildtools.exe `
        -OutFile vs_buildtools.exe ; `
    Start-Process -FilePath vs_buildtools.exe -Wait -ArgumentList `
        '--quiet', '--wait', '--norestart', `
        '--add', 'Microsoft.VisualStudio.Workload.VCTools', `
        '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', `
        '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.22621' ; `
    Remove-Item vs_buildtools.exe

RUN Invoke-WebRequest -Uri https://dist.nuget.org/win-x86-commandline/latest/nuget.exe `
        -OutFile C:\Windows\nuget.exe

WORKDIR C:\src
COPY . .

RUN nuget restore Starstrike.slnx

# Build only the server executable and its dependencies
RUN & 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' `
        StarstrikeServer\StarstrikeServer.vcxproj `
        /p:Configuration=Release /p:Platform=x64 `
        /p:BuildProjectReferences=true

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM mcr.microsoft.com/windows/servercore:ltsc2022

WORKDIR C:\server

COPY --from=build C:\src\x64\Release\StarstrikeServer.exe .
COPY --from=build `
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.*\x64\Microsoft.VC143.CRT\msvcp140.dll" .
COPY --from=build `
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.*\x64\Microsoft.VC143.CRT\vcruntime140.dll" .
COPY --from=build `
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.*\x64\Microsoft.VC143.CRT\vcruntime140_1.dll" .

COPY StarstrikeServer\prefs.txt .

EXPOSE 4000/udp

ENTRYPOINT ["StarstrikeServer.exe"]
```

> The MSVC redist path contains a version-stamped subdirectory (e.g.
> `14.38.33130`).  Use `Get-ChildItem` in a RUN step during the build stage
> to resolve the exact path, or pin the toolset version in the vcxproj to
> produce a stable path.

---

## 10. Implementation Order

1. **Modify `NeuronServer/NeuronServer.h`** — replace `#include "NeuronCore.h"` with the curated WinRT-free content (section 4, Fix A).  This is the single change with the widest impact.

2. **Add `#ifdef SERVER_BUILD` guards** in the five locations across `server.cpp`, `servertoclient.cpp`, and `preferences.cpp` (section 5).

3. **Guard `g_prefsManager` definition** in `preferences.cpp` (section 7, end).

4. **Guard dead includes in `networkupdate.h`** (section 5.4).

5. **Update `NeuronServer.vcxproj`** — add `<ClCompile>` and `<ClInclude>` entries and update include directories (section 6).

6. **Create `StarstrikeServer` project** — `vcxproj`, `pch.h`, `pch.cpp`, `main.cpp`, `prefs.txt` (section 7).

7. **Update `Starstrike.slnx`** — add `StarstrikeServer` project entry.

8. **Verify build** — `msbuild StarstrikeServer\StarstrikeServer.vcxproj /p:Configuration=Release /p:Platform=x64`.  Use `dumpbin /imports StarstrikeServer.exe` and confirm no `Windows.Foundation.dll` or other WinRT DLLs appear in the import table.

9. **Clean up client** — remove `m_server` from `GameApp.h`, remove server init/advance/cleanup from `main.cpp`, remove bypass branch from `clienttoserver.cpp` (section 8).

10. **Build Docker image** and verify the container starts and binds port 4000.

---

## 11. Dependency Graph (After Separation)

```
StarstrikeServer.exe
  └── NeuronServer.lib  [SERVER_BUILD PCH — zero WinRT imports]
        ├── server.cpp / servertoclient.cpp
        ├── servertoclientletter.cpp / networkupdate.cpp
        ├── preferences.cpp
        └── NeuronCore.lib  [individual net_*, math, debug headers — WinRT-free]
              ├── net_lib / net_socket / net_socket_listener
              ├── net_thread / net_mutex / net_udp_packet
              └── LegacyVector3, Debug, GameMath, llist, darray …

Starstrike.exe  [unchanged WinRT/DirectX client — no m_server]
  └── NeuronClient.lib  [WinRT PCH — fine for packaged app]
  └── GameLogic.lib
  └── NeuronCore.lib
```

---

## 12. File Change Summary

| Action | File | What changes |
|--------|------|-------------|
| **Edit** | `NeuronServer/NeuronServer.h` | Replace `#include "NeuronCore.h"` with curated WinRT-free header list; add `#define SERVER_BUILD` |
| **Edit** | `NeuronClient/server.cpp` | Guard `#include "GameApp.h"`, add `s_server` static, guard bypass paths |
| **Edit** | `NeuronClient/servertoclient.cpp` | Guard `#include "GameApp.h"`, guard bypass path in constructor |
| **Edit** | `NeuronClient/preferences.cpp` | Guard `#include "GameApp.h"` + UI headers, guard `g_app->m_resource` block, guard `g_prefsManager` definition |
| **Edit** | `NeuronClient/networkupdate.h` | Guard legacy `worldobject.h` / `entity.h` includes |
| **Edit** | `NeuronServer/NeuronServer.vcxproj` | Add `<ClCompile>` for 5 server source files; add include directories |
| **Create** | `StarstrikeServer/StarstrikeServer.vcxproj` | Console exe; refs NeuronServer + NeuronCore only |
| **Create** | `StarstrikeServer/pch.h` | `#include "NeuronServer.h"` |
| **Create** | `StarstrikeServer/pch.cpp` | `#include "pch.h"` |
| **Create** | `StarstrikeServer/main.cpp` | `PrefsManager` init, `Server` init, 10 Hz `Advance()` loop |
| **Create** | `StarstrikeServer/prefs.txt` | Server defaults |
| **Create** | `StarstrikeServer/Dockerfile` | Multi-stage Server Core build |
| **Create** | `docker-compose.yml` | Local test composition |
| **Edit** | `Starstrike.slnx` | Add `StarstrikeServer` project |
| **Edit** | `Starstrike/GameApp.h` | Remove `Server* m_server` |
| **Edit** | `Starstrike/main.cpp` | Remove server init / advance / cleanup |
| **Edit** | `NeuronClient/clienttoserver.cpp` | Remove bypass branch calling `m_server` |
