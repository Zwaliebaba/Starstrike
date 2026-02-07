#pragma once

#include <dxgi1_6.h>

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")


#define IID_GRAPHICS_PPV_ARGS(ppType)       __uuidof(ppType), (ppType).put_void()

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

consteval uint32_t FIXED_MSG(const char* _msg)
{
  return static_cast<uint32_t>(_msg[0]) << 24 | static_cast<uint32_t>(_msg[1]) << 16 |
    static_cast<uint32_t>(_msg[2]) << 8 | static_cast<uint32_t>(_msg[3]);
}

using namespace DirectX;

//********************************************************
#include <d3d9.h>
#include <C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include/d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/X64/d3dx9.lib")

//********************************************************

