#pragma once

#include "darwinia_window.h"

class DebugMenu : public DarwiniaWindow
{
  public:
    DebugMenu(const char* name);

    void Advance();
    void Create() override;
    void Render(bool hasFocus) override;
};

class DebugKeyBindings
{
  public:
    static void DebugMenu();
    static void NetworkButton();
    static void DebugCameraButton();
    static void FPSButton();
#ifdef PROFILER_ENABLED
    static void ProfileButton();
#endif
#ifdef CHEATMENU_ENABLED
    static void CheatButton();
#endif
    static void ReallyQuitButton();
};
