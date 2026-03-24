#include "pch.h"
#include "gamecursor_2d.h"
#include "GameApp.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "camera.h"
#include "gamecursor.h"
#include "global_world.h"
#include "main.h"
#include "profiler.h"
#include "renderer.h"
#include "resource.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"

GameCursor2D::GameCursor2D(GameCursor& _parent)
  : m_parent(_parent),
    m_updatedArrowBoost(0.0f)
{
  m_cursorStandard = new MouseCursor("icons/mouse_main.bmp");
  m_cursorStandard->SetHotspot(0.055f, 0.070f);
  m_cursorStandard->SetSize(25.0f);

  m_cursorTurretTarget = new MouseCursor("icons/mouse_turrettarget.bmp");
  m_cursorTurretTarget->SetHotspot(0.5f, 0.5f);
  m_cursorTurretTarget->SetColour(RGBAColour(255, 255, 255, 255));
  m_cursorTurretTarget->SetShadowed(false);

  m_cursorSelection = new MouseCursor("icons/mouse_selection.bmp");
  m_cursorSelection->SetHotspot(0.5f, 0.5f);

  //
  // Load selection arrow graphic

  snprintf(m_selectionArrowFilename, sizeof(m_selectionArrowFilename), "icons/selectionarrow.bmp");

  BinaryReader* binReader = Resource::GetBinaryReader(m_selectionArrowFilename);
  ASSERT_TEXT(binReader, "Failed to open mouse cursor resource %s", m_selectionArrowFilename);
  BitmapRGBA bmp(binReader, "bmp");
  SAFE_DELETE(binReader);

  Resource::AddBitmap(m_selectionArrowFilename, bmp);

  snprintf(m_selectionArrowShadowFilename, sizeof(m_selectionArrowShadowFilename), "shadow_%s", m_selectionArrowFilename);
  bmp.ApplyBlurFilter(10.0f);
  Resource::AddBitmap(m_selectionArrowShadowFilename, bmp);
}

GameCursor2D::~GameCursor2D()
{
  SAFE_DELETE(m_cursorStandard);
  SAFE_DELETE(m_cursorTurretTarget);
  SAFE_DELETE(m_cursorSelection);
}

void GameCursor2D::Render()
{
  START_PROFILE(g_context->m_profiler, "Render GameCursor 2D");

  // Caller provides ortho matrix and 2D GL state (depth off, blend on, cull off).

  const CursorFrameState& frame = m_parent.GetFrameState();

  // Set mip mapping for cursor textures (2D pass)
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Copy arrow boost so RenderSelectionArrows can decrement it
  m_updatedArrowBoost = frame.selectionArrowBoost;

  if (frame.isEditing)
  {
    // Editing — no cursor
  }
  else if (!frame.isInteractive)
  {
    // Cut scene: suppress standard cursor (set in PrepareCursorState)
  }
  else if (!frame.inLocation)
  {
    // We are in the global world.
    // Use the cached query result from PrepareCursorState — GetHighlightedLocation
    // calls GetClickRay which needs the perspective projection matrix that is only
    // active during the 3D pass.
    bool locAvailable = frame.globalWorldLocAvailable;
    MouseCursor* cursorPlacement = m_parent.GetCursorPlacement();
    MouseCursor* cursorDisabled = m_parent.GetCursorDisabled();
    cursorPlacement->SetAnimation(locAvailable);
    cursorPlacement->SetSize(40.0f);
    cursorPlacement->Render(frame.screenX, frame.screenY);
    if (!locAvailable)
      cursorDisabled->Render(frame.screenX, frame.screenY);
  }
  else if (frame.inLocation)
  {
    if (g_context->m_taskManagerInterface->m_visible)
    {
      // Looking at the task manager — selection arrows + halo
      if (frame.somethingSelected && frame.selectedId.GetUnitId() != UNIT_BUILDINGS)
        RenderSelectionArrows();

      if (frame.somethingHighlighted)
      {
        m_cursorSelection->SetSize(frame.highlightedRadius * 100 / sqrt(frame.highlightedCamDist));
        m_cursorSelection->SetColour(RGBAColour(255, 255, 100, 255));
        m_cursorSelection->SetAnimation(false);
        m_cursorSelection->Render(frame.highlightedScreenX, g_context->m_renderer->ScreenH() - frame.highlightedScreenY);
      }
    }
    else if (frame.currentTask && frame.currentTask->m_state == Task::StateStarted && frame.currentTask->m_type !=
      GlobalResearch::TypeOfficer && !frame.somethingHighlighted)
    {
      // Placing a task — 3D path handles the cursor, nothing 2D here
    }
    else if (g_context->m_camera->IsInMode(Camera::ModeEntityTrack))
    {
      // Cursor suppression handled in PrepareCursorState
    }
    else
    {
      if (frame.somethingHighlighted && !(frame.somethingSelected && frame.highlightedId.GetUnitId() == UNIT_BUILDINGS))
      {
        if (frame.highlightedCamDist > 100 || !frame.somethingSelected || frame.selectedId != frame.highlightedId)
        {
          m_cursorSelection->SetSize(frame.highlightedRadius * 100 / sqrt(frame.highlightedCamDist));
          m_cursorSelection->SetColour(RGBAColour(255, 255, 100, 255));
          m_cursorSelection->SetAnimation(false);
          m_cursorSelection->Render(frame.highlightedScreenX, g_context->m_renderer->ScreenH() - frame.highlightedScreenY);
        }
      }

      if (frame.somethingSelected && frame.selectedId.GetUnitId() != UNIT_BUILDINGS)
      {
        RenderSelectionArrows();

        // Radar dish entrance cursor (pre-computed in PrepareCursorState)
        if (frame.radarDishEntranceValid)
        {
          MouseCursor* cursorPlacement = m_parent.GetCursorPlacement();
          cursorPlacement->SetSize(60.0f);
          cursorPlacement->SetAnimation(true);
          cursorPlacement->Render(frame.radarDishEntranceScreenX, g_context->m_renderer->ScreenH() - frame.radarDishEntranceScreenY);
        }
      }

      if (frame.somethingSelected && frame.selectedId.GetUnitId() == UNIT_BUILDINGS)
      {
        // Selected a building — render a targeting crosshair
        m_cursorTurretTarget->SetSize(200.0f);
        m_cursorTurretTarget->Render(frame.screenX, frame.screenY);
      }
    }
  }

  if (!frame.suppressStandardCursor && g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD)
  {
    // Nobody has drawn a cursor yet — give us the default
    RenderStandardCursor(frame.screenX, frame.screenY);
  }

  END_PROFILE(g_context->m_profiler, "Render GameCursor 2D");
}

