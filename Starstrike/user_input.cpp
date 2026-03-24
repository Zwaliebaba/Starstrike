#include "pch.h"
#include "eclipse.h"
#include "input.h"
#include "targetcursor.h"
#include "math_utils.h"
#include "profiler.h"
#include "ShapeStatic.h"
#include "text_renderer.h"
#include "language_table.h"
#include "debugmenu.h"
#include "GameApp.h"
#include "camera.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "user_input.h"
#include "keydefs.h"

// *** Constructor
UserInput::UserInput()
  : m_removeTopLevelMenu(false)
{
  EclInitialise(g_context->m_renderer->ScreenW(), g_context->m_renderer->ScreenH());
}

extern signed char g_keyDeltas[KEY_MAX];

// *** AdvanceMenus
void UserInput::AdvanceMenus()
{
  	if ( g_keyDeltas[KEY_F1] )
  		DebugKeyBindings::DebugMenu();

  InputManager* im = g_inputManager;
  int mouseX = g_target->X();
  int mouseY = g_target->Y();
  bool lmb = im->controlEvent(ControlEclipseLMousePressed);
  bool rmb = im->controlEvent(ControlEclipseRMousePressed);

  EclUpdateMouse(mouseX, mouseY, lmb, rmb);
  EclUpdate();

  if (im->controlEvent(ControlEclipseLMouseDown))
  {
    EclWindow* winUnderMouse = EclGetWindow(mouseX, mouseY);
    if (winUnderMouse)
      im->suppressEvent(ControlEclipseLMouseDown);
  }
}

// *** Advance
void UserInput::Advance()
{
  START_PROFILE(g_context->m_profiler, "Advance UserInput");

  g_inputManager->Advance();

  if (m_removeTopLevelMenu)
  {
    EclWindow* win = EclGetWindow(LANGUAGEPHRASE("dialog_toolsmenu"));
    if (win)
      EclRemoveWindow(win->m_name);
    m_removeTopLevelMenu = false;
  }

  AdvanceMenus();

#ifdef CHEATMENU_ENABLED
  if (g_inputManager->controlEvent(ControlToggleCheatMenu))
    DebugKeyBindings::CheatButton();
#endif

  END_PROFILE(g_context->m_profiler, "Advance UserInput");
}

// *** Render
void UserInput::Render()
{
  START_PROFILE(g_context->m_profiler, "Render UserInput");

  EclRender();

  END_PROFILE(g_context->m_profiler, "Render UserInput");
}

// *** GetMousePos3d
LegacyVector3 UserInput::GetMousePos3d() { return m_mousePos3d; }

// *** RecalcMousePos3d
void UserInput::RecalcMousePos3d()
{
  // Get click ray
  LegacyVector3 rayStart;
  LegacyVector3 rayDir;
  g_context->m_camera->GetClickRay(g_target->X(), g_target->Y(), &rayStart, &rayDir);
  ASSERT_VECTOR3_IS_SANE(rayStart);
  ASSERT_VECTOR3_IS_SANE(rayDir);
  rayStart += rayDir * 0.0f;

  bool landscapeHit = false;
  if (g_context->m_location)
    landscapeHit = g_context->m_location->m_landscape.RayHit(rayStart, rayDir, &m_mousePos3d);
  else
  {
    // We are in the global world
    // So hit against the outer sphere

    LegacyVector3 sphereCenter(0, 0, 0);
    float sphereRadius = 36000.0f;

    rayStart += rayDir * (sphereRadius * 4.0f);
    rayDir = -rayDir;
    landscapeHit = RaySphereIntersection(rayStart, rayDir, sphereCenter, sphereRadius, 1e10, &m_mousePos3d);
    return;
  }

  if (!landscapeHit)
  {
    // OK, we didn't hit against the landscape mesh, so hit against a sphere that
    // encloses the whole world
    LegacyVector3 sphereCenter;
    sphereCenter.x = g_context->m_globalWorld->GetSize() * 0.5f;
    sphereCenter.y = 0.0f;
    sphereCenter.z = g_context->m_globalWorld->GetSize() * 0.5f;

    float sphereRadius = g_context->m_globalWorld->GetSize() * 40.0f;

    rayStart += rayDir * (sphereRadius * 4.0f);
    rayDir = -rayDir;
    landscapeHit = RaySphereIntersection(rayStart, rayDir, sphereCenter, sphereRadius, 1e10, &m_mousePos3d);
  }
}
