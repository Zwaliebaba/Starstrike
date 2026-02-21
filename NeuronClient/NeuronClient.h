#pragma once

#include "NeuronCore.h"

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

#ifdef TARGET_VISTA
#define DARWINIA_GAMETYPE "vista"
#define LOCATION_EDITOR
#define TARGET_OS_VISTA
#define ATTRACTMODE_ENABLED
#endif

#ifdef TARGET_FULLGAME
#define DARWINIA_GAMETYPE "full"
#endif

#ifdef TARGET_FULLGAME_FRENCH
#define DARWINIA_GAMETYPE "full"
#define LOCATION_EDITOR
#define FRENCH
#endif

#ifdef TARGET_DEMOGAME
#define DARWINIA_GAMETYPE "demo"
#define DEMOBUILD
#endif

#ifdef TARGET_DEMO2
#define DARWINIA_GAMETYPE "demo2"
#define DEMOBUILD
#define DEMO2
#endif

#ifdef TARGET_VISTA_DEMO2
#define DARWINIA_GAMETYPE "vista-demo2"
#define DEMOBUILD
#define DEMO2
#define TARGET_OS_VISTA
#endif

#ifdef TARGET_PURITYCONTROL
#define DARWINIA_GAMETYPE "full"
#define PURITY_CONTROL
#endif

#ifdef TARGET_DEBUG
#define DARWINIA_GAMETYPE "debug"
#define CHEATMENU_ENABLED
#define D3D_DEBUG_INFO
#endif

//#define PROMOTIONAL_BUILD                         // Their company logo is shown on screen

#if !defined(TARGET_DEBUG) &&       \
    !defined(TARGET_FULLGAME) &&    \
    !defined(TARGET_DEMOGAME) &&    \
    !defined(TARGET_DEMO2) &&       \
    !defined(TARGET_PURITYCONTROL) && \
    !defined(TARGET_VISTA) && \
    !defined(TARGET_VISTA_DEMO2 )
#error "Unknown target, cannot determine game type"
#endif

#ifndef PROFILER_ENABLED
#define DARWINIA_VERSION_PROFILER "-np"
#else
#define DARWINIA_VERSION_PROFILER ""
#endif

#define DARWINIA_VERSION_STRING DARWINIA_PLATFORM "-" DARWINIA_GAMETYPE "-" DARWINIA_VERSION DARWINIA_VERSION_PROFILER

#ifdef TARGET_MSVC

#define snprintf _snprintf

// Visual studio 2005 insists that we use the underscored versions
#include <string.h>
#define stricmp _stricmp
#define strupr _strupr
#define strnicmp _strnicmp
#define strlwr _strlwr
#define strdup _strdup

#include <stdlib.h>
#define itoa _itoa

#define DARWINIA_PLATFORM "win32"

#define HAVE_DSOUND
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#define SAFE_FREE(x) {free(x);x=NULL;}
#define SAFE_DELETE(x) {delete x;x=NULL;}
#define SAFE_DELETE_ARRAY(x) {delete[] x;x=NULL;}
#define SAFE_RELEASE(x) {if(x){(x)->Release();x=NULL;}}

#endif
