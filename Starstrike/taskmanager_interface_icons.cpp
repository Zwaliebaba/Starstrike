#include "pch.h"
#include "text_renderer.h"
#include "math_utils.h"
#include "input.h"
#include "targetcursor.h"
#include "vector2.h"
#include "debug_utils.h"
#include "resource.h"
#include "bitmap.h"
#include "profiler.h"
#include "hi_res_time.h"
#include "language_table.h"
#include "preferences.h"
#include "eclipse.h"
#include "clienttoserver.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"
#include "app.h"
#include "gamecursor.h"
#include "global_world.h"
#include "location.h"
#include "renderer.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "taskmanager_interface_icons.h"
#include "team.h"
#include "unit.h"
#include "level_file.h"
#include "main.h"
#include "particle_system.h"
#include "camera.h"
#include "script.h"
#include "location_input.h"
#include "prefs_other_window.h"
#include "soundsystem.h"
#include "insertion_squad.h"
#include "researchitem.h"
#include "engineer.h"

// ============================================================================

TaskManagerInterfaceIcons::TaskManagerInterfaceIcons()
  : m_screenId(ScreenTaskManager),
    m_screenY(0.0f),
    m_desiredScreenY(0.0f),
    m_screenW(800),
    m_screenH(600),
    m_chatLogY(0.0f),
    m_currentScreenZone(-1),
    m_currentMouseScreenZone(-1),
    m_currentScrollZone(1),
    m_screenZoneTimer(0.0f),
    m_taskManagerDownTime(0.0),
    m_currentQuickUnit(-1),
    m_quickUnitDirection(0)
{
  m_viewingDefaultObjective = false;
  m_messageTimer = 0.0f;
  m_currentTaskType = -1;
  m_currentMessageType = -1;
  m_highlightedTaskId = -1;
  SetVisible(false);

  //
  // Pre-load all the graphics we intend to use for these screens
  // If we don't do this the interface screens are slow and sluggish first visit

  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    char iconFilename[256];
    sprintf(iconFilename, "icons/icon_%s.bmp", GlobalResearch::GetTypeName(i));
    if (g_app->m_resource->DoesTextureExist(iconFilename))
      unsigned int texId = g_app->m_resource->GetTexture(iconFilename, true, false);

    char gestureFilename[256];
    sprintf(gestureFilename, "icons/gesture_%s.bmp", GlobalResearch::GetTypeName(i));
    if (g_app->m_resource->DoesTextureExist(gestureFilename))
      unsigned int texId = g_app->m_resource->GetTexture(gestureFilename, true, false);
  }

  g_app->m_resource->GetTexture("textures/interface_grey.bmp", true, false);
  g_app->m_resource->GetTexture("textures/interface_red.bmp", true, false);
  g_app->m_resource->GetTexture("icons/gestureguide.bmp", true, false);
  g_app->m_resource->GetTexture("icons/icon_shadow.bmp", true, false);

  //
  // Create keyboard shortcuts

  m_keyboardShortcuts.PutData(new KeyboardShortcut("NewTask", GlobalResearch::TypeSquad, ControlIconsTaskManagerNewSquad));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("NewTask", GlobalResearch::TypeEngineer, ControlIconsTaskManagerNewEngineer));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("NewTask", GlobalResearch::TypeOfficer, ControlIconsTaskManagerNewOfficer));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("NewTask", GlobalResearch::TypeArmour, ControlIconsTaskManagerNewArmour));

  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeGrenade, ControlIconsTaskManagerSelectGrenade));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeRocket, ControlIconsTaskManagerSelectRocket));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeAirStrike, ControlIconsTaskManagerSelectAirStrike));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectWeapon", GlobalResearch::TypeController,
                                                   ControlIconsTaskManagerSelectController));

  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 0, ControlTaskManagerSelectTask1));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 1, ControlTaskManagerSelectTask2));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 2, ControlTaskManagerSelectTask3));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 3, ControlTaskManagerSelectTask4));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 4, ControlTaskManagerSelectTask5));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 5, ControlTaskManagerSelectTask6));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 6, ControlTaskManagerSelectTask7));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 7, ControlTaskManagerSelectTask8));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("SelectTask", 8, ControlTaskManagerSelectTask9));

  m_keyboardShortcuts.PutData(new KeyboardShortcut("DeleteTask", -1, ControlTaskManagerEndTask));

  m_keyboardShortcuts.PutData(new KeyboardShortcut("ScreenUp", -1, ControlUnitCycleLeft));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("ScreenDown", -1, ControlUnitCycleRight));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("ScreenUp", -1, ControlCameraForwards));
  m_keyboardShortcuts.PutData(new KeyboardShortcut("ScreenDown", -1, ControlCameraBackwards));
}

void TaskManagerInterfaceIcons::AdvanceTerminate()
{
  if (g_inputManager->controlEvent(ControlIconsTaskManagerEndTask))
  {
    g_app->m_taskManager->TerminateTask(g_app->m_taskManager->m_currentTaskId);
    g_app->m_taskManager->m_currentTaskId = -1;
  }
}

void TaskManagerInterfaceIcons::HideTaskManager()
{
  //
  // Put the mouse to the middle of the screen
  float midX = g_app->m_renderer->ScreenW() / 2.0f;
  float midY = g_app->m_renderer->ScreenH() / 2.0f;
  g_target->SetMousePos(midX, midY);
  g_app->m_camera->Advance();

  if (g_app->m_taskManager->m_tasks.Size() > 0)
    g_app->m_taskManager->SelectTask(g_app->m_taskManager->m_currentTaskId);

  m_screenY = 0.0f;
  m_desiredScreenY = 0.0f;
  m_screenId = ScreenTaskManager;
  SetVisible(false);

  g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Hide", SoundSourceBlueprint::TypeInterface);
}

void TaskManagerInterfaceIcons::Advance()
{
  if (m_lockTaskManager)
    return;

  AdvanceScrolling();
  AdvanceTab();

  if (m_viewingDefaultObjective)
  {
    // We were running a default objective description (trunk port, research item)
    // So shut it down now
    g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
    m_viewingDefaultObjective = false;
  }

  bool inCutscene = false;
  if (g_app->m_script->IsRunningScript() && g_app->m_script->m_permitEscape)
    inCutscene = true;
  if (g_app->m_camera->IsInMode(Camera::ModeBuildingFocus))
    inCutscene = true;

  if (inCutscene)
  {
    if (m_visible)
      HideTaskManager();
    return;
  }

  if (!m_visible && (g_inputManager->controlEvent(ControlIconsTaskManagerDisplay) || g_inputManager->controlEvent(
    ControlIconsTaskManagerDisplayDown)))
  {
    // Tab key just pressed
    // Pop up the task manager
    m_screenY = 0.0f;
    m_desiredScreenY = 0.0f;
    m_screenId = ScreenTaskManager;
    m_currentScrollZone = 1;

    SetVisible();

    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Show", SoundSourceBlueprint::TypeInterface);

    if (g_inputManager->controlEvent(ControlIconsTaskManagerDisplayDown))
    {
      // Controller activated the task manager, record the time
      m_taskManagerDownTime = GetHighResTime();
    }
  }
  else if (m_visible && g_inputManager->controlEvent(ControlIconsTaskManagerHide))
  {
    // Tab key pressed while visible
    // So remove the task manager
    HideTaskManager();
  }

  if (m_visible)
    AdvanceScreenEdges();
  AdvanceScreenZones();
  AdvanceKeyboardShortcuts();
  AdvanceTerminate();
  AdvanceQuickUnit();

  if (g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnit))
  {
    if (!m_quickUnitVisible)
      CreateQuickUnitInterface();
    else
      DestroyQuickUnitInterface();
  }
}

void TaskManagerInterfaceIcons::AdvanceScrolling()
{
  if (m_screenY != m_desiredScreenY)
  {
    float diff = g_advanceTime * 10;
    m_screenY = m_screenY * (1.0f - diff) + (m_desiredScreenY * diff);
    if (fabs(m_desiredScreenY - m_screenY) < 0.001f)
      m_screenY = m_desiredScreenY;
  }
}

void TaskManagerInterfaceIcons::AdvanceScreenEdges()
{
  bool scrollPermitted = fabs(m_desiredScreenY - m_screenY) == 0;

  int screenBorder = 10;

  bool scrollRequested = false;
  switch (m_screenId)
  {
  case ScreenTaskManager:
    if (scrollPermitted)
    {
      if (g_target->Y() < screenBorder)
      {
        m_screenId = ScreenResearch;
        m_desiredScreenY = -1.0f;
        scrollRequested = true;
      }
      if (g_target->Y() > g_app->m_renderer->ScreenH() - screenBorder)
      {
        m_screenId = ScreenObjectives;
        m_desiredScreenY = 1.0f;
        scrollRequested = true;
      }
    }
    break;

  case ScreenObjectives:
    if (scrollPermitted)
    {
      if (g_target->Y() < screenBorder)
      {
        m_screenId = ScreenTaskManager;
        m_desiredScreenY = 0.0f;
        scrollRequested = true;
      }
    }
    break;

  case ScreenResearch:
    if (scrollPermitted)
    {
      if (g_target->Y() > g_app->m_renderer->ScreenH() - screenBorder)
      {
        m_screenId = ScreenTaskManager;
        m_desiredScreenY = 0.0f;
        scrollRequested = true;
      }
    }
    break;
  }

  if (scrollRequested)
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
}

void TaskManagerInterfaceIcons::SetupRenderMatrices(int _screenId)
{
  /*
   *	This is designed to make rendering of the individual screens much easier.
   *  It maps the screen co-ordinates so that each renderer can simply render
   *  to a fixed size window, without having to worry about scrolling, screen resolution etc
   *  The window will be 800x600 on a normal screen
   *  And 960x600 on a widescreen screen
   */


  float screenRatio = static_cast<float>(g_app->m_renderer->ScreenW()) / static_cast<float>(g_app->m_renderer->ScreenH());
  m_screenH = 600.0f;
  m_screenW = m_screenH * screenRatio;

  float left = 0;
  float top = 0;

  switch (_screenId)
  {
  case ScreenOverlay:
    break;

  case ScreenTaskManager:
    top += m_screenY * m_screenH;
    break;

  case ScreenObjectives:
    top += (m_screenY - 1.0f) * m_screenH;
    break;

  case ScreenResearch:
    top += (m_screenY + 1.0f) * m_screenH;
    break;
  }

  float right = left + m_screenW;
  float bottom = top + m_screenH;

  g_imRenderer->SetProjectionMatrix(DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1, 1));
  g_imRenderer->SetViewMatrix(DirectX::XMMatrixIdentity());
  g_imRenderer->LoadIdentity();

}

void TaskManagerInterfaceIcons::ConvertMousePosition(float& _x, float& _y)
{
  float fractionX = _x / static_cast<float>(g_app->m_renderer->ScreenW());
  float fractionY = _y / static_cast<float>(g_app->m_renderer->ScreenH());

  _x = m_screenW * fractionX;
  _y = m_screenH * fractionY;
}

void TaskManagerInterfaceIcons::RestoreRenderMatrices() { g_app->m_renderer->SetupMatricesFor2D(); }