void GameCursor2D::RenderStandardCursor(float _screenX, float _screenY) { m_cursorStandard->Render(_screenX, _screenY); }

void GameCursor2D::RenderSelectionArrows()
{
  const CursorFrameState& frame = m_parent.GetFrameState();

  // Alive checks pre-computed in PrepareCursorState
  if (!frame.selectionArrowsValid)
    return;

  float triSize = 40.0f;

  int screenH = g_context->m_renderer->ScreenH();

  // Read pre-computed screen position from frame state
  // (Get2DScreenPos was called in PrepareCursorState while perspective matrices were active)
  float screenX = frame.selectedScreenX;
  float screenY = frame.selectedScreenY;

  if (frame.selectedOnScreen)
  {
    // _pos is onscreen
    LegacyVector3 toCam = g_context->m_camera->GetPos() - frame.selectedWorldPos;
    float camDist = toCam.Mag();
    m_updatedArrowBoost -= g_advanceTime * 0.4f;

    float distanceOut = 1000 / sqrtf(camDist);
    float alpha = std::min(m_updatedArrowBoost, 0.9f);

    if (camDist > 200.0f)
      alpha = std::max(std::min((camDist - 200.0f) / 200.0f, 0.9f), alpha);
    RenderSelectionArrow(screenX, screenY - distanceOut, 0, -1, triSize, alpha);
    RenderSelectionArrow(screenX, screenY + distanceOut, 0, 1, triSize, alpha);
    RenderSelectionArrow(screenX - distanceOut, screenY, -1, 0, triSize, alpha);
    RenderSelectionArrow(screenX + distanceOut, screenY, 1, 0, triSize, alpha);
  }
  else
  {
    // _pos is offscreen — read pre-computed edge data from frame state
    RenderSelectionArrow(frame.selectedEdgeX, screenH - frame.selectedEdgeY, frame.selectedEdgeNormal.x, frame.selectedEdgeNormal.y,
                         triSize * 1.5f, 0.9f);
  }
}

void GameCursor2D::RenderSelectionArrow(float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha)
{
  Vector2 pos(_screenX, _screenY);
  Vector2 gradient(_screenDX, _screenDY);
  Vector2 rightAngle = gradient;
  float tempX = rightAngle.x;
  rightAngle.x = rightAngle.y;
  rightAngle.y = tempX * -1;

  // Per-draw-call state only — pass-level state (blend, cull, depth) owned by outer 2D pass.

  glColor4f(_alpha, _alpha, _alpha, 0.0f);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_selectionArrowShadowFilename));

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2fv((pos - rightAngle * _size / 2.0f).GetData());
  glTexCoord2i(0, 0);
  glVertex2fv((pos - rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 0);
  glVertex2fv((pos + rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 1);
  glVertex2fv((pos + rightAngle * _size / 2.0f).GetData());
  glEnd();

  glColor4f(1.0f, 1.0f, 0.3f, _alpha);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBindTexture(GL_TEXTURE_2D, Resource::GetTexture(m_selectionArrowFilename));

  glBegin(GL_QUADS);
  glTexCoord2i(0, 1);
  glVertex2fv((pos - rightAngle * _size / 2.0f).GetData());
  glTexCoord2i(0, 0);
  glVertex2fv((pos - rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 0);
  glVertex2fv((pos + rightAngle * _size / 2.0f + gradient * _size).GetData());
  glTexCoord2i(1, 1);
  glVertex2fv((pos + rightAngle * _size / 2.0f).GetData());
  glEnd();

  glDisable(GL_TEXTURE_2D);
}
