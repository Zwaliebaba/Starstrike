#ifndef Types_h
#define Types_h

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#define STRICT 1

#include <windowsx.h>

// Enable extra D3D debugging in debug builds if using the debug DirectX runtime.  
// This makes D3D objects work well in the debugger watch window, but slows down 
// performance slightly.
#if defined(DEBUG) | defined(_DEBUG)
#define D3D_DEBUG_INFO
#endif

// Direct3D includes
#include <C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include/d3dx9.h>
#include <d3d9.h>

// DirectSound includes
#include <mmreg.h>
#include <mmsystem.h>

#include <dsound.h>

#endif Types_h