void TaskManagerInterfaceIcons::Render()
{
  if (g_app->m_editing || !g_app->m_location || EclGetWindows()->Size())
    return;

  START_PROFILE(g_app->m_profiler, "Render Taskman");

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);


  if (g_app->m_locationId != -1)
    RenderTargetAreas();

  g_gameFont.BeginText2D();
  SetupRenderMatrices(ScreenOverlay);

  RenderMessages();

  if (m_visible)
  {
    RenderTitleBar();

    if (m_screenY > -1.0f && m_screenY < 1.0f)
      RenderTaskManager();
    if (m_screenY < 0.0f)
      RenderResearch();
    if (m_screenY > 0.0f)
      RenderObjectives();

    SetupRenderMatrices(ScreenOverlay);

    //
    // Render silver bar

    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

    int divTexId = g_app->m_resource->GetTexture("textures/interface_divider.bmp");
    g_imRenderer->BindTexture(divTexId);
    g_imRenderer->SetSampler(SAMPLER_NEAREST_WRAP);

    g_imRenderer->BindTexture(divTexId);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex2i(0, m_screenH - 26);
    g_imRenderer->TexCoord2i(100, 0);
    g_imRenderer->Vertex2i(m_screenW, m_screenH - 26);
    g_imRenderer->TexCoord2i(100, 1);
    g_imRenderer->Vertex2i(m_screenW, m_screenH);
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex2i(0, m_screenH);
    g_imRenderer->End();

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex2i(0, m_screenH - 26);
    g_imRenderer->TexCoord2i(100, 0);
    g_imRenderer->Vertex2i(m_screenW, m_screenH - 26);
    g_imRenderer->TexCoord2i(100, 1);
    g_imRenderer->Vertex2i(m_screenW, m_screenH);
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex2i(0, m_screenH);
    g_imRenderer->End();


    g_imRenderer->UnbindTexture();

    RenderTooltip();
    RenderScreenZones();
  }

  RestoreRenderMatrices();

  SetupRenderMatrices(ScreenOverlay);

  if (!m_visible)
    RenderRunningTasks();
  RestoreRenderMatrices();

  //
  // Render a mouse cursor if we are visible
  // The normal one is obscured by the task manager

  if (m_visible)
  {
    float mouseX = g_target->X();
    float mouseY = g_target->Y();
    g_app->m_gameCursor->RenderStandardCursor(mouseX, mouseY);
  }

  g_gameFont.EndText2D();

  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_BACK);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);


  END_PROFILE(g_app->m_profiler, "Render Taskman");
}

void TaskManagerInterfaceIcons::AdvanceScreenZones()
{
  //
  // Copy the newly generated screenzone list over
  // Remove the old set of screen zones

  m_screenZones.EmptyAndDelete();
  while (m_newScreenZones.Size() > 0)
  {
    ScreenZone* zone = m_newScreenZones[0];
    m_screenZones.PutData(zone);
    m_newScreenZones.RemoveData(0);
    if (m_currentScreenZone == -1 && zone->m_scrollZone == m_currentScrollZone)
      m_currentScreenZone = m_screenZones.Size() - 1;
  }

  //
  // Where is the mouse right now

  bool found = false;

  if (m_visible)
  {
    for (int i = 0; i < m_screenZones.Size(); ++i)
    {
      ScreenZone* zone = m_screenZones[i];
      if (ScreenZoneHighlighted(zone))
      {
        if (m_currentScreenZone != i && m_currentMouseScreenZone != i)
        {
          m_currentMouseScreenZone = i;
          m_currentScreenZone = i;
          m_screenZoneTimer = GetHighResTime();
          g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
        }
        found = true;
      }
    }

    if (g_inputManager->controlEvent(ControlMenuDown) && m_currentScrollZone != 3)
    {
      int numZones = m_screenZones.Size();
      int zonesRemaining = numZones;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
      while (zonesRemaining)
      {
        m_currentScreenZone = (m_currentScreenZone + 1) % numZones;
        zonesRemaining--;

        if (m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone)
          break;
      }
    }

    if (g_inputManager->controlEvent(ControlMenuUp) && m_currentScrollZone != 3)
    {
      int numZones = m_screenZones.Size();
      int zonesRemaining = numZones;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
      while (zonesRemaining)
      {
        m_currentScreenZone = (m_currentScreenZone + numZones - 1) % numZones;
        zonesRemaining--;

        if (m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone)
          break;
      }
    }

    bool changeScrollZone = false;
    if (m_screenId == ScreenTaskManager)
    {
      if (g_inputManager->controlEvent(ControlMenuLeft))
      {
        if (m_currentScrollZone == 1)
        {
          m_currentScrollZone = 2;
          changeScrollZone = true;
        }
        else if (m_currentScrollZone == 3)
        {
          while (true)
          {
            m_currentScreenZone--;
            if (m_currentScreenZone < 0)
            {
              m_currentScrollZone = 2;
              changeScrollZone = true;
              break;
            }

            if ((m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone)
              || m_screenZones.Size() == 0)
              break;
          }
        }
      }

      if (g_inputManager->controlEvent(ControlMenuRight))
      {
        if (m_currentScrollZone == 2)
        {
          if (m_screenZones[m_currentScreenZone]->m_subZones)
            m_currentScrollZone = 3;
          else
            m_currentScrollZone = 1;
          changeScrollZone = true;
        }
        else if (m_currentScrollZone == 3)
        {
          while (true)
          {
            m_currentScreenZone++;
            if (m_currentScreenZone >= m_screenZones.Size())
            {
              m_currentScrollZone = 1;
              changeScrollZone = true;
              break;
            }

            if ((m_screenZones.ValidIndex(m_currentScreenZone) && m_screenZones[m_currentScreenZone]->m_scrollZone == m_currentScrollZone)
              || m_screenZones.Size() == 0)
              break;
          }
        }
      }
    }

    if (changeScrollZone)
    {
      m_currentScreenZone = -1;
      for (int i = 0; i < m_screenZones.Size(); ++i)
      {
        if (m_screenZones[i]->m_scrollZone == m_currentScrollZone)
        {
          m_currentMouseScreenZone = i;
          m_currentScreenZone = i;
          m_screenZoneTimer = GetHighResTime();
          g_app->m_soundSystem->TriggerOtherEvent(nullptr, "MouseOverIcon", SoundSourceBlueprint::TypeInterface);
          break;
        }
      }
      if (m_currentScreenZone == -1)
        m_currentScrollZone = (m_currentScreenZone == 1) ? 2 : 1;
    }
  }

  //
  // Special case - the player is highlighting a unit/entity in the real world
  // We want to highlight the button that corrisponds to that task ID
  // (if it exists)

  bool highlightOnly = false;

  if (!m_visible)
  {
    m_currentScreenZone = -1;
    WorldObjectId id;
    bool somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse(id, g_app->m_globalWorld->m_myTeamId);
    if (somethingHighlighted)
    {
      int taskIndex = -1;
      for (int i = 0; i < g_app->m_taskManager->m_tasks.Size(); ++i)
      {
        if (g_app->m_taskManager->m_tasks[i]->m_objId == id)
        {
          taskIndex = i;
          break;
        }
      }

      if (taskIndex != -1)
      {
        for (int i = 0; i < m_screenZones.Size(); ++i)
        {
          ScreenZone* zone = m_screenZones[i];
          if (stricmp(zone->m_name, "SelectTask") == 0 && zone->m_data == taskIndex)
          {
            if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
            {
              //m_currentScreenZone = i;
            }
            found = true;
            highlightOnly = true;
          }
        }
      }
    }
  }

  //if( !found ) m_currentScreenZone = -1;

  //
  // Are we highlighting a task?

  m_highlightedTaskId = -1;
  if (m_currentScreenZone != -1)
  {
    ScreenZone* currentZone = m_screenZones[m_currentScreenZone];
    if (currentZone)
    {
      if (stricmp(currentZone->m_name, "SelectTask") == 0 && g_app->m_taskManager->m_tasks.ValidIndex(currentZone->m_data))
      {
        Task* task = g_app->m_taskManager->m_tasks[currentZone->m_data];
        m_highlightedTaskId = task->m_id;
      }
    }
  }

  //
  // Handle mouse clickage

  if (m_currentScreenZone != -1 && (g_inputManager->controlEvent(ControlActivateTMButton) || ButtonHeldAndReleased()) && !highlightOnly)
  {
    ScreenZone* currentZone = m_screenZones[m_currentScreenZone];
    if (currentZone)
      RunScreenZone(currentZone->m_name, currentZone->m_data);
  }
}

bool TaskManagerInterfaceIcons::ButtonHeld() { return GetHighResTime() - m_taskManagerDownTime > 0.5; }

bool TaskManagerInterfaceIcons::ButtonHeldAndReleased()
{
  return ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayUp);
}

void TaskManagerInterfaceIcons::AdvanceKeyboardShortcuts()
{
  for (int i = 0; i < m_keyboardShortcuts.Size(); ++i)
  {
    KeyboardShortcut* shortcut = m_keyboardShortcuts[i];
    if ((*shortcut)())
      RunScreenZone(shortcut->name(), shortcut->data());
  }
}

void TaskManagerInterfaceIcons::RunScreenZone(const char* _name, int _data)
{
  //
  // New task buttons

  if (stricmp(_name, "NewTask") == 0)
  {
    //
    // Prevent creation of new tasks if we're placing one

    Task* task = g_app->m_taskManager->GetCurrentTask();
    if (task && task->m_state == Task::StateStarted && task->m_type != GlobalResearch::TypeOfficer)
    {
      SetCurrentMessage(MessageFailure, -1, 2.5f);
      return;
    }

    if (g_app->m_globalWorld->m_research->HasResearch(_data))
    {
      g_app->m_clientToServer->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, _data);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "GestureBegin", SoundSourceBlueprint::TypeGesture);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
    else
    {
      SetCurrentMessage(MessageFailure, -1, 2.5f);
    }
  }

  //
  // Select task buttons

  if (stricmp(_name, "SelectTask") == 0)
  {
    if (g_app->m_taskManager->m_tasks.ValidIndex(_data))
    {
      Task* nextTask = g_app->m_taskManager->m_tasks[_data];
      g_app->m_taskManager->m_currentTaskId = nextTask->m_id;
      g_app->m_taskManager->SelectTask(g_app->m_taskManager->m_currentTaskId);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "SelectTask", SoundSourceBlueprint::TypeInterface);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
  }

  //
  // Delete task buttons

  if (stricmp(_name, "DeleteTask") == 0)
  {
    if (_data == -1)
      g_app->m_taskManager->TerminateTask(g_app->m_taskManager->m_currentTaskId);
    else
      g_app->m_taskManager->TerminateTask(_data);
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "DeleteTask", SoundSourceBlueprint::TypeInterface);
  }

  //
  // Select weapon buttons

  if (stricmp(_name, "SelectWeapon") == 0)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(_data))
    {
      g_app->m_clientToServer->RequestRunProgram(g_app->m_globalWorld->m_myTeamId, _data);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "GestureBegin", SoundSourceBlueprint::TypeGesture);
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "GestureSuccess", SoundSourceBlueprint::TypeGesture);
      if (g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD)
        HideTaskManager();
    }
    else
    {
      SetCurrentMessage(MessageFailure, -1, 2.5f);
    }
  }

  //
  // Show screen

  if (m_visible)
  {
    if (stricmp(_name, "ShowScreen") == 0)
    {
      m_screenId = _data;

      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        m_desiredScreenY = 0.0f;
        break;
      case ScreenObjectives:
        m_desiredScreenY = 1.0f;
        break;
      case ScreenResearch:
        m_desiredScreenY = -1.0f;
        break;
      }

      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (stricmp(_name, "ScreenUp") == 0)
    {
      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        RunScreenZone("ShowScreen", ScreenResearch);
        break;
      case ScreenObjectives:
        RunScreenZone("ShowScreen", ScreenTaskManager);
        break;
      }
    }

    if (stricmp(_name, "ScreenDown") == 0)
    {
      switch (m_screenId)
      {
      case ScreenOverlay:
      case ScreenTaskManager:
        RunScreenZone("ShowScreen", ScreenObjectives);
        break;
      case ScreenResearch:
        RunScreenZone("ShowScreen", ScreenTaskManager);
        break;
      }
    }
  }

  //
  // Research

  if (stricmp(_name, "Research") == 0)
    g_app->m_globalWorld->m_research->SetCurrentResearch(_data);

  //
  // Objectives

  if (stricmp(_name, "Objective") == 0)
  {
    bool primary = _data > 0;
    int objectiveId = _data;
    if (!primary)
      objectiveId *= -1;
    objectiveId -= 10;

    GlobalEventCondition* gec = nullptr;
    if (primary)
      gec = g_app->m_location->m_levelFile->m_primaryObjectives[objectiveId];
    else
      gec = g_app->m_location->m_levelFile->m_secondaryObjectives[objectiveId];
    DarwiniaDebugAssert(gec);

    if (gec->m_cutScene)
      g_app->m_script->RunScript(gec->m_cutScene);
    else
      RunDefaultObjective(gec);
    HideTaskManager();
  }
}

