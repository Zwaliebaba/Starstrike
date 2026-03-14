# Server Separation Plan

## Executive Summary

The blocker for extracting the server into a standalone executable is not
WinRT.  The C++/WinRT headers in `NeuronCore.h` are compile-time template
definitions only — they produce no DLL imports unless a WinRT type is
actually instantiated in code.  The server never instantiates any WinRT
type, so the existing PCH chain is fine and needs no changes.

The real issue is that `server.cpp`, `servertoclient.cpp`, and
`preferences.cpp` include `GameApp.h` and reference `g_app->` for one
reason only: **bypass mode**.  Bypass mode is an in-process shortcut where,
when server and client run in the same process, messages are delivered
directly via object pointers rather than through loopback UDP.

This is a design smell: the `Server` class should not know about the
client's networking mode.  Removing bypass mode from the server files is the
correct fix.  No preprocessor flag (`SERVER_BUILD` or similar) is needed —
the changes are unconditional and leave both the client and server builds
cleaner.

The integrated server + client mode (single-player / LAN host) continues to
work via loopback networking (`127.0.0.1`) at negligible overhead cost.

---

## 1. Root Cause

### 1.1 Why NeuronCore.h's WinRT includes are not the problem

`NeuronCore/NeuronCore.h` includes six C++/WinRT headers and
`using namespace winrt`.  These are compile-time template definitions only.
C++/WinRT is a header-only library: the `#include <winrt/…>` directives
bring in type aliases, concept constraints, and template specialisations —
none of which produce object code or DLL import records unless a WinRT type
is instantiated in a translation unit.

Because the server code never creates a `winrt::` object or calls a WinRT
factory, the linker emits zero WinRT DLL import entries.  The existing
NeuronServer PCH chain is therefore unaffected and does not need to change.

### 1.2 The actual coupling: bypass mode in the Server class

Three server source files include `GameApp.h` and access `g_app->`:

| File | `g_app->` expression | Reason it exists |
|------|----------------------|-----------------|
| `NeuronClient/server.cpp:40` | `g_app->m_server` | Route UDP packet to this server in `ListenCallback` |
| `NeuronClient/server.cpp:86` | `g_app->m_bypassNetworking` | Skip real UDP setup when running in-process |
| `NeuronClient/server.cpp:288–289` | `g_app->m_bypassNetworking`, `g_app->m_clientToServer->ReceiveLetter()` | Deliver outbound letter directly to client object |
| `NeuronClient/servertoclient.cpp:13` | `g_app->m_bypassNetworking` | Skip socket creation when running in-process |
| `NeuronClient/preferences.cpp:308` | `g_app->m_resource` | Load defaults from the game's resource system |

Every one of these is either a bypass-mode shortcut or a client-only
resource feature.  None of them belong in the Server class.

---

## 2. What the Server Actually Needs

Looking at `server.cpp` without bypass mode:

- **`ListenCallback`** — receives a UDP packet and routes it to the server
  inbox.  The only reason it accesses `g_app->m_server` is that `Server`
  exposes no self-registration mechanism.  Replace with a module-static
  `static Server* s_server` that is set in `Initialise()`.

- **`Initialise()`** — the `if (!g_app->m_bypassNetworking)` guard exists
  only to skip real networking in bypass mode.  A dedicated server always
  uses real networking.  Remove the guard and always initialise.

- **`AdvanceSender()`** — the `if (g_app->m_bypassNetworking)` branch
  delivers outbound letters directly to `g_app->m_clientToServer`.  This is
  client knowledge inside the server.  Remove the branch; always use the
  real socket path that already exists in the `else` block.

After these three changes `server.cpp` has zero `g_app->` references and
`#include "GameApp.h"` can be deleted outright.

The same logic applies to `servertoclient.cpp` (one bypass guard) and
`preferences.cpp` (one resource-system block).

---

## 3. Transitive Type Dependencies from networkupdate.h

`networkupdate.h` includes `worldobject.h`, `entity.h`, and `team.h`.

```
networkupdate.h
  ├── worldobject.h   → rgb_colour.h (clean), LegacyVector3.h (clean)
  ├── entity.h        → llist.h (clean), texture_uv.h (clean), worldobject.h
  └── team.h          → fast_darray.h (clean), slice_darray.h (clean),
                         rgb_colour.h, worldobject.h, entity.h
```

