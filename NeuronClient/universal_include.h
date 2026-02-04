#ifndef INCLUDED_UNIVERSAL_INCLUDE_H
#define INCLUDED_UNIVERSAL_INCLUDE_H

#define DARWINIA_VERSION "1.5.11"
#define DARWINIA_EXE_VERSION 1,5,11,0
#define STR_DARWINIA_EXE_VERSION "1, 5, 11, 0\0"

#define DEBUG_RENDER_ENABLED

#define DARWINIA_GAMETYPE "debug"
#define CHEATMENU_ENABLED
#define D3D_DEBUG_INFO
#define PROFILER_ENABLED

#define DARWINIA_VERSION_STRING DARWINIA_PLATFORM "-" DARWINIA_GAMETYPE "-" DARWINIA_VERSION

#ifdef TARGET_MSVC
#pragma warning( disable : 4244 4305 4800 4018 )

// Visual studio 2005 insists that we use the underscored versions
#define _stricmp _stricmp
#define strupr _strupr
#define _strnicmp _strnicmp
#define _strlwr _strlwr
#define _strdup _strdup

#define itoa _itoa

#define DARWINIA_PLATFORM "win32"

#define HAVE_DSOUND
#endif

#endif