bool TaskManagerInterfaceIcons::ScreenZoneHighlighted(ScreenZone* _zone)
{
  float mouseX = g_target->X();
  float mouseY = g_target->Y();
  ConvertMousePosition(mouseX, mouseY);

  mouseY += m_screenY * m_screenH;

  return (mouseX >= _zone->m_x && mouseY >= _zone->m_y && mouseX <= _zone->m_x + _zone->m_w && mouseY <= _zone->m_y + _zone->m_h);
}

void TaskManagerInterfaceIcons::RenderScreenZones()
{
  /*
      for( int i = 0; i < m_screenZones.Size(); ++i )
      {
          ScreenZone *zone = m_screenZones[i];
  
          float hX = zone->m_x;
          float hY = zone->m_y;
          float hW = zone->m_w;
          float hH = zone->m_h;
  
          hX -= m_screenY * m_screenW;
  
          g_imRenderer->Color4f( 1.0f, 1.0f, 1.0f, 1.0f );
  
          g_imRenderer->Begin(PRIM_LINE_LOOP);
              g_imRenderer->Vertex2f( hX, hY );
              g_imRenderer->Vertex2f( hX+hW, hY );
              g_imRenderer->Vertex2f( hX+hW, hY+hH );
              g_imRenderer->Vertex2f( hX, hY+hH );
          g_imRenderer->End();

      }
  
  */

  if (m_screenZones.ValidIndex(m_currentScreenZone))
  {
    ScreenZone* zone = m_screenZones[m_currentScreenZone];

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

    float overSpill = zone->m_h * 0.1f;
    float hX = zone->m_x - overSpill;
    float hY = zone->m_y - overSpill;
    float hW = zone->m_w + overSpill * 2;
    float hH = zone->m_h + overSpill * 2;

    //hX -= m_screenY * m_screenW;
    hY -= m_screenY * m_screenH;

    g_imRenderer->Color4f(0.05f, 0.05f, 0.5f, 0.3f);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2f(hX, hY);
    g_imRenderer->Vertex2f(hX + hW, hY);
    g_imRenderer->Vertex2f(hX + hW, hY + hH);
    g_imRenderer->Vertex2f(hX, hY + hH);
    g_imRenderer->End();

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2f(hX, hY);
    g_imRenderer->Vertex2f(hX + hW, hY);
    g_imRenderer->Vertex2f(hX + hW, hY + hH);
    g_imRenderer->Vertex2f(hX, hY + hH);
    g_imRenderer->End();


    g_imRenderer->Color4f(1.0f, 1.0f, 0.3f, 1.0f);
    int selTexId = g_app->m_resource->GetTexture("icons/mouse_selection.bmp");
    g_imRenderer->BindTexture(selTexId);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2f(0.0f, 0.0f);
    g_imRenderer->Vertex2f(hX, hY);
    g_imRenderer->TexCoord2f(0.5f, 0.0f);
    g_imRenderer->Vertex2f(hX + hH / 2, hY);
    g_imRenderer->TexCoord2f(0.5f, 1.0f);
    g_imRenderer->Vertex2f(hX + hH / 2, hY + hH);
    g_imRenderer->TexCoord2f(0.0f, 1.0f);
    g_imRenderer->Vertex2f(hX, hY + hH);

    g_imRenderer->TexCoord2f(0.5f, 0.0f);
    g_imRenderer->Vertex2f(hX + hW - hH / 2, hY);
    g_imRenderer->TexCoord2f(1.0f, 0.0f);
    g_imRenderer->Vertex2f(hX + hW, hY);
    g_imRenderer->TexCoord2f(1.0f, 1.0f);
    g_imRenderer->Vertex2f(hX + hW, hY + hH);
    g_imRenderer->TexCoord2f(0.5f, 1.0f);
    g_imRenderer->Vertex2f(hX + hW - hH / 2, hY + hH);
    g_imRenderer->End();

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2f(0.0f, 0.0f);
    g_imRenderer->Vertex2f(hX, hY);
    g_imRenderer->TexCoord2f(0.5f, 0.0f);
    g_imRenderer->Vertex2f(hX + hH / 2, hY);
    g_imRenderer->TexCoord2f(0.5f, 1.0f);
    g_imRenderer->Vertex2f(hX + hH / 2, hY + hH);
    g_imRenderer->TexCoord2f(0.0f, 1.0f);
    g_imRenderer->Vertex2f(hX, hY + hH);

    g_imRenderer->TexCoord2f(0.5f, 0.0f);
    g_imRenderer->Vertex2f(hX + hW - hH / 2, hY);
    g_imRenderer->TexCoord2f(1.0f, 0.0f);
    g_imRenderer->Vertex2f(hX + hW, hY);
    g_imRenderer->TexCoord2f(1.0f, 1.0f);
    g_imRenderer->Vertex2f(hX + hW, hY + hH);
    g_imRenderer->TexCoord2f(0.5f, 1.0f);
    g_imRenderer->Vertex2f(hX + hW - hH / 2, hY + hH);
    g_imRenderer->End();


    g_imRenderer->UnbindTexture();
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_imRenderer->UnbindTexture();
  }
}

void TaskManagerInterfaceIcons::RenderTooltip()
{
  g_gameFont.SetRenderShadow(true);
  g_imRenderer->Color4ub(255, 255, 150, 30);

  if (m_screenZones.ValidIndex(m_currentScreenZone))
  {
    ScreenZone* zone = m_screenZones[m_currentScreenZone];

    //
    // Render tooltip caption

    float timeRequired = strlen(zone->m_toolTip) / 50.0f;
    float timeSoFar = GetHighResTime() - m_screenZoneTimer;

    char clippedTooltip[1024];
    strcpy(clippedTooltip, zone->m_toolTip);
    if (timeSoFar < timeRequired)
    {
      float fraction = timeSoFar / timeRequired;
      clippedTooltip[static_cast<int>(strlen(clippedTooltip) * fraction)] = '\x0';
    }

    g_gameFont.DrawText2D(20, m_screenH - 12, 12, clippedTooltip);
    g_gameFont.DrawText2D(20, m_screenH - 12, 12, clippedTooltip);

    if (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
    {
      //
      // Render keyboard shortcut

      KeyboardShortcut* selectedShortcut = nullptr;

      for (int i = 0; i < m_keyboardShortcuts.Size(); ++i)
      {
        KeyboardShortcut* shortcut = m_keyboardShortcuts[i];
        if (stricmp(shortcut->name(), zone->m_name) == 0 && shortcut->data() == zone->m_data)
        {
          selectedShortcut = shortcut;
          break;
        }

        if (stricmp(shortcut->name(), zone->m_name) == 0 && stricmp(shortcut->name(), "DeleteTask") == 0)
        {
          // Special case : DeleteTask shortcut key
          selectedShortcut = shortcut;
          break;
        }
      }

      if (selectedShortcut)
      {
        char caption[256];

        sprintf(caption, "Keyboard shortcut : %s", selectedShortcut->noun().c_str());

        g_gameFont.DrawText2D(m_screenW - 250, m_screenH - 12, 12, caption);
        g_gameFont.DrawText2D(m_screenW - 250, m_screenH - 12, 12, caption);
      }
    }
  }

  //    else
  //    {
  //        //
  //        // How do I escape the task manager
  //
  //        char *toolTip = LANGUAGEPHRASE("newcontrols_hidetaskmanager");
  //        glColor4ub( 255, 255, 150, 50 );
  //        g_gameFont.DrawText2D( 20, m_screenH - 12, 12, toolTip );
  //    }

  g_gameFont.SetRenderShadow(false);
}

void TaskManagerInterfaceIcons::RenderMessages()
{
  if (m_currentMessageType != -1)
  {
    //
    // Are we done yet?

    if (GetHighResTime() >= m_messageTimer)
    {
      m_currentMessageType = -1;
      return;
    }

    //
    // Lookup message portion

    char currentMessageStringId[256];
    sprintf(currentMessageStringId, "taskmanager_msg%d", m_currentMessageType);
    if (!ISLANGUAGEPHRASE(currentMessageStringId))
    {
      m_currentMessageType = -1;
      return;
    }

    const char* message = LANGUAGEPHRASE(currentMessageStringId);

    //
    // Lookup task name

    char* taskName = nullptr;

    if (m_currentTaskType == 999)
      taskName = LANGUAGEPHRASE("taskmanager_mapeditor");
    else if (m_currentTaskType == 998)
      taskName = LANGUAGEPHRASE("taskmanager_accessallareas");
    else if (m_currentTaskType != -1)
      taskName = Task::GetTaskNameTranslated(m_currentTaskType);

    //
    // Build string

    char fullMessage[256];
    if (taskName)
    {
      if (m_currentMessageType == MessageResearchUpgrade)
      {
        int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel(m_currentTaskType);
        sprintf(fullMessage, "%s: %s v%d.0", message, taskName, researchLevel);
      }
      else
        sprintf(fullMessage, "%s: %s", message, taskName);
    }
    else
      sprintf(fullMessage, "%s", message);

    //
    // Render string

    float timeRemaining = m_messageTimer - GetHighResTime();
    float alpha = timeRemaining * 0.5f;
    alpha = min(alpha, 1.0f);
    alpha = max(alpha, 0.0f);
    float size = 40.0f;
    if (timeRemaining < 2.0f)
    {
      //size = sqrtf( 2.7f - timeRemaining ) * 30.0f;
      size += sqrtf(2.0f - timeRemaining) * 15.0f;
    }

    float outlineAlpha = alpha * alpha * alpha;

    g_gameFont.SetRenderOutline(true);
    g_imRenderer->Color4f(outlineAlpha, outlineAlpha, outlineAlpha, 0.0f);
    g_gameFont.DrawText2DCentre(m_screenW / 2.0f, 370.0f, size, fullMessage);

    g_gameFont.SetRenderOutline(false);
    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, alpha);
    g_gameFont.DrawText2DCentre(m_screenW / 2.0f, 370.0f, size, fullMessage);
  }
}