None of these headers contain WinRT.  They are plain C++ data-class headers
and are safe to include in the server build.

`worldobject.h` and `entity.h` are legacy dead-includes in `networkupdate.h`
— the `NetworkUpdate` struct has no `WorldObject` or `Entity` fields.  Only
`TeamControls` (from `team.h`) and `LegacyVector3` are actually used.
Removing those two dead includes from `networkupdate.h` shrinks the
dependency graph and is good hygiene regardless.

---

## 4. Exact Code Changes — Unconditional

No preprocessor flags.  All changes apply to the shared source files and
improve both the client and server builds.

### 4.1 `NeuronClient/server.cpp`

**Remove** `#include "GameApp.h"` (line 3) and `#include "clienttoserver.h"` (line 4).

**Replace** `ListenCallback` (lines 32–51):

```cpp
// Add before ListenCallback:
static Server* s_server = nullptr;

static NetCallBackRetType ListenCallback(NetUdpPacket* udpdata)
{
  if (udpdata)
  {
    NetIpAddress* fromAddr = &udpdata->m_clientAddress;
    char newip[16];
    IpToString(fromAddr->sin_addr, newip);

    if (s_server)
    {
      auto letter = new NetworkUpdate(udpdata->m_data);
      s_server->ReceiveLetter(letter, newip);
    }

    delete udpdata;
  }
  return 0;
}
```

**Replace** `Server::Initialise()` (lines 81–93) — remove bypass guard,
always initialise real networking, register `s_server`:

```cpp
void Server::Initialise()
{
  m_inboxMutex  = new NetMutex();
  m_outboxMutex = new NetMutex();

  s_server = this;
  m_netLib = new NetLib();
  m_netLib->Initialise();
  NetStartThread(ListenThread);
}
```

**Replace** the dispatch block inside `Server::AdvanceSender()` (lines 286–299)
— remove the bypass branch, keep only the real-socket path:

```cpp
// Before (inside while loop):
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

// After:
int linearSize = 0;
ServerToClient* client = m_clients[letter->GetClientId()];
NetSocket* socket = client->GetSocket();
char* linearisedLetter = letter->GetByteStream(&linearSize);
socket->WriteData(linearisedLetter, linearSize);
bytesSentThisFrame += linearSize;
delete letter;
```

### 4.2 `NeuronClient/servertoclient.cpp`

**Remove** `#include "GameApp.h"` (line 4).

**Replace** the constructor (lines 8–21) — remove bypass guard, always
create a real socket:

```cpp
ServerToClient::ServerToClient(char* _ip)
  : m_socket(nullptr)
{
  strcpy(m_ip, _ip);

  m_socket = new NetSocket();
  NetRetCode retCode = m_socket->Connect(_ip, 4001);
  DEBUG_ASSERT(retCode == NetOk);

  m_lastKnownSequenceId = -1;
}
```

### 4.3 `NeuronClient/preferences.cpp`

**Remove** `#include "GameApp.h"` (line 2), `#include "resource.h"` (line 5),
`#include "text_stream_readers.h"` (line 6), and `#include "prefs_other_window.h"` (line 7).

**Remove** the resource-system default loading block in `CreateDefaultValues()`
(lines 307–312) — the server reads a plain text file; the client can load
its defaults through a separate mechanism or always rely on the prefs file:

```cpp
// Remove entirely:
if (g_app && g_app->m_resource)
{
  TextReader* reader = g_app->m_resource->GetTextReader("default_preferences.txt");
  if (reader && reader->IsOpen()) { while (reader->ReadLine()) { AddLine(reader->GetRestOfLine(), true); } }
}
```

The `g_prefsManager` global definition at line 9 stays exactly as-is — no
guarding required.  NeuronServer.lib will contain the definition when
`preferences.cpp` is compiled into it, and `StarstrikeServer/main.cpp`
simply assigns to it rather than redefining it.

### 4.4 `NeuronClient/networkupdate.h`

**Remove** the two dead includes (no `WorldObject` or `Entity` fields exist
in `NetworkUpdate`):

