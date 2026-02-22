#include "pch.h"
#include "input.h"
#include "win32_eventhandler.h"
#include "profiler.h"
#include "text_renderer.h"
#include "hi_res_time.h"
#include "language_table.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"
#include "startsequence.h"
#include "app.h"
#include "camera.h"
#include "user_input.h"
#include "renderer.h"
#include "global_world.h"
#include "soundsystem.h"

StartSequence::StartSequence()
{
  m_startTime = GetHighResTime();

  float screenRatio = static_cast<float>(g_app->m_renderer->ScreenH()) / static_cast<float>(g_app->m_renderer->ScreenW());
  int screenH = 800 * screenRatio;

  float x = 150;
  float y = screenH * 4 / 5.0f;
  float size = 10.0f;

  RegisterCaption(LANGUAGEPHRASE("intro_1"), x, y, size, 3, 15);
  RegisterCaption(LANGUAGEPHRASE("intro_2"), x, y + 15, 20, 8, 15);
  RegisterCaption(LANGUAGEPHRASE("intro_3"), x, y + 40, size, 30, 45);
  RegisterCaption(LANGUAGEPHRASE("intro_4"), x, y + 50, size, 42, 45);
  RegisterCaption(LANGUAGEPHRASE("intro_5"), x, y, size, 54, 65);
  RegisterCaption(LANGUAGEPHRASE("intro_6"), x, y, size, 66, 74);
  RegisterCaption(LANGUAGEPHRASE("intro_7"), x, y + 15, size, 72, 74);
  RegisterCaption(LANGUAGEPHRASE("intro_8"), x, y, size, 74, 90);
  RegisterCaption(LANGUAGEPHRASE("intro_9"), x, y + 15, 15, 82, 90);
  RegisterCaption(LANGUAGEPHRASE("intro_10"), x, y + 30, 15, 86, 90);
}

void StartSequence::RegisterCaption(char* _caption, float _x, float _y, float _size, float _startTime, float _endTime)
{
  auto caption = new StartSequenceCaption();

  caption->m_caption = strdup(_caption);
  caption->m_x = _x;
  caption->m_y = _y;
  caption->m_size = _size;
  caption->m_startTime = _startTime;
  caption->m_endTime = _endTime;

  m_captions.PutData(caption);
}

bool StartSequence::Advance()
{
  static bool started = false;
  if (GetHighResTime() > m_startTime && !started)
  {
    started = true;
    g_app->m_soundSystem->TriggerOtherEvent(nullptr, "StartSequence", SoundSourceBlueprint::TypeMusic);
    g_app->m_camera->SetDebugMode(Camera::DebugModeAuto);
    g_app->m_camera->RequestMode(Camera::ModeSphereWorldIntro);
  }

  g_inputManager->PollForEvents();

  if (!g_eventHandler->WindowHasFocus())
  {
    Sleep(1);
    g_app->m_userInput->Advance();
    return false;
  }

  if (g_inputManager->controlEvent(ControlSkipMessage) || g_app->m_requestQuit || (GetHighResTime() - m_startTime) > 90)
  {
    g_app->m_soundSystem->StopAllSounds(WorldObjectId(), "Music StartSequence");
    return true;
  }

  g_app->m_userInput->Advance();
  g_app->m_camera->Advance();
  g_app->m_soundSystem->Advance();
#ifdef PROFILER_ENABLED
  g_app->m_profiler->Advance();
#endif // PROFILER_ENABLED

  g_app->m_renderer->Render();

  return false;
}