void TaskManagerInterfaceIcons::RenderTargetAreas()
{
  Task* task = g_app->m_taskManager->GetCurrentTask();

  if (task && task->m_type != GlobalResearch::TypeOfficer && task->m_state == Task::StateStarted)
  {
    LList<TaskTargetArea>* targetAreas = g_app->m_taskManager->GetTargetArea(task->m_id);
    RGBAColour* colour = &g_app->m_location->GetMyTeam()->m_colour;

    for (int i = 0; i < targetAreas->Size(); ++i)
    {
      TaskTargetArea* tta = targetAreas->GetPointer(i);

      float angle = g_gameTime * 3.0f;
      LegacyVector3 dif(tta->m_radius * sinf(angle), 0.0f, tta->m_radius * cosf(angle));

      LegacyVector3 pos = tta->m_centre + dif;
      pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 5.0f;
      g_app->m_particleSystem->CreateParticle(pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f);

      pos = tta->m_centre - dif;
      pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 5.0f;
      g_app->m_particleSystem->CreateParticle(pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0f);
    }

    delete targetAreas;
  }
}

static void RenderIcon(const char* _foreground, const char* _background, int _x, int _y, float _iconSize, unsigned _alpha)
{
  // Render the shadow
  int bgTexId = g_app->m_resource->GetTexture(_background);
  g_imRenderer->BindTexture(bgTexId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_imRenderer->Color4ub(_alpha, _alpha, _alpha, 0);

  g_imRenderer->BindTexture(bgTexId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_imRenderer->Color4ub(_alpha, _alpha, _alpha, 0.0f);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(_x, _y);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(_x + _iconSize, _y);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(_x + _iconSize, _y + _iconSize);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(_x, _y + _iconSize);
  g_imRenderer->End();

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(_x, _y);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(_x + _iconSize, _y);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(_x + _iconSize, _y + _iconSize);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(_x, _y + _iconSize);
  g_imRenderer->End();


  // Render the icon
  int fgTexId = g_app->m_resource->GetTexture(_foreground);
  g_imRenderer->BindTexture(fgTexId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_imRenderer->Color4ub(255, 255, 255, _alpha);

  g_imRenderer->BindTexture(fgTexId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_imRenderer->Color4ub(255, 255, 255, _alpha);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(_x, _y);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(_x + _iconSize, _y);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(_x + _iconSize, _y + _iconSize);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(_x, _y + _iconSize);
  g_imRenderer->End();

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(_x, _y);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(_x + _iconSize, _y);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(_x + _iconSize, _y + _iconSize);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(_x, _y + _iconSize);
  g_imRenderer->End();

}

void TaskManagerInterfaceIcons::RenderTaskManager()
{
  SetupRenderMatrices(ScreenTaskManager);

  g_gameFont.SetRenderOutline(true);
  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_taskmanager"));
  g_gameFont.SetRenderOutline(false);
  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_taskmanager"));

  //
  // Render running tasks

  RenderRunningTasks();

  //
  // Render task creation menu

  RenderCreateTaskMenu();

  //
  // Render navigation arrows

  if (fabs(m_screenY) < 0.1f)
  {
    bool render360Controls = g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1);

    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    if (render360Controls)
    {
      RenderIcon("icons/button_lb.bmp", "icons/button_lb_shadow.bmp", m_screenW - 60, 10, 40, alpha);
      RenderIcon("icons/button_rb.bmp", "icons/button_rb_shadow.bmp", m_screenW - 60, m_screenH - 70, 40, alpha);
    }
    else
    {
      g_imRenderer->Color4ub(199, 214, 220, alpha);

      g_imRenderer->Begin(PRIM_TRIANGLES);
      g_imRenderer->Vertex2f(m_screenW - 40, 10);
      g_imRenderer->Vertex2f(m_screenW - 20, 30);
      g_imRenderer->Vertex2f(m_screenW - 60, 30);

      g_imRenderer->Vertex2f(m_screenW - 40, m_screenH - 30);
      g_imRenderer->Vertex2f(m_screenW - 20, m_screenH - 50);
      g_imRenderer->Vertex2f(m_screenW - 60, m_screenH - 50);
      g_imRenderer->End();


    }

    auto zoneLeft = new ScreenZone("ScreenUp", LANGUAGEPHRASE("newcontrols_showresearch"), m_screenW - 60, 10, 40, 20, -1);
    auto zoneRight = new ScreenZone("ScreenDown", LANGUAGEPHRASE("newcontrols_showobjectives"), m_screenW - 60, m_screenH - 50, 40, 20, -1);

    m_newScreenZones.PutData(zoneLeft);
    m_newScreenZones.PutData(zoneRight);
  }
}

void TaskManagerInterfaceIcons::RenderCreateTaskMenu()
{
  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeSquad, GlobalResearch::TypeEngineer, GlobalResearch::TypeOfficer, GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  //float w = 200;
  //float h = 36;
  //float x = (m_screenW-80)/2 - w/2;
  //float y = 350;

  float w = 200;
  float h = 46;
  float x = m_screenW / 2 + 30;
  float y = m_screenH / 2;

  //
  // Title bar

  float titleHeight = 20.0f;
  float boxH = numAvailable * (h + 5);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Vertex2f(x, y - titleHeight);
  g_imRenderer->Vertex2f(x + w, y - titleHeight);
  g_imRenderer->Color4ub(112, 141, 168, 255);
  g_imRenderer->Vertex2f(x + w, y);
  g_imRenderer->Vertex2f(x, y);
  g_imRenderer->End();

  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Color4ub(112, 141, 168, 255);

  g_gameFont.SetRenderShadow(true);
  g_imRenderer->Color4ub(255, 255, 150, 30);
  g_gameFont.DrawText2DCentre(x + w / 2.0f, y - titleHeight / 2 + 1, 13, LANGUAGEPHRASE("newcontrols_createnewtask"));
  g_gameFont.DrawText2DCentre(x + w / 2.0f, y - titleHeight / 2 + 1, 13, LANGUAGEPHRASE("newcontrols_createnewtask"));
  g_gameFont.SetRenderShadow(false);

  //
  // Background box

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/interface_red.bmp"));

  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.8f);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2f(0, 0);
  g_imRenderer->Vertex2f(x, y);
  g_imRenderer->TexCoord2f(1, 0);
  g_imRenderer->Vertex2f(x + w, y);
  g_imRenderer->TexCoord2f(1, boxH / m_screenH * 1.5);
  g_imRenderer->Vertex2f(x + w, y + boxH);
  g_imRenderer->TexCoord2f(0, boxH / m_screenH * 1.5);
  g_imRenderer->Vertex2f(x, y + boxH);
  g_imRenderer->End();


  g_imRenderer->UnbindTexture();

  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Begin(PRIM_LINE_LOOP);
  g_imRenderer->Vertex2f(x, y - titleHeight);
  g_imRenderer->Vertex2f(x + w, y - titleHeight);
  g_imRenderer->Vertex2f(x + w, y + boxH);
  g_imRenderer->Vertex2f(x, y + boxH);
  g_imRenderer->End();


  y += titleHeight;

  for (int i = 0; i < numRunnableTasks; ++i)
  {
    int taskType = runnableTaskType[i];
    if (g_app->m_globalWorld->m_research->HasResearch(taskType))
    {
      char tooltipId[128];
      sprintf(tooltipId, "newcontrols_create_%s", GlobalResearch::GetTypeName(taskType), i + 1);

      auto zone = new ScreenZone("NewTask", LANGUAGEPHRASE(tooltipId), x + 5, y - h / 3, w - 10, h, taskType);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      //
      // Render task name and F-key shortcut

      g_gameFont.SetRenderOutline(true);
      g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(x + 70, y + 5, 18, GlobalResearch::GetTypeNameTranslated(taskType));
      g_gameFont.SetRenderOutline(false);
      g_imRenderer->Color4f(0.8f, 0.8f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(x + 70, y + 5, 18, GlobalResearch::GetTypeNameTranslated(taskType));

      //
      // Render the shadow

      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
      g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
      g_imRenderer->Color4f(0.5f, 0.5f, 0.5f, 0.0f);

      float iconSize = (h - 6);
      float iconX = x + 10;
      float iconY = y - 12;

      float shadowSize = iconSize * 1.1f;

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex2f(iconX, iconY);
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex2f(iconX + shadowSize, iconY);
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex2f(iconX + shadowSize, iconY + shadowSize);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex2f(iconX, iconY + shadowSize);
      g_imRenderer->End();


      g_imRenderer->UnbindTexture();

      //
      // Render the task symbol

      char iconFilename[256];
      sprintf(iconFilename, "icons/icon_%s.bmp", GlobalResearch::GetTypeName(taskType));
      unsigned int texId = g_app->m_resource->GetTexture(iconFilename);
      if (texId != -1)
      {
        g_imRenderer->BindTexture(texId);
        g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

        g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->TexCoord2i(0, 1);
        g_imRenderer->Vertex2f(iconX, iconY);
        g_imRenderer->TexCoord2i(1, 1);
        g_imRenderer->Vertex2f(iconX + iconSize, iconY);
        g_imRenderer->TexCoord2i(1, 0);
        g_imRenderer->Vertex2f(iconX + iconSize, iconY + iconSize);
        g_imRenderer->TexCoord2i(0, 0);
        g_imRenderer->Vertex2f(iconX, iconY + iconSize);
        g_imRenderer->End();


        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
        g_imRenderer->UnbindTexture();
      }
    }

    y += h + 4;
  }
}

void TaskManagerInterfaceIcons::RenderTitleBar()
{
  SetupRenderMatrices(ScreenTaskManager);

  //
  // Render panel

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

  g_imRenderer->Color4f(0.5f, 0.5f, 1.0f, 0.7f);
  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/interface_grey.bmp"));

  float panelW = 80;

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2i(m_screenW, -m_screenH);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2i(m_screenW, m_screenH * 2);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2i(m_screenW - panelW, m_screenH * 2);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2i(m_screenW - panelW, -m_screenH);
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  g_imRenderer->UnbindTexture();

  //
  // Render dividing line

  g_imRenderer->Begin(PRIM_LINES);
  g_imRenderer->Vertex2i(m_screenW - panelW, -m_screenH);
  g_imRenderer->Vertex2i(m_screenW - panelW, m_screenH * 2);
  g_imRenderer->End();

}

void TaskManagerInterfaceIcons::RenderRunningTasks()
{
  int numSlots = g_app->m_taskManager->Capacity();

  static float s_iconSize = 45;
  static float s_iconX = 40;
  static float s_iconY = 130;

  float desiredIconSize = 50;
  float desiredIconX = 40;
  float desiredIconY = 130;

  if (m_visible)
  {
    desiredIconSize = 70;
    desiredIconX = 80;
    desiredIconY = 130;
  }

  float factor = g_advanceTime * 10;
  s_iconSize = (s_iconSize * (1 - factor)) + (desiredIconSize * factor);
  s_iconX = (s_iconX * (1 - factor)) + (desiredIconX * factor);
  s_iconY = (s_iconY * (1 - factor)) + (desiredIconY * factor);

  float iconX = s_iconX;
  float iconY = s_iconY;
  float iconSize = s_iconSize;
  float iconGap = 10;
  float shadowOffset = 0;
  float shadowSize = iconSize + 10;
  float totalWidth = (numSlots - 1) * (iconSize + iconGap);

  float iconAlpha = 0.9f;

  //
  // Render shadows for available task slots

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_imRenderer->Color4f(iconAlpha, iconAlpha, iconAlpha, 0.0f);

  for (int i = 0; i < numSlots; ++i)
  {
    Vector2 iconCentre(iconX, iconY);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
    g_imRenderer->End();


    iconY += iconSize;
    iconY += iconGap;
  }

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_imRenderer->UnbindTexture();
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);

  iconY = s_iconY;

  //
  // Render our running tasks

  int numTasks = g_app->m_taskManager->m_tasks.Size();

  for (int i = 0; i < numTasks; ++i)
  {
    Task* task = g_app->m_taskManager->m_tasks[i];
    char bmpFilename[256];
    sprintf(bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type));
    unsigned int texId = g_app->m_resource->GetTexture(bmpFilename);

    //
    // Create clickable zone over the task

    char captionId[256];
    sprintf(captionId, "newcontrols_select_%s", Task::GetTaskName(task->m_type));
    if (task->m_state == Task::StateStarted)
      sprintf(captionId, "newcontrols_place_%s", Task::GetTaskName(task->m_type));

    auto zone = new ScreenZone("SelectTask", LANGUAGEPHRASE(captionId), iconX - iconSize / 2, iconY - iconSize / 2, iconSize, iconSize, i);
    m_newScreenZones.PutData(zone);
    zone->m_scrollZone = 2;

    bool invisible = (task->m_state == Task::StateStarted && fmod(g_gameTime, 1.0) < 0.4);

    Vector2 iconCentre(iconX, iconY);

    if (!invisible)
    {
      g_imRenderer->BindTexture(texId);
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

      if (task->m_id == g_app->m_taskManager->m_currentTaskId)
      {
        g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, iconAlpha);
      }
      else
      {
        g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, iconAlpha * 0.3f);
      }

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y - iconSize / 2);
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y - iconSize / 2);
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y + iconSize / 2);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y + iconSize / 2);
      g_imRenderer->End();


      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
      g_imRenderer->UnbindTexture();

      if (task->m_type == GlobalResearch::TypeEngineer)
      {
        auto engineer = static_cast<Engineer*>(g_app->m_location->GetEntitySafe(task->m_objId, Entity::TypeEngineer));
        if (engineer)
        {
          char* state = engineer->GetCurrentAction();
          int numSpirits = engineer->GetNumSpirits();

          g_gameFont.SetRenderShadow(true);
          g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.0f);
          g_gameFont.DrawText2DCentre(iconCentre.x + 3, iconCentre.y + 25, 7, state);
          g_gameFont.DrawText2DCentre(iconCentre.x + 3, iconCentre.y + 2, 12, "%d", numSpirits);

          g_gameFont.SetRenderShadow(false);
          g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.9f);
          g_gameFont.DrawText2DCentre(iconCentre.x + 3, iconCentre.y + 25, 7, state);
          g_gameFont.DrawText2DCentre(iconCentre.x + 3, iconCentre.y + 2, 12, "%d", numSpirits);
        }
      }
      else if (task->m_type == GlobalResearch::TypeSquad)
      {
        auto squad = static_cast<InsertionSquad*>(g_app->m_location->GetUnit(task->m_objId));
        if (squad)
        {
          int numSquaddies = squad->NumAliveEntities();

          g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.0f);
          g_gameFont.SetRenderShadow(true);
          g_gameFont.DrawText2D(iconCentre.x + 11, iconCentre.y + 5, 10, "x%d", numSquaddies);

          g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.9f);
          g_gameFont.SetRenderShadow(false);
          g_gameFont.DrawText2D(iconCentre.x + 11, iconCentre.y + 5, 10, "x%d", numSquaddies);
        }
      }

      //
      // Render weapon selection if we're a squad
      // Render mode selection if we're an officer or armour
      // Render compass if we are an object

      LegacyVector3 taskWorldPos;
      Unit* unit = g_app->m_location->GetUnit(task->m_objId);
      Entity* entity = g_app->m_location->GetEntity(task->m_objId);
      if (unit)
        taskWorldPos = unit->m_centrePos;
      if (entity)
        taskWorldPos = entity->m_pos;
      float compassSize = iconSize * 0.75f;

      if (taskWorldPos != g_zeroVector)
      {
        RenderCompass(iconCentre.x, iconCentre.y, taskWorldPos, task->m_id == g_app->m_taskManager->m_currentTaskId, compassSize);
      }

      if (task->m_state != Task::StateStarted && m_visible && task->m_id == g_app->m_taskManager->m_currentTaskId)
      {
        if (task->m_type == GlobalResearch::TypeSquad)
        {
          LList<int> availableWeapons;
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeGrenade))
            availableWeapons.PutData(GlobalResearch::TypeGrenade);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeRocket))
            availableWeapons.PutData(GlobalResearch::TypeRocket);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeAirStrike))
            availableWeapons.PutData(GlobalResearch::TypeAirStrike);
          if (g_app->m_globalWorld->m_research->HasResearch(GlobalResearch::TypeController))
            availableWeapons.PutData(GlobalResearch::TypeController);

          auto squad = static_cast<InsertionSquad*>(unit);
          int currentWeapon = -1;
          if (squad)
            currentWeapon = squad->m_weaponType;

          float weaponSize = iconSize * 0.5f;
          float weaponGap = weaponSize * 0.15f;
          float weaponX = iconCentre.x + iconSize / 1.2f;
          float weaponY = iconCentre.y + iconSize / 5;


          if (availableWeapons.Size() > 0)
            zone->m_subZones = true;

          for (int i = 0; i < availableWeapons.Size(); ++i)
          {
            int weaponType = availableWeapons[i];
            sprintf(bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(weaponType));
            texId = g_app->m_resource->GetTexture(bmpFilename);

            g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
            g_imRenderer->Color4f(0.9f, 0.9f, 0.9f, 0.0f);
            g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
            g_imRenderer->Begin(PRIM_QUADS);
            g_imRenderer->TexCoord2i(0, 1);
            g_imRenderer->Vertex2f(weaponX - weaponSize / 2, weaponY - weaponSize / 2);
            g_imRenderer->TexCoord2i(1, 1);
            g_imRenderer->Vertex2f(weaponX + weaponSize / 2, weaponY - weaponSize / 2);
            g_imRenderer->TexCoord2i(1, 0);
            g_imRenderer->Vertex2f(weaponX + weaponSize / 2, weaponY + weaponSize / 2);
            g_imRenderer->TexCoord2i(0, 0);
            g_imRenderer->Vertex2f(weaponX - weaponSize / 2, weaponY + weaponSize / 2);
            g_imRenderer->End();


            g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

            g_imRenderer->BindTexture(texId);
            g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, iconAlpha * 0.3f);
            if (weaponType == currentWeapon)
              g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, iconAlpha);

            g_imRenderer->Begin(PRIM_QUADS);
            g_imRenderer->TexCoord2i(0, 1);
            g_imRenderer->Vertex2f(weaponX - weaponSize / 2, weaponY - weaponSize / 2);
            g_imRenderer->TexCoord2i(1, 1);
            g_imRenderer->Vertex2f(weaponX + weaponSize / 2, weaponY - weaponSize / 2);
            g_imRenderer->TexCoord2i(1, 0);
            g_imRenderer->Vertex2f(weaponX + weaponSize / 2, weaponY + weaponSize / 2);
            g_imRenderer->TexCoord2i(0, 0);
            g_imRenderer->Vertex2f(weaponX - weaponSize / 2, weaponY + weaponSize / 2);
            g_imRenderer->End();


            char captionId[256];
            sprintf(captionId, "newcontrols_select_%s", Task::GetTaskName(weaponType));
            auto zone = new ScreenZone("SelectWeapon", LANGUAGEPHRASE(captionId), weaponX - weaponSize / 2, weaponY - weaponSize / 2,
                                       weaponSize, weaponSize, weaponType);
            m_newScreenZones.PutData(zone);
            zone->m_scrollZone = 3;

            weaponX += weaponSize;
            weaponX += weaponGap;
          }

          g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
          g_imRenderer->UnbindTexture();
        }
      }
    }

    //
    // Render delete icon

    if (m_visible)
    {
      float deleteX = iconCentre.x - iconSize / 1.7f;
      float deleteY = iconCentre.y - iconSize / 1.7f;
      float deleteSize = iconSize * 0.4f;

      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
      g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
      g_imRenderer->Color4f(0.9f, 0.9f, 0.9f, 0.0f);

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex2f(deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex2f(deleteX + deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex2f(deleteX + deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex2f(deleteX - deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      g_imRenderer->End();


      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_delete.bmp"));
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
      g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex2f(deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex2f(deleteX + deleteSize / 2.0f, deleteY - deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex2f(deleteX + deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex2f(deleteX - deleteSize / 2.0f, deleteY + deleteSize / 2.0f);
      g_imRenderer->End();


      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
      g_imRenderer->UnbindTexture();

      char captionId[256];
      sprintf(captionId, "newcontrols_delete_%s", Task::GetTaskName(task->m_type));

      auto zone = new ScreenZone("DeleteTask", LANGUAGEPHRASE(captionId), deleteX - deleteSize / 2.0f, deleteY - deleteSize / 2.0f,
                                 deleteSize, deleteSize, task->m_id);
      m_newScreenZones.PutData(zone);
    }

    iconY += iconSize;
    iconY += iconGap;
  }
}

void TaskManagerInterfaceIcons::RenderCompass(float _screenX, float _screenY, const LegacyVector3& _worldPos, bool _selected, float _size)
{
  g_app->m_renderer->SetupMatricesFor3D();
  float screenH = g_app->m_renderer->ScreenH();
  float screenW = g_app->m_renderer->ScreenW();

  //
  // Project worldPos into screen co-ordinates
  // Is it on screen or not?

  float screenX, screenY;
  g_app->m_camera->Get2DScreenPos(_worldPos, &screenX, &screenY);

  LegacyVector3 toCam = g_app->m_camera->GetPos() - _worldPos;
  float angle = toCam * g_app->m_camera->GetFront();
  LegacyVector3 rotationVector = toCam ^ g_app->m_camera->GetFront();

  Vector2 compassVector;

  if (angle <= 0.0f && screenX >= 0 && screenX < screenW && screenY >= 0 && screenY < screenH)
  {
    // _pos is onscreen
    screenX *= (m_screenW / screenW);
    screenY *= (m_screenH / screenH);

    compassVector.Set(screenX - _screenX, screenY - _screenY);
    compassVector.Normalise();
  }
  else
  {
    // _pos is offscreen
    LegacyVector3 rayStart, rayDir;
    g_app->m_camera->GetClickRay(_screenX * screenW / m_screenW, _screenY * screenH / m_screenH, &rayStart, &rayDir);
    LegacyVector3 camPos = rayStart + rayDir * 1000;
    LegacyVector3 camToTarget = (_worldPos - camPos).SetLength(100);

    float posX, posY;
    g_app->m_camera->Get2DScreenPos(camPos + camToTarget, &posX, &posY);
    posX *= (m_screenW / screenW);
    posY *= (m_screenH / screenH);

    compassVector.Set(posX - _screenX, posY - _screenY);
    compassVector.Normalise();
  }

  //
  // Render the compass

  Vector2 screenPos(_screenX - 0.5f, _screenY - 0.5f);
  Vector2 compassRight = compassVector;
  float temp = compassRight.x;
  compassRight.x = compassRight.y;
  compassRight.y = temp * -1.0f;

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/compass.bmp", true, false));

  g_app->m_renderer->SetupMatricesFor2D();
  SetupRenderMatrices(ScreenTaskManager);

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  if (_selected)
  {
    g_imRenderer->Color4f(1.0f, 1.0f, 0.3f, 0.8f);
  }
  else
  {
    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.6f);
  }

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_imRenderer->UnbindTexture();
}

void TaskManagerInterfaceIcons::RenderOverview()
{
  float boxW = 100;
  float boxX = (m_screenW / 2.0f) - (boxW / 2) + (m_screenY * boxW);
  float boxY = 95;
  float boxH = 16;
  float screenCentre = m_screenW / 2.0f;

  g_gameFont.SetRenderShadow(true);
  g_imRenderer->Color4ub(255, 255, 150, 30);
  g_gameFont.DrawText2DCentre(screenCentre - boxW, 105, 12, LANGUAGEPHRASE("taskmanager_research"));
  g_gameFont.DrawText2DCentre(screenCentre, 105, 12, LANGUAGEPHRASE("taskmanager_taskmanager"));
  g_gameFont.DrawText2DCentre(screenCentre + boxW, 105, 12, LANGUAGEPHRASE("taskmanager_objectives"));
  g_gameFont.SetRenderShadow(false);

  g_imRenderer->Color4f(0.0f, 0.0f, 0.5f, 0.6f);
  g_imRenderer->Begin(PRIM_LINE_LOOP);
  g_imRenderer->Vertex2f(boxX, boxY);
  g_imRenderer->Vertex2f(boxX + boxW, boxY);
  g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
  g_imRenderer->Vertex2f(boxX, boxY + boxH);
  g_imRenderer->End();


  g_imRenderer->Color4f(0.0f, 0.0f, 0.5f, 0.2f);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->Vertex2f(boxX, boxY);
  g_imRenderer->Vertex2f(boxX + boxW, boxY);
  g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
  g_imRenderer->Vertex2f(boxX, boxY + boxH);
  g_imRenderer->End();

}

void TaskManagerInterfaceIcons::RenderObjectives()
{
  SetupRenderMatrices(ScreenObjectives);

  //
  // Render title

  g_gameFont.SetRenderOutline(true);
  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_objectives"));
  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_gameFont.SetRenderOutline(false);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_objectives"));

  //
  // Render background fill

  float panelW = 81;
  float textX = 50;
  float completeX = m_screenW - 300;
  float textY = 100;
  float textH = 25;

  float boxX = 30;
  float boxW = m_screenW - boxX * 2 - panelW;

  //
  // Render primary objectives

  float mouseX = g_target->X();
  float mouseY = g_target->Y();

  ConvertMousePosition(mouseX, mouseY);

  for (int o = 0; o < 2; ++o)
  {
    LList<GlobalEventCondition*>* objectives = nullptr;
    if (o == 0)
      objectives = &g_app->m_location->m_levelFile->m_primaryObjectives;
    else
      objectives = &g_app->m_location->m_levelFile->m_secondaryObjectives;

    int numObjectives = objectives->Size();
    if (numObjectives == 0)
      continue;

    //
    // Title bar

    float boxY = textY - textH;
    float boxH = numObjectives * (textH * 1.5f) + textH * 0.75f;
    float titleHeight = 20.0f;

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Color4ub(199, 214, 220, 255);
    g_imRenderer->Vertex2f(boxX, boxY - titleHeight);
    g_imRenderer->Vertex2f(boxX + boxW, boxY - titleHeight);
    g_imRenderer->Color4ub(112, 141, 168, 255);
    g_imRenderer->Vertex2f(boxX + boxW, boxY);
    g_imRenderer->Vertex2f(boxX, boxY);
    g_imRenderer->End();

    g_imRenderer->Color4ub(199, 214, 220, 255);
    g_imRenderer->Color4ub(112, 141, 168, 255);

    g_gameFont.SetRenderShadow(true);
    g_imRenderer->Color4ub(255, 255, 150, 30);
    const char* theString = (o == 0 ? LANGUAGEPHRASE("taskmanager_primarys") : LANGUAGEPHRASE("taskmanager_secondarys"));
    g_gameFont.DrawText2DCentre(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
    g_gameFont.DrawText2DCentre(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
    g_gameFont.SetRenderShadow(false);

    //
    // Background box

    g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/interface_red.bmp"));

    g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.9f);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2f(0, 0);
    g_imRenderer->Vertex2f(boxX, boxY);
    g_imRenderer->TexCoord2f(1, 0);
    g_imRenderer->Vertex2f(boxX + boxW, boxY);
    g_imRenderer->TexCoord2f(1, boxH / m_screenH * 1.5);
    g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
    g_imRenderer->TexCoord2f(0, boxH / m_screenH * 1.5);
    g_imRenderer->Vertex2f(boxX, boxY + boxH);
    g_imRenderer->End();


    g_imRenderer->UnbindTexture();

    g_imRenderer->Color4ub(199, 214, 220, 255);
    g_imRenderer->Begin(PRIM_LINE_LOOP);
    g_imRenderer->Vertex2f(boxX, boxY - titleHeight);
    g_imRenderer->Vertex2f(boxX + boxW, boxY - titleHeight);
    g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
    g_imRenderer->Vertex2f(boxX, boxY + boxH);
    g_imRenderer->End();


    for (int i = 0; i < numObjectives; ++i)
    {
      float objectiveId = i + 10;
      if (o == 1)
        objectiveId *= -1;
      auto zone = new ScreenZone("Objective", LANGUAGEPHRASE("help_explainobjective"), boxX + 10, m_screenH + textY - textH / 2, boxW - 20,
                                 textH * 1.5f, objectiveId);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      GlobalEventCondition* condition = objectives->GetData(i);
      bool completed = condition->Evaluate();

      char* descriptor = LANGUAGEPHRASE(condition->m_stringId);

      g_gameFont.SetRenderOutline(true);
      g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(textX, textY, textH * 0.65f, "%s", descriptor);
      g_gameFont.SetRenderOutline(false);
      if (completed)
      {
        g_imRenderer->Color4f(0.8f, 0.8f, 1.0f, 1.0f);
      }
      else
      {
        g_imRenderer->Color4f(1.0f, 0.2f, 0.2f, 1.0f);
      }
      g_gameFont.DrawText2D(textX, textY, textH * 0.65f, "%s", descriptor);

      const char* completedString = (completed ? LANGUAGEPHRASE("taskmanager_complete") : LANGUAGEPHRASE("taskmanager_incomplete"));
      g_gameFont.SetRenderOutline(true);
      g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(completeX, textY, textH * 0.75f, completedString);
      g_gameFont.SetRenderOutline(false);
      if (completed)
      {
        g_imRenderer->Color4f(0.8f, 0.8f, 1.0f, 1.0f);
      }
      else
      {
        g_imRenderer->Color4f(1.0f, 0.2f, 0.2f, 1.0f);
      }
      g_gameFont.DrawText2D(completeX, textY, textH * 0.75f, completedString);

      if (condition->m_type == GlobalEventCondition::BuildingOnline)
      {
        Building* building = g_app->m_location->GetBuilding(condition->m_id);
        if (building)
        {
          char* objectiveCounter = building->GetObjectiveCounter();
          g_gameFont.DrawText2D(completeX, textY + textH * 0.75f, textH / 3, "%s", objectiveCounter);
        }
      }

      textY += textH * 1.5f;
    }

    textY += 70;
  }

  //
  // Render nav arrows

  if (m_screenY > 0.9f)
  {
    bool render360Controls = g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1);

    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    if (render360Controls)
      RenderIcon("icons/button_lb.bmp", "icons/button_lb_shadow.bmp", m_screenW - 60, 10, 40, alpha);
    else
    {
      g_imRenderer->Color4ub(199, 214, 220, alpha);

      g_imRenderer->Begin(PRIM_TRIANGLES);
      g_imRenderer->Vertex2f(m_screenW - 40, 10);
      g_imRenderer->Vertex2f(m_screenW - 20, 30);
      g_imRenderer->Vertex2f(m_screenW - 60, 30);
      g_imRenderer->End();

    }

    auto zoneLeft = new ScreenZone("ScreenUp", LANGUAGEPHRASE("newcontrols_showtaskmanager"), m_screenW - 60, m_screenH + 10, 40, 20, -1);

    m_newScreenZones.PutData(zoneLeft);
  }
}

void TaskManagerInterfaceIcons::RenderResearch()
{
  SetupRenderMatrices(ScreenResearch);

  //
  // Render title text

  g_gameFont.SetRenderOutline(true);
  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_research"));
  g_gameFont.SetRenderOutline(false);
  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
  g_gameFont.DrawText2DDown(m_screenW - 65, 100, 45, LANGUAGEPHRASE("taskmanager_research"));
  //g_gameFont.DrawText2DCentre( m_screenW/2.0f, 80, 20, "ResearchPoints : %d", g_app->m_globalWorld->m_research->m_researchPoints );

  float mouseX = g_target->X();
  float mouseY = g_target->Y();
  ConvertMousePosition(mouseX, mouseY);

  int numItemsResearched = 0;
  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(i))
      ++numItemsResearched;
  }

  //
  // Title bar

  float panelW = 80;
  float textH = 26;
  float boxX = 30;
  float boxW = m_screenW - boxX * 2 - panelW;
  float textY = 70;
  float boxY = textY - textH;
  float boxH = numItemsResearched * (textH * 1.7f) + textH * 1.0f;
  float titleHeight = 20.0f;

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Vertex2f(boxX, boxY - titleHeight);
  g_imRenderer->Vertex2f(boxX + boxW, boxY - titleHeight);
  g_imRenderer->Color4ub(112, 141, 168, 255);
  g_imRenderer->Vertex2f(boxX + boxW, boxY);
  g_imRenderer->Vertex2f(boxX, boxY);
  g_imRenderer->End();

  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Color4ub(112, 141, 168, 255);

  g_gameFont.SetRenderShadow(true);
  g_imRenderer->Color4ub(255, 255, 150, 30);
  const char* theString = LANGUAGEPHRASE("taskmanager_research");
  g_gameFont.DrawText2DCentre(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
  g_gameFont.DrawText2DCentre(boxX + boxW / 2.0f, boxY - titleHeight / 2 + 1, 13, theString);
  g_gameFont.SetRenderShadow(false);

  //
  // Background box

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/interface_red.bmp"));

  g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.8f);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2f(0, 0);
  g_imRenderer->Vertex2f(boxX, boxY);
  g_imRenderer->TexCoord2f(1, 0);
  g_imRenderer->Vertex2f(boxX + boxW, boxY);
  g_imRenderer->TexCoord2f(1, boxH / m_screenH * 1.5);
  g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
  g_imRenderer->TexCoord2f(0, boxH / m_screenH * 1.5);
  g_imRenderer->Vertex2f(boxX, boxY + boxH);
  g_imRenderer->End();


  g_imRenderer->UnbindTexture();

  g_imRenderer->Color4ub(199, 214, 220, 255);
  g_imRenderer->Begin(PRIM_LINE_LOOP);
  g_imRenderer->Vertex2f(boxX, boxY - titleHeight);
  g_imRenderer->Vertex2f(boxX + boxW, boxY - titleHeight);
  g_imRenderer->Vertex2f(boxX + boxW, boxY + boxH);
  g_imRenderer->Vertex2f(boxX, boxY + boxH);
  g_imRenderer->End();


  //
  // Render each item

  float iconY = textY - 20;
  float totalSize = (boxH - 20) / static_cast<float>(numItemsResearched);
  totalSize = min(totalSize, 50.0f);

  for (int i = 0; i < GlobalResearch::NumResearchItems; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(i))
    {
      float iconX = 50;
      float gapSize = 3;
      float iconSize = totalSize - gapSize;

      //
      // Render selection boxes

      if (g_app->m_globalWorld->m_research->m_currentResearch == i)
      {
        g_imRenderer->Color4f(0.05f, 0.05f, 0.5f, 0.4f);
        g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->Vertex2i(40, iconY - 1);
        g_imRenderer->Vertex2i(m_screenW - 120, iconY - 1);
        g_imRenderer->Vertex2i(m_screenW - 120, iconY + iconSize + 1);
        g_imRenderer->Vertex2i(40, iconY + iconSize + 1);
        g_imRenderer->End();

        g_imRenderer->Color4f(0.5f, 0.5f, 0.7f, 0.8f);
        g_imRenderer->Begin(PRIM_LINE_LOOP);
        g_imRenderer->Vertex2i(40, iconY - 1);
        g_imRenderer->Vertex2i(m_screenW - 120, iconY - 1);
        g_imRenderer->Vertex2i(m_screenW - 120, iconY + iconSize + 1);
        g_imRenderer->Vertex2i(40, iconY + iconSize + 1);
        g_imRenderer->End();

      }

      char tooltipId[256];
      sprintf(tooltipId, "newcontrols_research_%s", GlobalResearch::GetTypeName(i));
      auto zone = new ScreenZone("Research", LANGUAGEPHRASE(tooltipId), 40, -m_screenH + iconY, m_screenW - 160, iconSize, i);
      zone->m_scrollZone = 1;
      m_newScreenZones.PutData(zone);

      //
      // Render the shadow

      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
      g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
      g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
      g_imRenderer->Color4f(0.5f, 0.5f, 0.5f, 0.0f);

      float shadowSize = iconSize * 1.1f;

      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->TexCoord2i(0, 1);
      g_imRenderer->Vertex2f(iconX, iconY);
      g_imRenderer->TexCoord2i(1, 1);
      g_imRenderer->Vertex2f(iconX + shadowSize, iconY);
      g_imRenderer->TexCoord2i(1, 0);
      g_imRenderer->Vertex2f(iconX + shadowSize, iconY + shadowSize);
      g_imRenderer->TexCoord2i(0, 0);
      g_imRenderer->Vertex2f(iconX, iconY + shadowSize);
      g_imRenderer->End();


      g_imRenderer->UnbindTexture();

      //
      // Render the task symbol

      char iconFilename[256];
      sprintf(iconFilename, "icons/icon_%s.bmp", GlobalResearch::GetTypeName(i));
      unsigned int texId = g_app->m_resource->GetTexture(iconFilename);
      if (texId != -1)
      {
        g_imRenderer->BindTexture(texId);
        g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

        g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->TexCoord2i(0, 1);
        g_imRenderer->Vertex2f(iconX, iconY);
        g_imRenderer->TexCoord2i(1, 1);
        g_imRenderer->Vertex2f(iconX + iconSize, iconY);
        g_imRenderer->TexCoord2i(1, 0);
        g_imRenderer->Vertex2f(iconX + iconSize, iconY + iconSize);
        g_imRenderer->TexCoord2i(0, 0);
        g_imRenderer->Vertex2f(iconX, iconY + iconSize);
        g_imRenderer->End();


        g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
        g_imRenderer->UnbindTexture();
      }

      //
      // Render research progress

      int researchLevel = g_app->m_globalWorld->m_research->CurrentLevel(i);
      int researchProgress = g_app->m_globalWorld->m_research->CurrentProgress(i);

      g_gameFont.SetRenderOutline(true);
      g_imRenderer->Color4f(0.8f, 0.8f, 0.8f, 0.0f);
      g_gameFont.DrawText2D(iconX + iconSize + 10, iconY + iconSize / 2, 20, "%s", GlobalResearch::GetTypeNameTranslated(i));
      g_gameFont.SetRenderOutline(false);
      g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(iconX + iconSize + 10, iconY + iconSize / 2, 20, "%s", GlobalResearch::GetTypeNameTranslated(i));

      float boxX = 300;
      float boxY = iconY + iconSize * 0.25f;
      float boxH = iconSize * 0.4f;
      float boxScale = 0.85f;

      g_imRenderer->BindTexture(g_app->m_resource->GetTexture("textures/interface_grey.bmp"));

      for (int level = 1; level < 4; ++level)
      {
        int requiredProgress = g_app->m_globalWorld->m_research->RequiredProgress(level);

        float shadowOffset = 2.0f;
        g_imRenderer->Color4f(0.0f, 0.0f, 0.0f, 0.5f);
        g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->Vertex2i(boxX, boxY);
        g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale + shadowOffset, boxY);
        g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale + shadowOffset, boxY + boxH + shadowOffset);
        g_imRenderer->Vertex2i(boxX, boxY + boxH + shadowOffset);
        g_imRenderer->End();


        if (researchLevel > level)
        {
          g_imRenderer->Color4f(0.7f, 0.7f, 0.9f, 1.0f);
          g_imRenderer->Begin(PRIM_QUADS);
          g_imRenderer->TexCoord2i(0, 0);
          g_imRenderer->Vertex2i(boxX, boxY);
          g_imRenderer->TexCoord2i(10, 0);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY);
          g_imRenderer->TexCoord2i(10, 1);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY + boxH);
          g_imRenderer->TexCoord2i(0, 1);
          g_imRenderer->Vertex2i(boxX, boxY + boxH);
          g_imRenderer->End();

          float alpha = 1.0f;
          if (g_app->m_globalWorld->m_research->m_currentResearch == i)
            alpha = fabs(sinf(g_gameTime * 2));
          g_imRenderer->Color4f(0.9f, 0.9f, 1.0f, alpha);
          g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
          g_imRenderer->Begin(PRIM_QUADS);
          g_imRenderer->TexCoord2i(0, 0);
          g_imRenderer->Vertex2i(boxX, boxY);
          g_imRenderer->TexCoord2i(10, 0);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY);
          g_imRenderer->TexCoord2i(10, 1);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY + boxH);
          g_imRenderer->TexCoord2i(0, 1);
          g_imRenderer->Vertex2i(boxX, boxY + boxH);
          g_imRenderer->TexCoord2i(0, 0);
          g_imRenderer->Vertex2i(boxX, boxY);
          g_imRenderer->TexCoord2i(10, 0);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY);
          g_imRenderer->TexCoord2i(10, 1);
          g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY + boxH);
          g_imRenderer->TexCoord2i(0, 1);
          g_imRenderer->Vertex2i(boxX, boxY + boxH);
          g_imRenderer->End();

          g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
          g_imRenderer->UnbindTexture();
        }

        if (researchLevel == level && researchProgress > 0)
        {
          g_imRenderer->Color4f(0.7f, 0.7f, 1.0f, 1.0f);
          g_imRenderer->Begin(PRIM_QUADS);
          g_imRenderer->TexCoord2i(0, 0);
          g_imRenderer->Vertex2i(boxX, boxY);
          g_imRenderer->TexCoord2i(10, 0);
          g_imRenderer->Vertex2i(boxX + researchProgress * boxScale, boxY);
          g_imRenderer->TexCoord2i(10, 1);
          g_imRenderer->Vertex2i(boxX + researchProgress * boxScale, boxY + boxH);
          g_imRenderer->TexCoord2i(0, 1);
          g_imRenderer->Vertex2i(boxX, boxY + boxH);
          g_imRenderer->End();

          if (g_app->m_globalWorld->m_research->m_currentResearch == i)
          {
            float alpha = fabs(sinf(g_gameTime * 2));
            g_imRenderer->Color4f(0.9f, 0.9f, 1.0f, alpha);
            g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
            g_imRenderer->Begin(PRIM_QUADS);
            g_imRenderer->TexCoord2i(0, 0);
            g_imRenderer->Vertex2i(boxX, boxY);
            g_imRenderer->TexCoord2i(10, 0);
            g_imRenderer->Vertex2i(boxX + researchProgress * boxScale, boxY);
            g_imRenderer->TexCoord2i(10, 1);
            g_imRenderer->Vertex2i(boxX + researchProgress * boxScale, boxY + boxH);
            g_imRenderer->TexCoord2i(0, 1);
            g_imRenderer->Vertex2i(boxX, boxY + boxH);
            g_imRenderer->End();

            g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
          }

          g_imRenderer->UnbindTexture();
        }

        g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.8f);
        g_imRenderer->Begin(PRIM_LINE_LOOP);
        g_imRenderer->Vertex2i(boxX, boxY);
        g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY);
        g_imRenderer->Vertex2i(boxX + requiredProgress * boxScale, boxY + boxH);
        g_imRenderer->Vertex2i(boxX, boxY + boxH);
        g_imRenderer->End();


        boxX += requiredProgress * boxScale;
        boxX += 3;
      }

      float texX = 630;

      g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
      g_gameFont.DrawText2D(texX, iconY + iconSize / 2, 12, "v%d.0", researchLevel);

      iconY += totalSize;
    }
  }

  //
  // Render navigation arrows

  if (m_screenY < 0.1f)
  {
    bool render360Controls = g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && g_prefsManager->GetInt(OTHER_CONTROLHELPENABLED, 1);

    unsigned alpha = (fmodf(g_gameTime, 2.0f) < 1.0f ? 155 : 255);

    if (render360Controls)
      RenderIcon("icons/button_rb.bmp", "icons/button_rb_shadow.bmp", m_screenW - 60, m_screenH - 70, 40, alpha);
    else
    {
      g_imRenderer->Color4ub(199, 214, 220, alpha);

      g_imRenderer->Begin(PRIM_TRIANGLES);
      g_imRenderer->Vertex2f(m_screenW - 40, m_screenH - 30);
      g_imRenderer->Vertex2f(m_screenW - 20, m_screenH - 50);
      g_imRenderer->Vertex2f(m_screenW - 60, m_screenH - 50);
      g_imRenderer->End();

    }

    auto zoneRight = new ScreenZone("ScreenDown", LANGUAGEPHRASE("newcontrols_showtaskmanager"), m_screenW - 60, -50, 40, 20, -1);
    zoneRight->m_scrollZone = -1;
    m_newScreenZones.PutData(zoneRight);
  }
}