```cpp
// Remove:
#include "worldobject.h"
#include "entity.h"
// Keep:
#include "team.h"     // TeamControls is an actual field
```

---

## 5. Files to Add to `NeuronServer.vcxproj`

Add the following as `<ClCompile>` items:

```xml
<ClCompile Include="..\NeuronClient\server.cpp" />
<ClCompile Include="..\NeuronClient\servertoclient.cpp" />
<ClCompile Include="..\NeuronClient\servertoclientletter.cpp" />
<ClCompile Include="..\NeuronClient\networkupdate.cpp" />
<ClCompile Include="..\NeuronClient\preferences.cpp" />
```

Header references for IDE navigation:

```xml
<ClInclude Include="..\NeuronClient\server.h" />
<ClInclude Include="..\NeuronClient\servertoclient.h" />
<ClInclude Include="..\NeuronClient\servertoclientletter.h" />
<ClInclude Include="..\NeuronClient\networkupdate.h" />
<ClInclude Include="..\NeuronClient\preferences.h" />
```

**`<AdditionalIncludeDirectories>`** must include:

```
$(SolutionDir)NeuronCore;
$(SolutionDir)NeuronClient;
$(SolutionDir)GameLogic;
$(SolutionDir)Starstrike;
```

The GameLogic and Starstrike paths are needed for `team.h`, `worldobject.h`,
and `entity.h`, which are reachable through `networkupdate.h` → `team.h`.

Do NOT add a project reference from `NeuronServer` to `NeuronClient`.  The
files are compiled directly into NeuronServer with NeuronServer's own PCH.

---

## 6. Create `StarstrikeServer` Executable Project

### `StarstrikeServer/StarstrikeServer.vcxproj`

| Setting | Value |
|---------|-------|
| Output type | Application (`.exe`) |
| Platform toolset | v145 |
| C++ standard | C++20 |
| Subsystem | `/SUBSYSTEM:CONSOLE` |
| No packaging | headless — no MSIX |

**Project references:**
- `NeuronServer`
- `NeuronCore`

**Do NOT reference:** `NeuronClient`, `GameLogic`, `Starstrike`.

### `StarstrikeServer/pch.h`

```cpp
#pragma once
#include "NeuronServer.h"
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

// g_prefsManager is defined in preferences.cpp (compiled via NeuronServer.lib)

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
    server.Initialise();   // binds UDP 4000, starts listener thread

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

`g_prefsManager` is defined in `preferences.cpp` (compiled into
`NeuronServer.lib`).  `main.cpp` assigns to it — no redefinition.

### `StarstrikeServer/prefs.txt`

```
IAmAServer 1
BypassNetwork 0
RecordDemo 0
```

---

## 7. Client-Side Cleanup (Starstrike.exe)

**`Starstrike/GameApp.h`**
- Remove `Server* m_server;` (line 44) and the `class Server;` forward declaration.

**`Starstrike/main.cpp`**
- Remove `IAmAServer` init block (lines 541–548): `new Server()` + `Initialise()`
- Remove server advance block (lines 229–237): `m_server->Advance()`
- Remove cleanup (lines 403–404): `delete g_app->m_server`

**`NeuronClient/clienttoserver.cpp`**
- Remove the bypass branch (lines 101–102) that called `g_app->m_server->ReceiveLetter(...)`.  The client now always sends UDP to the server socket; loopback handles the in-process case transparently.

---

## 8. Windows Container

### Why Server Core is correct

Windows Server Core (`mcr.microsoft.com/windows/servercore:ltsc2022`):
- Provides Win32 API, WinSock2, `Ws2_32.dll`, `kernel32.dll`
- No graphics stack, no WinRT runtime

Because the server never instantiates a WinRT type, the binary has no WinRT
DLL entries in its import table even though `NeuronCore.h` pulls in the
C++/WinRT headers.  Confirm after build with:

```
dumpbin /imports StarstrikeServer.exe
```

No `Windows.Foundation.dll` or equivalent should appear.

The MSVC C++ runtime (`vcruntime140.dll`, `msvcp140.dll`, `vcruntime140_1.dll`)
must be shipped alongside the binary as Server Core does not include it.

### `StarstrikeServer/Dockerfile`

```dockerfile
# ── Build stage ──────────────────────────────────────────────────────────────
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS build

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop';"]

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

