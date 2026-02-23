#ifndef _included_debugmenu_h
#define _included_debugmenu_h

#include "darwinia_window.h"
#include "GameApp.h"


class DebugMenu : public DarwiniaWindow
{
public:
    DebugMenu( char *name );

	void Advance();
    void Create();
	void Render(bool hasFocus);
};


class DebugKeyBindings
{
public:
	static void DebugMenu();
	static void NetworkButton();
	static void DebugCameraButton();
	static void FollowCameraButton();
	static void FPSButton();
	static void InputLogButton();
#ifdef PROFILER_ENABLED
	static void ProfileButton();
#endif
#ifdef CHEATMENU_ENABLED
    static void CheatButton();
#endif
	static void ReallyQuitButton();
};


#endif