bool TaskManagerInterfaceIcons::ControlEvent(TMControl _type)
{
  switch (_type)
  {
  case TMTerminate:
    return g_inputManager->controlEvent(ControlIconsTaskManagerEndTask);
  case TMDisplay:
    return g_inputManager->controlEvent(ControlIconsTaskManagerDisplay);
  default:
    return false;
  }
}

void TaskManagerInterfaceIcons::AdvanceQuickUnit()
{
  if (!m_quickUnitVisible)
    return;

  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeSquad, GlobalResearch::TypeEngineer, GlobalResearch::TypeOfficer, GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  if (numAvailable == 0)
  {
    m_quickUnitVisible = false;
    return;
  }

  for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
    m_quickUnitButtons[i]->Advance();

  bool showUnitName = false;

  if (numAvailable > 1 && m_quickUnitDirection == 0)
  {
    InputDetails cameraDetails, targetDetails;
    g_inputManager->controlEvent(ControlTargetMove, targetDetails);
    g_inputManager->controlEvent(ControlCameraMove, cameraDetails);

    bool right = (g_inputManager->controlEvent(ControlUnitCycleLeft) || cameraDetails.x < -10 || targetDetails.x < -10 || g_inputManager->
      controlEvent(ControlIconsTaskManagerQuickUnitLeft));

    bool left = (g_inputManager->controlEvent(ControlUnitCycleRight) || cameraDetails.x > 10 || targetDetails.x > 10 || g_inputManager->
      controlEvent(ControlIconsTaskManagerQuickUnitRight));

    if (left)
    {
      while (true)
      {
        m_currentQuickUnit--;
        if (m_currentQuickUnit < 0)
          m_currentQuickUnit = numRunnableTasks - 1;
        if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[m_currentQuickUnit]))
          break;
      }

      for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
        m_quickUnitButtons[i]->m_movable = true;
      m_quickUnitDirection = -1;
      showUnitName = true;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (right)
    {
      while (true)
      {
        m_currentQuickUnit++;
        if (m_currentQuickUnit >= numRunnableTasks)
          m_currentQuickUnit = 0;
        if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[m_currentQuickUnit]))
          break;
      }

      for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
        m_quickUnitButtons[i]->m_movable = true;
      m_quickUnitDirection = 1;
      showUnitName = true;
      g_app->m_soundSystem->TriggerOtherEvent(nullptr, "Slide", SoundSourceBlueprint::TypeInterface);
    }

    if (showUnitName)
    {
      int taskId = GetQuickUnitTask(2 - m_quickUnitDirection);
      if (taskId != -1)
        g_app->m_taskManagerInterface->SetCurrentMessage(MessageSuccess, taskId, 3.0f);
    }

    if (g_inputManager->controlEvent(ControlIconsTaskManagerQuickUnitCreate))
    {
      int taskId = GetQuickUnitTask();
      if (taskId != -1)
      {
        g_app->m_clientToServer->RequestRunProgram(g_app->m_location->GetMyTeam()->m_teamId, taskId);
        g_app->m_soundSystem->TriggerOtherEvent(nullptr, "GestureBegin", SoundSourceBlueprint::TypeGesture);
        DestroyQuickUnitInterface();
      }
    }
  }
  else if (m_quickUnitDirection != 0)
  {
    bool stop = true;
    for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
    {
      if (m_quickUnitButtons[i]->m_movable)
        stop = false;
    }
    if (stop)
      m_quickUnitDirection = 0;
  }
}