> Pin the MSVC toolset version in the vcxproj (e.g. `14.38.33130`) to
> produce a stable, predictable redist path in the Dockerfile.

---

## 9. Implementation Order

1. **Edit `NeuronClient/server.cpp`** — remove `GameApp.h` / `clienttoserver.h` includes, replace `g_app->m_server` with `s_server`, remove bypass guards from `Initialise()` and `AdvanceSender()`.

2. **Edit `NeuronClient/servertoclient.cpp`** — remove `GameApp.h` include, remove bypass guard from constructor.

3. **Edit `NeuronClient/preferences.cpp`** — remove `GameApp.h` and UI includes, remove `g_app->m_resource` block.

4. **Edit `NeuronClient/networkupdate.h`** — remove dead `worldobject.h` and `entity.h` includes.

5. **Update `NeuronServer.vcxproj`** — add `<ClCompile>` entries for the five server source files, update include directories.

6. **Create `StarstrikeServer` project** — `vcxproj`, `pch.h`, `pch.cpp`, `main.cpp`, `prefs.txt`.

7. **Update `Starstrike.slnx`** — add `StarstrikeServer`.

8. **Build and verify** — `msbuild StarstrikeServer\StarstrikeServer.vcxproj /p:Configuration=Release /p:Platform=x64`, then `dumpbin /imports` to confirm no WinRT DLL entries.

9. **Clean up client** — remove `m_server` from `GameApp.h`, remove server init/advance/cleanup from `main.cpp`, remove bypass branch from `clienttoserver.cpp`.

10. **Build Docker image** — verify the container starts and listens on UDP 4000.

---

## 10. Dependency Graph (After Separation)

```
StarstrikeServer.exe
  └── NeuronServer.lib   [server.cpp, servertoclient.cpp, servertoclientletter.cpp,
  |                       networkupdate.cpp, preferences.cpp — no GameApp dependency]
  └── NeuronCore.lib     [net_*, math, debug — winrt/ headers present but unused]

Starstrike.exe  [client, m_server removed, bypass mode removed]
  └── NeuronClient.lib
  └── GameLogic.lib
  └── NeuronCore.lib
```

---

## 11. File Change Summary

| Action | File | What changes |
|--------|------|-------------|
| **Edit** | `NeuronClient/server.cpp` | Remove `GameApp.h`; add `s_server` static; remove bypass paths from `Initialise()` and `AdvanceSender()` |
| **Edit** | `NeuronClient/servertoclient.cpp` | Remove `GameApp.h`; always create socket in constructor |
| **Edit** | `NeuronClient/preferences.cpp` | Remove `GameApp.h` and UI includes; remove `g_app->m_resource` block |
| **Edit** | `NeuronClient/networkupdate.h` | Remove dead `worldobject.h` / `entity.h` includes |
| **Edit** | `NeuronServer/NeuronServer.vcxproj` | Add `<ClCompile>` for 5 server source files; update include directories |
| **Create** | `StarstrikeServer/StarstrikeServer.vcxproj` | Console exe; refs NeuronServer + NeuronCore |
| **Create** | `StarstrikeServer/pch.h` | `#include "NeuronServer.h"` |
| **Create** | `StarstrikeServer/pch.cpp` | `#include "pch.h"` |
| **Create** | `StarstrikeServer/main.cpp` | `g_prefsManager` init, `Server` init, 10 Hz `Advance()` loop |
| **Create** | `StarstrikeServer/prefs.txt` | Server defaults |
| **Create** | `StarstrikeServer/Dockerfile` | Multi-stage Server Core build |
| **Create** | `docker-compose.yml` | Local test composition |
| **Edit** | `Starstrike.slnx` | Add `StarstrikeServer` project |
| **Edit** | `Starstrike/GameApp.h` | Remove `Server* m_server` |
| **Edit** | `Starstrike/main.cpp` | Remove server init / advance / cleanup |
| **Edit** | `NeuronClient/clienttoserver.cpp` | Remove bypass branch calling `m_server` |
