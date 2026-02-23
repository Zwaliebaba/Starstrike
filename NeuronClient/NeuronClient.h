#pragma once

#include "NeuronCore.h"

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Input.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.Core.h>
#include <winrt/Windows.UI.Popups.h>

using namespace winrt;

#include "DirectXHelper.h"
#include "GraphicsCore.h"
#include "PixProfiler.h"
#include "Strings.h"

#include "GameMain.h"

namespace Neuron::Client
{
    class ClientEngine
    {
    public:
        static void Startup(const wchar_t* _gameName, HINSTANCE _hInstance, int nCmdShow);
        static void StartGame(const com_ptr<GameMain>& _gameMain);
        static void Shutdown();

        static void Run();

        static void OnDeviceLost();
        static void OnDeviceRestored();

        static HINSTANCE Instance() {
            return m_instance;
        }
        static HWND Window() {
            return m_hwnd;
        }
        static Windows::Foundation::Size OutputSize() {
            return m_outputSize;
        }
        static Windows::Foundation::Point OutputTopLeft();

        static void RegisterDeviceNotify(IDeviceNotify* deviceNotify)
        {
            m_deviceNotify = deviceNotify;

            // On RS4 and higher, applications that handle device removal
            // should declare themselves as being able to do so
            __if_exists(DXGIDeclareAdapterRemovalSupport)
            {
                if (deviceNotify)
                    if (FAILED(DXGIDeclareAdapterRemovalSupport()))
                        OutputDebugString(L"Warning: application failed to declare adapter removal support.\n");
            }
        }

    protected:
        inline static IDeviceNotify* m_deviceNotify{};
        inline static HINSTANCE m_instance;
        inline static HWND m_hwnd;
        inline static Windows::Foundation::Size m_outputSize;
        inline static com_ptr<GameMain> m_main;

        inline static bool sm_resourcesLoaded = { false };
    };

} // namespace Neuron::Client

using namespace Neuron::Client;

#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")

#ifndef INCLUDED_UNIVERSAL_INCLUDE_H
#define INCLUDED_UNIVERSAL_INCLUDE_H

#define DARWINIA_VERSION "1.5.11"
#define DARWINIA_EXE_VERSION 1,5,11,0
#define STR_DARWINIA_EXE_VERSION "1, 5, 11, 0\0"

// === PICK ONE OF THESE TARGETS ===

//#define TARGET_FULLGAME
//#define TARGET_DEMOGAME
//#define TARGET_PURITYCONTROL
//#define TARGET_DEMO2
#define TARGET_DEBUG
//#define TARGET_VISTA
//#define TARGET_VISTA_DEMO2

// === PICK ONE OF THESE TARGETS ===

#define DEBUG_RENDER_ENABLED

//#define USE_CRASHREPORTING

#ifndef _OPENMP
#define PROFILER_ENABLED
#endif

#ifdef TARGET_DEBUG
#define DARWINIA_GAMETYPE "debug"
#define CHEATMENU_ENABLED
#endif

#if !defined(TARGET_DEBUG) &&       \
    !defined(TARGET_FULLGAME) &&    \
    !defined(TARGET_DEMOGAME) &&    \
    !defined(TARGET_DEMO2) &&       \
    !defined(TARGET_PURITYCONTROL) && \
    !defined(TARGET_VISTA) && \
    !defined(TARGET_VISTA_DEMO2 )
#error "Unknown target, cannot determine game type"
#endif

#define DARWINIA_VERSION_STRING DARWINIA_GAMETYPE "-" DARWINIA_VERSION

#ifdef TARGET_MSVC

// Visual studio 2005 insists that we use the underscored versions
#define strupr _strupr
#define strnicmp _strnicmp
#define strlwr _strlwr
#define strdup _strdup
#define itoa _itoa

#define HAVE_DSOUND
#endif

#include "opengl_directx.h"

#define SAFE_FREE(x) {free(x);x=NULL;}
#define SAFE_DELETE(x) {delete x;x=NULL;}
#define SAFE_DELETE_ARRAY(x) {delete[] x;x=NULL;}
#define SAFE_RELEASE(x) {if(x){(x)->Release();x=NULL;}}

#endif