void TaskManagerInterfaceIcons::RenderQuickUnit()
{
  if (!m_quickUnitVisible)
    return;

  float iconSize = 110;
  float iconGap = 10;
  float iconX = (g_app->m_renderer->ScreenW() / 2.0f);
  float iconY = g_app->m_renderer->ScreenH() - iconSize;
  float iconAlpha = 0.9f;

  Vector2 iconCentre(iconX, iconY);

  float shadowOffset = 0;
  float shadowSize = iconSize;
  //float totalWidth = (numSlots-1) * ( iconSize + iconGap );   */

  for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
    m_quickUnitButtons[i]->Render();

  char shadowFileName[256];
  sprintf(shadowFileName, "shadow_icons/mouse_selection.bmp");

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture(shadowFileName));
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_imRenderer->Color4f(iconAlpha, iconAlpha, iconAlpha, 0.0f);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
  g_imRenderer->End();


  char bmpFilename[256];
  sprintf(bmpFilename, "icons/mouse_selection.bmp");
  unsigned int texId = g_app->m_resource->GetTexture(bmpFilename);

  g_imRenderer->BindTexture(texId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

  g_imRenderer->Color4f(1.0f, 1.0f, 0.0f, iconAlpha);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y - iconSize / 2);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y - iconSize / 2);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y + iconSize / 2);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y + iconSize / 2);
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_imRenderer->UnbindTexture();
}

