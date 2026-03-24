#include "pch.h"
#include "text_renderer.h"
#include "preferences.h"
#include "language_table.h"
#include "debugmenu.h"
#include "network_window.h"
#include "profilewindow.h"
#include "cheat_window.h"
#include "reallyquit_window.h"
#include "GameApp.h"
#include "location.h"
#include "tree.h"
#include "camera.h"
#include "renderer.h"

// ****************************************************************************
// Menu Buttons
// ****************************************************************************

#ifdef PROFILER_ENABLED
class ProfileButton : public DarwiniaButton
{
  public:
    void MouseUp() override { DebugKeyBindings::ProfileButton(); }
};
#endif // PROFILER_ENABLED

class NetworkButton : public DarwiniaButton
{
  public:
    void MouseUp() override { DebugKeyBindings::NetworkButton(); }
};

class DebugCameraButton : public DarwiniaButton
{
  public:
    void MouseUp() override { DebugKeyBindings::DebugCameraButton(); }
};

class FPSButton : public DarwiniaButton
{
  public:
    void MouseUp() override { DebugKeyBindings::FPSButton(); }
};

#ifdef CHEATMENU_ENABLED
class CheatButton : public DarwiniaButton
{
  public:
    void MouseUp() override { DebugKeyBindings::CheatButton(); }
};

class ExplodeAllBuildingsButton : public DarwiniaButton
{
  public:
    void MouseUp() override
    {
        if( g_context->m_location )
        {
            for( int i = 0; i < g_context->m_location->m_buildings.Size(); ++i )
            {
                if( g_context->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_context->m_location->m_buildings[i];
                    if( building->m_type != Building::TypeTree )
                    {
                        building->Destroy( 100.0f );
                    }
                }
            }
        }
    }
};

class IgniteAllTreesButton : public DarwiniaButton
{
  public:
    void MouseUp() override
    {
        if( g_context->m_location )
        {
            for( int i = 0; i < g_context->m_location->m_buildings.Size(); ++i )
            {
                if( g_context->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_context->m_location->m_buildings[i];
                    if( building->m_type == Building::TypeTree )
                    {
                        building->Damage( -999.0f );
                    }
                }
            }
        }
    }
};
#endif // CHEATMENU_ENABLED

DebugMenu::DebugMenu(const char* name)
  : DarwiniaWindow(name)
{
  m_x = 10;
  m_y = 20;
  m_w = 170;
  m_h = 75;
}

void DebugMenu::Advance() {}

void DebugMenu::Create()
{
  DarwiniaWindow::Create();

  int pitch = 18;
  int y = 5;

  DarwiniaButton* button;

#ifdef PROFILER_ENABLED
  button = new ProfileButton();
  button->SetShortProperties("Profile (F6)", 10, y += pitch, m_w - 20);
  RegisterButton(button);
#endif // PROFILER_ENABLED

  button = new NetworkButton();
  button->SetShortProperties("Network Stats", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  button = new FPSButton();
  button->SetShortProperties("Display FPS (F5)", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  y += pitch / 2.0f;

  y += pitch / 2.0f;

  button = new DebugCameraButton();
  button->SetShortProperties("Dbg Cam (F2)", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  y += pitch / 2.0f;

  bool modsEnabled = g_prefsManager->GetInt("ModSystemEnabled", 0) != 0;

#ifdef CHEATMENU_ENABLED
  button = new CheatButton();
  button->SetShortProperties("Cheat Menu (F4)", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  button = new ExplodeAllBuildingsButton();
  button->SetShortProperties("Explode Buildings", 10, y += pitch, m_w - 20);
  RegisterButton(button);

  button = new IgniteAllTreesButton();
  button->SetShortProperties("Ignite All Trees", 10, y += pitch, m_w - 20);
  RegisterButton(button);
#endif

  y += pitch / 2.0f;
}

void DebugMenu::Render(bool hasFocus)
{
  Advance();

  DarwiniaWindow::Render(hasFocus);

  EclButton* camDbgButton = GetButton("Dbg Cam (F2)");
  DEBUG_ASSERT(camDbgButton);
  int y = m_y + camDbgButton->m_y + 11;

  switch (g_context->m_camera->GetDebugMode())
  {
  case Camera::DebugModeAlways:
    g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Always");
    break;
  case Camera::DebugModeAuto:
    g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Auto");
    break;
  case Camera::DebugModeNever:
    g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Never");
    break;
  }
}

// ****************************************************************************
// Class DebugKeyBindings
// ****************************************************************************

void DebugKeyBindings::DebugMenu()
{
  char* debugMenuWindowName = LANGUAGEPHRASE("dialog_toolsmenu");
  if (EclGetWindow(debugMenuWindowName))
    EclRemoveWindow(debugMenuWindowName);
  else
    EclRegisterWindow(new ::DebugMenu(debugMenuWindowName));
}

#ifdef PROFILER_ENABLED
void DebugKeyBindings::ProfileButton()
{
  if (EclGetWindow("Profiler"))
    EclRemoveWindow("Profiler");
  else
  {
    auto pw = new ProfileWindow("Profiler");
    pw->m_w = 570;
    pw->m_h = 450;
    pw->m_x = g_context->m_renderer->ScreenW() - pw->m_w - 20;
    pw->m_y = 30;
    EclRegisterWindow(pw);
  }
}
#endif

void DebugKeyBindings::NetworkButton()
{
  if (!EclGetWindow("Network Stats"))
  {
    auto nw = new NetworkWindow("Network Stats");
    nw->m_w = 200;
    nw->m_h = 200;
    nw->m_x = 10;
    nw->m_y = g_context->m_renderer->ScreenH() - nw->m_h;
    EclRegisterWindow(nw);
  }
}

void DebugKeyBindings::DebugCameraButton() { g_context->m_camera->SetNextDebugMode(); }

void DebugKeyBindings::FPSButton() { g_context->m_renderer->m_displayFPS = !g_context->m_renderer->m_displayFPS; }

#ifdef CHEATMENU_ENABLED
void DebugKeyBindings::CheatButton()
{
  if (!EclGetWindow("Cheat Window"))
  {
    auto window = new CheatWindow("Cheat Window");
    window->m_w = 200;
    window->m_h = 200;
    window->m_x = 250;
    window->m_y = 50;
    EclRegisterWindow(window);
  }
}
#endif

void DebugKeyBindings::ReallyQuitButton()
{
  // Bring up a really quit window
  if (!EclGetWindow(REALLYQUIT_WINDOWNAME))
    EclRegisterWindow(new ReallyQuitWindow());
}