void StartSequence::Render()
{
  float screenRatio = static_cast<float>(g_app->m_renderer->ScreenH()) / static_cast<float>(g_app->m_renderer->ScreenW());
  int screenH = 800 * screenRatio;

  g_imRenderer->SetProjectionMatrix(XMMatrixOrthographicOffCenterRH(0, 800, static_cast<float>(screenH), 0, -1, 1));
  g_imRenderer->SetViewMatrix(XMMatrixIdentity());
  g_imRenderer->LoadIdentity();

  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

  float timeNow = GetHighResTime() - m_startTime;

  if (timeNow < 3.0f)
  {
    float alpha = 1.0f - timeNow / 3.0f;
    g_imRenderer->Color4f(0, 0, 0, alpha);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2i(0, 0);
    g_imRenderer->Vertex2i(800, 0);
    g_imRenderer->Vertex2i(800, screenH);
    g_imRenderer->Vertex2i(0, screenH);
    g_imRenderer->End();
  }

  if (timeNow > 87)
  {
    float alpha = (timeNow - 87) / 2.0f;
    g_imRenderer->Color4f(1, 1, 1, alpha);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2i(0, 0);
    g_imRenderer->Vertex2i(800, 0);
    g_imRenderer->Vertex2i(800, screenH);
    g_imRenderer->Vertex2i(0, screenH);
    g_imRenderer->End();
  }

  Vector2 cursorPos;
  bool cursorFlash = false;
  float cursorSize = 0.0f;

  for (int i = 0; i < m_captions.Size(); ++i)
  {
    StartSequenceCaption* caption = m_captions[i];
    if (timeNow >= caption->m_startTime && timeNow <= caption->m_endTime)
    {
      char theString[256];
      sprintf(theString, caption->m_caption);
      int stringLength = strlen(theString);
      int maxTimeLength = (timeNow - caption->m_startTime) * 20;
      if (maxTimeLength < stringLength)
        theString[maxTimeLength] = '\x0';

      g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 0.8f);
      g_gameFont.DrawText2D(caption->m_x, caption->m_y, caption->m_size, theString);

      int finishedLen = strlen(theString);
      int texW = g_gameFont.GetTextWidth(finishedLen, caption->m_size);
      cursorPos.Set(caption->m_x + texW, caption->m_y - 7.25f);
      cursorFlash = maxTimeLength > stringLength;
      cursorSize = caption->m_size;
    }
  }

  if (cursorPos != Vector2(0, 0))
  {
    if (!cursorFlash || fmod(timeNow, 1) < 0.5f)
    {
      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->Vertex2f(cursorPos.x, cursorPos.y);
      g_imRenderer->Vertex2f(cursorPos.x + cursorSize * 0.7f, cursorPos.y);
      g_imRenderer->Vertex2f(cursorPos.x + cursorSize * 0.7f, cursorPos.y + cursorSize * 0.88f);
      g_imRenderer->Vertex2f(cursorPos.x, cursorPos.y + cursorSize * 0.88f);
      g_imRenderer->End();
    }
  }

  g_app->m_renderer->SetupMatricesFor3D();

  //
  // Render grid behind darwinia

  float scale = 1000.0f;

  float fog = 0.0f;
  float fogCol[] = {fog, fog, fog, fog};

  int fogVal = 5810000;

  float r = 2.0f;
  float height = -400.0f;
  float gridSize = 100.0f;

  float xStart = -4000.0f * r;
  float xEnd = 4000.0f + 4000.0f * r;
  float zStart = -4000.0f * r;
  float zEnd = 4000.0f + 4000.0f * r;

  float fogColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  if (timeNow > 50.0f)
  {
    g_imRenderer->PushMatrix();
    g_imRenderer->Scalef(scale, scale, scale);

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);

    g_imRenderer->Color4f(0.5, 0.5, 1.0, 0.5);

    float percentDrawn = 1.0f - (timeNow - 50.0f) / 10.0f;
    percentDrawn = max(percentDrawn, 0.0f);
    xEnd -= (8000 + 4000 * r * percentDrawn);
    zEnd -= (8000 + 4000 * r * percentDrawn);

    for (int x = xStart; x < xEnd; x += gridSize)
    {
      g_imRenderer->Begin(PRIM_LINES);
      g_imRenderer->Vertex3f(x, height, zStart);
      g_imRenderer->Vertex3f(x, height, zEnd);
      g_imRenderer->Vertex3f(xStart, height, x);
      g_imRenderer->Vertex3f(xEnd, height, x);
      g_imRenderer->End();
    }

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);

    g_app->m_globalWorld->SetupFog();

    g_imRenderer->PopMatrix();
  }
}