int TaskManagerInterfaceIcons::GetQuickUnitTask(int _position)
{
  for (int i = 0; i < m_quickUnitButtons.Size(); ++i)
  {
    if (m_quickUnitButtons[i]->m_positionId == _position)
      return m_quickUnitButtons[i]->m_taskId;
  }
  return -1;
}

bool TaskManagerInterfaceIcons::AdviseCreateControlHelpBlue()
{
  return m_currentScreenZone != -1 && ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayPressed) && g_app->
    m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity();
}

bool TaskManagerInterfaceIcons::AdviseCreateControlHelpGreen()
{
  ScreenZone* screenZone = nullptr;

  return m_currentScreenZone != -1 && g_app->m_taskManager->CapacityUsed() < g_app->m_taskManager->Capacity() && (screenZone = m_screenZones
    [m_currentScreenZone]) && strcmp(screenZone->m_name, "NewTask") == 0;
}

bool TaskManagerInterfaceIcons::AdviseOverSelectableZone()
{
  ScreenZone* screenZone = nullptr;

  return m_currentScreenZone != -1 && (screenZone = m_screenZones[m_currentScreenZone]) && strcmp(screenZone->m_name, "NewTask") != 0;
}

bool TaskManagerInterfaceIcons::AdviseCloseControlHelp()
{
  if (!m_visible)
    return false;

  if (m_currentScreenZone != -1 && ButtonHeld() && g_inputManager->controlEvent(ControlIconsTaskManagerDisplayPressed))
    return false;

  return true;
}

void TaskManagerInterfaceIcons::CreateQuickUnitInterface()
{
  int numRunnableTasks = 4;
  int runnableTaskType[] = {
    GlobalResearch::TypeOfficer, GlobalResearch::TypeEngineer, GlobalResearch::TypeSquad, GlobalResearch::TypeArmour
  };

  int numAvailable = 0;
  for (int i = 0; i < numRunnableTasks; ++i)
  {
    if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      numAvailable++;
  }

  while (m_quickUnitButtons.Size() < 5)
  {
    for (int i = 0; i < numRunnableTasks; ++i)
    {
      if (g_app->m_globalWorld->m_research->HasResearch(runnableTaskType[i]))
      {
        bool buttonFound = false;
        for (int j = 0; j < m_quickUnitButtons.Size(); ++j)
        {
          if (m_quickUnitButtons[j]->m_taskId == runnableTaskType[i])
          {
            buttonFound = true;
            break;
          }
        }

        if (!buttonFound || (buttonFound && m_quickUnitButtons.Size() >= numAvailable && m_quickUnitButtons.Size() < 5))
        {
          auto button = new QuickUnitButton();
          button->m_taskId = runnableTaskType[i];
          m_quickUnitButtons.PutData(button);
        }
      }

      if (m_quickUnitButtons.Size() == 5)
        break;
    }
  }
  m_quickUnitVisible = true;
  if (m_currentQuickUnit == -1)
    m_currentQuickUnit = 0;
  g_app->m_location->GetMyTeam()->SelectUnit(-1, -1, -1);
}

void TaskManagerInterfaceIcons::DestroyQuickUnitInterface()
{
  m_quickUnitButtons.EmptyAndDelete();
  m_quickUnitVisible = false;
  m_currentQuickUnit = -1;
  m_quickUnitDirection = 0;
}

QuickUnitButton::QuickUnitButton()
  : m_taskId(-1),
    m_positionId(-1),
    m_size(0.0f),
    m_x(-1),
    m_y(-1),
    m_alpha(0.9f),
    m_movable(false)
{
  float iconSize = 100.0f;
  float iconGap = 10.0f;
  float iconX = (g_app->m_renderer->ScreenW() / 2.0f); // - iconSize - iconGap;
  float iconY = g_app->m_renderer->ScreenH() - iconSize - iconGap;

  static Vector2 buttonPositions[5] = {
    Vector2(iconX - iconSize * 2 - iconGap * 2, iconY), Vector2(iconX - iconSize - iconGap, iconY), Vector2(iconX, iconY),
    Vector2(iconX + iconSize + iconGap, iconY), Vector2(iconX + iconSize * 2 + iconGap * 2, iconY)
  };

  auto tm = static_cast<TaskManagerInterfaceIcons*>(g_app->m_taskManagerInterface);
  m_positionId = tm->m_quickUnitButtons.Size();
  if (m_positionId == 0 || m_positionId == 4)
    m_alpha = 0.0f;

  m_x = buttonPositions[m_positionId].x;
  m_y = buttonPositions[m_positionId].y;
}

void QuickUnitButton::Advance()
{
  auto tm = static_cast<TaskManagerInterfaceIcons*>(g_app->m_taskManagerInterface);
  int direction = tm->m_quickUnitDirection;

  if (direction == 0)
    return;

  float iconSize = 100.0f;
  float iconGap = 10.0f;
  float iconX = (g_app->m_renderer->ScreenW() / 2.0f); // - iconSize - iconGap;
  float iconY = g_app->m_renderer->ScreenH() - iconSize - iconGap;

  static Vector2 buttonPositions[5] = {
    Vector2(iconX - iconSize * 2 - iconGap * 2, iconY), Vector2(iconX - iconSize - iconGap, iconY), Vector2(iconX, iconY),
    Vector2(iconX + iconSize + iconGap, iconY), Vector2(iconX + iconSize * 2 + iconGap * 2, iconY)
  };

  if (direction != 0 && m_movable)
  {
    int nextPos = m_positionId + direction;
    if (nextPos < 0)
      nextPos = 4;
    if (nextPos > 4)
      nextPos = 0;

    if (nextPos == 0 || nextPos == 4)
    {
      int distance = m_x - buttonPositions[nextPos].x;
      if (abs(distance) > iconSize + iconGap)
        m_x = buttonPositions[nextPos].x - direction * iconSize;

      if (nextPos == 0 && direction == -1)
        m_alpha -= 0.09f;
      else if (nextPos == 4 && direction == 1)
        m_alpha -= 0.09f;
    }
    else if (nextPos == 1 && direction == 1)
    {
      m_alpha += 0.09f;
      m_alpha = min(m_alpha, 0.9f);
    }
    else if (nextPos == 3 && direction == -1)
    {
      m_alpha += 0.09f;
      m_alpha = min(m_alpha, 0.9f);
    }

    m_x += direction * 10;

    if (m_x == buttonPositions[nextPos].x)
    {
      m_movable = false;
      m_positionId = nextPos;
      m_x = buttonPositions[m_positionId].x;
    }
  }
}

void QuickUnitButton::Render()
{
  //if( m_positionId == 0 || m_positionId == 4 ) return;

  float iconSize = 100.0f;
  float iconGap = 10.0f;
  float iconX = (g_app->m_renderer->ScreenW() / 2.0f); // - iconSize - iconGap;
  float iconY = g_app->m_renderer->ScreenH() - iconSize - iconGap;
  float iconAlpha = m_alpha;
  float shadowOffset = 0;
  float shadowSize = iconSize + 10;

  auto iconCentre = Vector2(m_x, m_y);

  g_imRenderer->BindTexture(g_app->m_resource->GetTexture("icons/icon_shadow.bmp"));
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_imRenderer->Color4f(iconAlpha, iconAlpha, iconAlpha, 0.0f);
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y - shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(iconCentre.x + shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(iconCentre.x - shadowSize / 2 + shadowOffset, iconCentre.y + shadowSize / 2 + shadowOffset);
  g_imRenderer->End();


  char bmpFilename[256];
  if (m_taskId == -1)
    sprintf(bmpFilename, "icons/icon_notask.bmp");
  else
    sprintf(bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(m_taskId));
  unsigned int texId = g_app->m_resource->GetTexture(bmpFilename);

  g_imRenderer->BindTexture(texId);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, iconAlpha);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2i(0, 1);
  g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y - iconSize / 2);
  g_imRenderer->TexCoord2i(1, 1);
  g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y - iconSize / 2);
  g_imRenderer->TexCoord2i(1, 0);
  g_imRenderer->Vertex2f(iconCentre.x + iconSize / 2, iconCentre.y + iconSize / 2);
  g_imRenderer->TexCoord2i(0, 0);
  g_imRenderer->Vertex2f(iconCentre.x - iconSize / 2, iconCentre.y + iconSize / 2);
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_imRenderer->UnbindTexture();
}
