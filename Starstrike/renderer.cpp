#include "pch.h"
#include "input.h"
#include "debug_utils.h"
#include "hi_res_time.h"
#include "win32_eventhandler.h"
#include "math_utils.h"
#include "persisting_debug_render.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "text_renderer.h"
#include "window_manager.h"
#include "language_table.h"
#include "debug_render.h"
#include "app.h"
#include "camera.h"
#include "explosion.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "taskmanager_interface.h"
#include "user_input.h"
#include "render_device.h"
#include "render_states.h"
#include "im_renderer.h"
#include "gamecursor.h"
#include "startsequence.h"
#include "soundsystem.h"
#include "eclipse.h"
#include "message_dialog.h"

Renderer::Renderer()
  : m_fps(60),
    m_displayFPS(false),
    m_renderDebug(false),
    m_displayInputMode(false),
    m_renderDarwinLogo(-1.0f),
    m_nearPlane(5.0f),
    m_farPlane(150000.0f),
    m_tileIndex(0),
    m_fadedness(0.0f),
    m_fadeRate(0.0f),
    m_fadeDelay(0.0f),
    m_pixelSize(256) {}

void Renderer::Initialise()
{
  m_screenW = g_prefsManager->GetInt("ScreenWidth", 0);
  m_screenH = g_prefsManager->GetInt("ScreenHeight", 0);
  bool windowed = g_prefsManager->GetInt("ScreenWindowed", 0) ? true : false;
  int colourDepth = g_prefsManager->GetInt("ScreenColourDepth", 32);
  int refreshRate = g_prefsManager->GetInt("ScreenRefresh", 75);
  int zDepth = g_prefsManager->GetInt("ScreenZDepth", 24);
  bool waitVRT = g_prefsManager->GetInt("WaitVerticalRetrace", 1);

  if (m_screenW == 0 || m_screenH == 0)
  {
    g_windowManager->SuggestDefaultRes(&m_screenW, &m_screenH, &refreshRate, &colourDepth);
    g_prefsManager->SetInt("ScreenWidth", m_screenW);
    g_prefsManager->SetInt("ScreenHeight", m_screenH);
    g_prefsManager->SetInt("ScreenRefresh", refreshRate);
    g_prefsManager->SetInt("ScreenColourDepth", colourDepth);
  }

  bool success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);

  if (!success)
  {
    char caption[512];
    sprintf(caption, "Failed to set requested screen resolution of\n"
            "%d x %d, %d bit colour, %s\n\n" "Restored to safety settings of\n" "640 x 480, 16 bit colour, windowed", m_screenW, m_screenH,
            colourDepth, windowed ? "windowed" : "fullscreen");
    auto dialog = new MessageDialog("Error", caption);
    EclRegisterWindow(dialog);
    dialog->m_x = 100;
    dialog->m_y = 100;

    // Go for safety values
    m_screenW = 640;
    m_screenH = 480;
    windowed = true;
    colourDepth = 16;
    zDepth = 16;
    refreshRate = 60;

    success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);
    if (!success)
    {
      // next try with 24bit z (colour depth is automatic in windowed mode)
      zDepth = 24;
      success = g_windowManager->CreateWin(m_screenW, m_screenH, windowed, colourDepth, refreshRate, zDepth, waitVRT);
    }
    DarwiniaReleaseAssert(success, "Failed to set screen mode");

    g_prefsManager->SetInt("ScreenWidth", m_screenW);
    g_prefsManager->SetInt("ScreenHeight", m_screenH);
    g_prefsManager->SetInt("ScreenWindowed", 1);
    g_prefsManager->SetInt("ScreenColourDepth", colourDepth);
    g_prefsManager->SetInt("ScreenRefresh", 60);
    g_prefsManager->Save();
  }

  BuildOpenGlState();
}

void Renderer::Restart() { BuildOpenGlState(); }

void Renderer::BuildOpenGlState()
{
  // Phase 9: pixel effect texture now created via TextureManager
  // m_pixelEffectTexId will be set when needed
}

void Renderer::RenderFlatTexture()
{
  g_imRenderer->Color3ubv(g_colourWhite.GetData());
  int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, true);
  if (textureId == -1)
    return;
  g_imRenderer->BindTexture(textureId);

  float size = m_nearPlane * 0.3f;
  LegacyVector3 up = g_app->m_camera->GetUp() * 1.0f * size;
  LegacyVector3 right = g_app->m_camera->GetRight() * 1.0f * size;
  LegacyVector3 pos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * m_nearPlane * 1.01f;

  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2f(1.0f, 1.0f);
  g_imRenderer->Vertex3fv((pos + up - right).GetData());
  g_imRenderer->TexCoord2f(0.0f, 1.0f);
  g_imRenderer->Vertex3fv((pos + up + right).GetData());
  g_imRenderer->TexCoord2f(0.0f, 0.0f);
  g_imRenderer->Vertex3fv((pos - up + right).GetData());
  g_imRenderer->TexCoord2f(1.0f, 0.0f);
  g_imRenderer->Vertex3fv((pos - up - right).GetData());
  g_imRenderer->End();


  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);

  g_imRenderer->UnbindTexture();

  g_imRenderer->Begin(PRIM_LINE_LOOP);
  g_imRenderer->Vertex3fv((pos + up - right).GetData());
  g_imRenderer->Vertex3fv((pos + up + right).GetData());
  g_imRenderer->Vertex3fv((pos - up + right).GetData());
  g_imRenderer->Vertex3fv((pos - up - right).GetData());
  g_imRenderer->End();

}

void Renderer::RenderLogo()
{
  g_imRenderer->Color3ubv(g_colourWhite.GetData());
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

  g_imRenderer->Color4ub(0, 0, 0, 200);
  float logoW = 200;
  float logoH = 35;
  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->Vertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
  g_imRenderer->Vertex2f(m_screenW - 10, m_screenH - logoH - 10);
  g_imRenderer->Vertex2f(m_screenW - 10, m_screenH - 10);
  g_imRenderer->Vertex2f(m_screenW - logoW - 10, m_screenH - 10);
  g_imRenderer->End();


  g_imRenderer->Color4ub(255, 255, 255, 255);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
  int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, false);
  if (textureId == -1)
    return;
  g_imRenderer->BindTexture(textureId);

  g_imRenderer->Begin(PRIM_QUADS);
  g_imRenderer->TexCoord2f(0.0f, 1.0f);
  g_imRenderer->Vertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
  g_imRenderer->TexCoord2f(1.0f, 1.0f);
  g_imRenderer->Vertex2f(m_screenW - 10, m_screenH - logoH - 10);
  g_imRenderer->TexCoord2f(1.0f, 0.0f);
  g_imRenderer->Vertex2f(m_screenW - 10, m_screenH - 10);
  g_imRenderer->TexCoord2f(0.0f, 0.0f);
  g_imRenderer->Vertex2f(m_screenW - logoW - 10, m_screenH - 10);
  g_imRenderer->End();


  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
  g_imRenderer->UnbindTexture();
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

  g_imRenderer->Color4f(1.0f, 0.75f, 0.75f, 1.0f);
  g_gameFont.DrawText2D(20, m_screenH - 70, 25, LANGUAGEPHRASE("privatedemo1"));
  g_gameFont.DrawText2D(20, m_screenH - 40, 25, LANGUAGEPHRASE("privatedemo2"));
  g_gameFont.DrawText2D(20, m_screenH - 10, 10, LANGUAGEPHRASE("privatedemo2"));
}

void Renderer::Render()
{
#ifdef PROFILER_ENABLED
  g_app->m_profiler->RenderStarted();
#endif

  RenderFrame();

#ifdef PROFILER_ENABLED
  g_app->m_profiler->RenderEnded();
#endif // PROFILER_ENABLED
}

bool Renderer::IsFadeComplete() const
{
  if (NearlyEquals(m_fadeRate, 0.0f))
    return true;

  return false;
}

void Renderer::StartFadeOut()
{
  m_fadeDelay = 0.0f;
  m_fadeRate = 1.0f;
}

void Renderer::StartFadeIn(float _delay)
{
  m_fadedness = 1.0f;
  m_fadeDelay = _delay;
  m_fadeRate = -1.0f;
}

void Renderer::RenderFadeOut()
{
  static double lastTime = GetHighResTime();
  double timeNow = GetHighResTime();
  double timeIncrement = timeNow - lastTime;
  if (timeIncrement > 0.05f)
    timeIncrement = 0.05f;
  lastTime = timeNow;

  if (m_fadeDelay > 0.0f)
    m_fadeDelay -= timeIncrement;
  else
  {
    m_fadedness += m_fadeRate * timeIncrement;
    if (m_fadedness < 0.0f)
    {
      m_fadedness = 0.0f;
      m_fadeRate = 0.0f;
    }
    else if (m_fadedness > 1.0f)
    {
      m_fadeRate = 0.0f;
      m_fadedness = 1.0f;
    }
  }

  if (m_fadedness > 0.0001f)
  {
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_READONLY);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);

    g_imRenderer->Color4ub(0, 0, 0, static_cast<int>(m_fadedness * 255.0f));
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2i(-1, -1);
    g_imRenderer->Vertex2i(m_screenW, -1);
    g_imRenderer->Vertex2i(m_screenW, m_screenH);
    g_imRenderer->Vertex2i(-1, m_screenH);
    g_imRenderer->End();


    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_DISABLED);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_ENABLED_WRITE);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
  }
}

void Renderer::RenderPaused()
{
  auto msg = "PAUSED";
  int x = g_app->m_renderer->ScreenW() / 2;
  int y = g_app->m_renderer->ScreenH() / 2;
  TextRenderer& font = g_gameFont;

  font.BeginText2D();

  // Black Background
  g_gameFont.SetRenderShadow(true);
  g_imRenderer->Color4f(0.3f, 0.3f, 0.3f, 0.0f);
  font.DrawText2DCentre(x, y, 80, msg);

  // White Foreground
  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
  font.SetRenderShadow(false);
  font.DrawText2DCentre(x, y, 80, msg);

  font.EndText2D();
}

void Renderer::RenderFrame(bool withFlip)
{
  int renderPixelShaderPref = g_prefsManager->GetInt("RenderPixelShader");

  SetOpenGLState();

  if (g_app->m_locationId == -1)
  {
    m_nearPlane = 50.0f;
    m_farPlane = 10000000.0f;
  }
  else
  {
    m_nearPlane = 5.0f;
    m_farPlane = 15000.0f;
  }

  FPSMeterAdvance();
  SetupMatricesFor3D();

  START_PROFILE(g_app->m_profiler, "Render Clear");
  RGBAColour* col = &g_app->m_backgroundColour;
  float clearR = col->r / 255.0f, clearG = col->g / 255.0f, clearB = col->b / 255.0f;
  g_renderDevice->BeginFrame(clearR, clearG, clearB, 1.0f);
  END_PROFILE(g_app->m_profiler, "Render Clear");

  if (g_app->m_locationId != -1)
  {
    g_app->m_location->Render();
  }
  else
    g_app->m_globalWorld->Render();


  g_explosionManager.Render();
  g_app->m_particleSystem->Render();
  g_app->m_userInput->Render();
  g_app->m_gameCursor->Render();
  g_app->m_taskManagerInterface->Render();
  g_app->m_camera->Render();

#ifdef DEBUG_RENDER_ENABLED
  g_debugRenderer.Render();
#endif

  g_editorFont.BeginText2D();

  RenderFadeOut();

  if (m_displayFPS)
  {
    g_imRenderer->Color4f(0, 0, 0, 0.6);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2f(8.0, 1.0f);
    g_imRenderer->Vertex2f(70.0, 1.0f);
    g_imRenderer->Vertex2f(70.0, 15.0f);
    g_imRenderer->Vertex2f(8.0, 15.0f);
    g_imRenderer->End();


    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(12, 10, DEF_FONT_SIZE, "FPS: %d", m_fps);
    //		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "TFPS: %2.0f", g_targetFrameRate);
    //		LegacyVector3 const camPos = g_app->m_camera->GetPos();
    //		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "cam: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
  }

  if (m_displayInputMode)
  {
    g_imRenderer->Color4f(0, 0, 0, 0.6);
    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->Vertex2f(80.0, 1.0f);
    g_imRenderer->Vertex2f(230.0, 1.0f);
    g_imRenderer->Vertex2f(230.0, 18.0f);
    g_imRenderer->Vertex2f(80.0, 18.0f);
    g_imRenderer->End();


    std::string inmode;
    switch (g_inputManager->getInputMode())
    {
    case INPUT_MODE_KEYBOARD:
      inmode = "keyboard";
      break;
    case INPUT_MODE_GAMEPAD:
      inmode = "gamepad";
      break;
    default:
      inmode = "unknown";
    }

    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(84, 10, DEF_FONT_SIZE, "InputMode: %s", inmode.c_str());
  }

  if (g_app->m_server)
  {
    int latency = g_app->m_server->m_sequenceId - g_lastProcessedSequenceId;
    if (latency > 10)
    {
      g_imRenderer->Color4f(0.0f, 0.0f, 0.0f, 0.8f);
      g_imRenderer->Begin(PRIM_QUADS);
      g_imRenderer->Vertex2f(m_screenW / 2 - 200, 120);
      g_imRenderer->Vertex2f(m_screenW / 2 + 200, 120);
      g_imRenderer->Vertex2f(m_screenW / 2 + 200, 80);
      g_imRenderer->Vertex2f(m_screenW / 2 - 200, 80);
      g_imRenderer->End();

      g_imRenderer->Color4f(1.0f, 0.0f, 0.0f, 1.0f);
      g_editorFont.DrawText2DCentre(m_screenW / 2, 100, 20, "Client LAG %dms behind Server ", latency * 100);
    }
  }

  //
  // Personalisation information

  if (!g_app->m_taskManagerInterface->m_visible && !g_app->
    m_camera->IsInMode(Camera::ModeSphereWorldIntro) && !g_app->m_camera->IsInMode(Camera::ModeSphereWorldOutro))
  {
#ifdef PROMOTIONAL_BUILD
    RenderLogo();
#endif
  }

  if (g_app->m_startSequence)
    g_app->m_startSequence->Render();

  if (m_renderDarwinLogo >= 0.0f)
  {
    int textureId = g_app->m_resource->GetTexture("icons/darwin_research_associates.bmp");

    g_imRenderer->BindTexture(textureId);

    float y = m_screenH * 0.05f;
    float h = m_screenH * 0.7f;

    float w = h;
    float x = m_screenW / 2 - w / 2;

    float alpha = 0.0f;

    float timeNow = GetHighResTime();
    if (timeNow > m_renderDarwinLogo + 10)
      m_renderDarwinLogo = -1.0f;
    else if (timeNow < m_renderDarwinLogo + 3)
      alpha = (timeNow - m_renderDarwinLogo) / 3.0f;
    else if (timeNow > m_renderDarwinLogo + 8)
      alpha = 1.0f - (timeNow - m_renderDarwinLogo - 8) / 2.0f;
    else
      alpha = 1.0f;

    alpha = max(alpha, 0.0f);
    alpha = min(alpha, 1.0f);

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_SUBTRACTIVE_COLOR);
    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_imRenderer->Color4f(alpha, alpha, alpha, 0.0f);

    for (float dx = -4; dx <= 4; dx += 4)
    {
      for (float dy = -4; dy <= 4; dy += 4)
      {
        g_imRenderer->Begin(PRIM_QUADS);
        g_imRenderer->TexCoord2i(0, 1);
        g_imRenderer->Vertex2f(x + dx, y + dy);
        g_imRenderer->TexCoord2i(1, 1);
        g_imRenderer->Vertex2f(x + w + dx, y + dy);
        g_imRenderer->TexCoord2i(1, 0);
        g_imRenderer->Vertex2f(x + w + dx, y + h + dy);
        g_imRenderer->TexCoord2i(0, 0);
        g_imRenderer->Vertex2f(x + dx, y + h + dy);
        g_imRenderer->End();

      }
    }

    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ADDITIVE);
    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, alpha);

    g_imRenderer->Begin(PRIM_QUADS);
    g_imRenderer->TexCoord2i(0, 1);
    g_imRenderer->Vertex2f(x, y);
    g_imRenderer->TexCoord2i(1, 1);
    g_imRenderer->Vertex2f(x + w, y);
    g_imRenderer->TexCoord2i(1, 0);
    g_imRenderer->Vertex2f(x + w, y + h);
    g_imRenderer->TexCoord2i(0, 0);
    g_imRenderer->Vertex2f(x, y + h);
    g_imRenderer->End();


    g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
    g_imRenderer->UnbindTexture();

    float textSize = m_screenH / 9.0f;
    g_gameFont.SetRenderOutline(true);
    g_imRenderer->Color4f(alpha, alpha, alpha, 0.0f);
    g_gameFont.DrawText2DCentre(m_screenW / 2, m_screenH * 0.8f, textSize, "DARWINIA");
    g_gameFont.SetRenderOutline(false);
    g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, alpha);
    g_gameFont.DrawText2DCentre(m_screenW / 2, m_screenH * 0.8f, textSize, "DARWINIA");
  }

  g_editorFont.EndText2D();

  if (!g_eventHandler->WindowHasFocus() || g_app->m_paused)
    RenderPaused();

  START_PROFILE(g_app->m_profiler, "GL Flip");

  if (withFlip)
    g_windowManager->Flip();

  END_PROFILE(g_app->m_profiler, "GL Flip");

}

int Renderer::ScreenW() const { return m_screenW; }

int Renderer::ScreenH() const { return m_screenH; }

void Renderer::SetupProjMatrixFor3D() const
{
  // D3D11: set projection on ImRenderer (RH to match OpenGL coordinate convention)
  if (g_imRenderer)
  {
    float fovY = DirectX::XMConvertToRadians(g_app->m_camera->GetFov());
    float aspect = static_cast<float>(m_screenW) / static_cast<float>(m_screenH);
    g_imRenderer->SetProjectionMatrix(DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, m_nearPlane, m_farPlane));
  }
}

void Renderer::SetupMatricesFor3D() const
{
  Camera* camera = g_app->m_camera;

  SetupProjMatrixFor3D();
  camera->SetupModelviewMatrix();
}

void Renderer::SetupMatricesFor2D() const
{
  float left   = 0.0f;
  float right  = static_cast<float>(m_screenW);
  float bottom = static_cast<float>(m_screenH);
  float top    = 0.0f;

  // D3D11: set ortho projection + identity modelview
  if (g_imRenderer)
  {
    g_imRenderer->SetProjectionMatrix(DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1.0f, 1.0f));
    g_imRenderer->SetViewMatrix(DirectX::XMMatrixIdentity());
    g_imRenderer->SetWorldMatrix(DirectX::XMMatrixIdentity());
  }
}

void Renderer::FPSMeterAdvance()
{
  static int framesThisSecond = 0;
  static double endOfSecond = GetHighResTime() + 1.0;

  framesThisSecond++;

  double currentTime = GetHighResTime();
  if (currentTime > endOfSecond)
  {
    if (currentTime > endOfSecond + 2.0)
      endOfSecond = currentTime + 1.0;
    else
      endOfSecond += 1.0;
    m_fps = framesThisSecond;
    framesThisSecond = 0;
  }
}

float Renderer::GetNearPlane() const { return m_nearPlane; }

float Renderer::GetFarPlane() const { return m_farPlane; }

void Renderer::SetNearAndFar(float _nearPlane, float _farPlane)
{
  DarwiniaDebugAssert(_nearPlane < _farPlane);
  DarwiniaDebugAssert(_nearPlane > 0.0f);
  m_nearPlane = _nearPlane;
  m_farPlane = _farPlane;
}

int Renderer::GetGLStateInt(int pname) const
{
  return 0;
}

float Renderer::GetGLStateFloat(int pname) const
{
  return 0.0f;
}

void Renderer::CheckOpenGLState() const
{
  // No-op: GL state validation removed in D3D11 migration
}

void Renderer::SetOpenGLState() const
{
  auto* ctx = g_renderDevice->GetContext();

  g_renderStates->SetRasterState(ctx, RASTER_CULL_BACK);

  if (g_app->m_location)
    g_app->m_location->SetupLights();
  else
    g_app->m_globalWorld->SetupLights();

  g_renderStates->SetBlendState(ctx, BLEND_ALPHA);
  if (g_app->m_location)
    g_app->m_location->SetupFog();
  else
    g_app->m_globalWorld->SetupFog();

  g_imRenderer->UnbindTexture();

  g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_WRITE);
}

void Renderer::SetObjectLighting() const
{
  // TODO Phase 10: Lighting via constant buffer update
}

void Renderer::UnsetObjectLighting() const
{
  // TODO Phase 10: Disable lighting via constant buffer update
}

void Renderer::UpdateTotalMatrix()
{
  // D3D11: compute total matrix from ImRenderer's stored matrices
  if (g_imRenderer)
  {
    using namespace DirectX;
    XMMATRIX wvp = g_imRenderer->GetWorldMatrix() * g_imRenderer->GetViewMatrix() * g_imRenderer->GetProjectionMatrix();
    // Store as column-major doubles (OpenGL layout) for compatibility with Get2DScreenPos
    XMFLOAT4X4 f;
    XMStoreFloat4x4(&f, wvp);
    // Transpose to column-major (OpenGL convention used by Get2DScreenPos)
    m_totalMatrix[0]  = f._11; m_totalMatrix[1]  = f._21; m_totalMatrix[2]  = f._31; m_totalMatrix[3]  = f._41;
    m_totalMatrix[4]  = f._12; m_totalMatrix[5]  = f._22; m_totalMatrix[6]  = f._32; m_totalMatrix[7]  = f._42;
    m_totalMatrix[8]  = f._13; m_totalMatrix[9]  = f._23; m_totalMatrix[10] = f._33; m_totalMatrix[11] = f._43;
    m_totalMatrix[12] = f._14; m_totalMatrix[13] = f._24; m_totalMatrix[14] = f._34; m_totalMatrix[15] = f._44;
    return;
  }

  double m[16];
  double p[16];

  DarwiniaDebugAssert(m[3] == 0.0);
  DarwiniaDebugAssert(m[7] == 0.0);
  DarwiniaDebugAssert(m[11] == 0.0);
  DarwiniaDebugAssert(NearlyEquals(m[15], 1.0));

  DarwiniaDebugAssert(p[1] == 0.0);
  DarwiniaDebugAssert(p[2] == 0.0);
  DarwiniaDebugAssert(p[3] == 0.0);
  DarwiniaDebugAssert(p[4] == 0.0);
  DarwiniaDebugAssert(p[6] == 0.0);
  DarwiniaDebugAssert(p[7] == 0.0);
  DarwiniaDebugAssert(p[8] == 0.0);
  DarwiniaDebugAssert(p[9] == 0.0);
  DarwiniaDebugAssert(p[12] == 0.0);
  DarwiniaDebugAssert(p[13] == 0.0);
  DarwiniaDebugAssert(p[15] == 0.0);

  m_totalMatrix[0] = m[0] * p[0] + m[1] * p[4] + m[2] * p[8] + m[3] * p[12];
  m_totalMatrix[1] = m[0] * p[1] + m[1] * p[5] + m[2] * p[9] + m[3] * p[13];
  m_totalMatrix[2] = m[0] * p[2] + m[1] * p[6] + m[2] * p[10] + m[3] * p[14];
  m_totalMatrix[3] = m[0] * p[3] + m[1] * p[7] + m[2] * p[11] + m[3] * p[15];

  m_totalMatrix[4] = m[4] * p[0] + m[5] * p[4] + m[6] * p[8] + m[7] * p[12];
  m_totalMatrix[5] = m[4] * p[1] + m[5] * p[5] + m[6] * p[9] + m[7] * p[13];
  m_totalMatrix[6] = m[4] * p[2] + m[5] * p[6] + m[6] * p[10] + m[7] * p[14];
  m_totalMatrix[7] = m[4] * p[3] + m[5] * p[7] + m[6] * p[11] + m[7] * p[15];

  m_totalMatrix[8] = m[8] * p[0] + m[9] * p[4] + m[10] * p[8] + m[11] * p[12];
  m_totalMatrix[9] = m[8] * p[1] + m[9] * p[5] + m[10] * p[9] + m[11] * p[13];
  m_totalMatrix[10] = m[8] * p[2] + m[9] * p[6] + m[10] * p[10] + m[11] * p[14];
  m_totalMatrix[11] = m[8] * p[3] + m[9] * p[7] + m[10] * p[11] + m[11] * p[15];

  m_totalMatrix[12] = m[12] * p[0] + m[13] * p[4] + m[14] * p[8] + m[15] * p[12];
  m_totalMatrix[13] = m[12] * p[1] + m[13] * p[5] + m[14] * p[9] + m[15] * p[13];
  m_totalMatrix[14] = m[12] * p[2] + m[13] * p[6] + m[14] * p[10] + m[15] * p[14];
  m_totalMatrix[15] = m[12] * p[3] + m[13] * p[7] + m[14] * p[11] + m[15] * p[15];
}

void Renderer::Get2DScreenPos(const LegacyVector3& v, LegacyVector3* _out)
{
  double out[4];

#define m m_totalMatrix
  out[0] = v.x * m[0] + v.y * m[4] + v.z * m[8] + m[12];
  out[1] = v.x * m[1] + v.y * m[5] + v.z * m[9] + m[13];
  out[2] = v.x * m[2] + v.y * m[6] + v.z * m[10] + m[14];
  out[3] = v.x * m[3] + v.y * m[7] + v.z * m[11] + m[15];
#undef m

  if (out[3] <= 0.0f)
    return;

  double multiplier = 0.5f / out[3];
  out[0] *= multiplier;
  out[1] *= multiplier;

  // Map x, y and z to range 0-1
  out[0] += 0.5;
  out[1] += 0.5;

  // Map x, y to viewport
  _out->x = out[0] * m_screenW;
  _out->y = out[1] * m_screenH;
  _out->z = out[3];
}

const double* Renderer::GetTotalMatrix() { return m_totalMatrix; }

void Renderer::RasteriseSphere(const LegacyVector3& _pos, float _radius)
{
  const float screenToGridFactor = static_cast<float>(PIXEL_EFFECT_GRID_RES) / static_cast<float>(m_screenW);
  Camera* cam = g_app->m_camera;
  LegacyVector3 centre;
  LegacyVector3 topLeft;
  LegacyVector3 bottomRight;
  const LegacyVector3 camUpRight = (cam->GetRight() + cam->GetUp()) * _radius;
  Get2DScreenPos(_pos, &centre);
  Get2DScreenPos(_pos + camUpRight, &topLeft);
  Get2DScreenPos(_pos - camUpRight, &bottomRight);

  int x1 = floorf(topLeft.x * screenToGridFactor);
  int x2 = ceilf(bottomRight.x * screenToGridFactor);
  int y1 = floorf(bottomRight.y * screenToGridFactor);
  int y2 = ceilf(topLeft.y * screenToGridFactor);

  clamp(x1, 0, PIXEL_EFFECT_GRID_RES);
  clamp(x2, 0, PIXEL_EFFECT_GRID_RES);
  clamp(y1, 0, PIXEL_EFFECT_GRID_RES);
  clamp(y2, 0, PIXEL_EFFECT_GRID_RES);

  const float nearestZ = centre.z - _radius;

  for (int y = y1; y < y2; ++y)
  {
    for (int x = x1; x < x2; ++x)
    {
      if (nearestZ < m_pixelEffectGrid[x][y])
        m_pixelEffectGrid[x][y] = nearestZ;
    }
  }
}

void Renderer::MarkUsedCells(const ShapeFragment* _frag, const Matrix34& _transform)
{
#if USE_PIXEL_EFFECT_GRID_OPTIMISATION
  Matrix34 total = _frag->m_transform * _transform;
  LegacyVector3 worldPos = _frag->m_centre * total;

  // Return early if this shape fragment isn't on the screen
  {
    if (!g_app->m_camera->SphereInViewFrustum(worldPos, _frag->m_radius))
      return;
  }

  if (_frag->m_radius > 0.0f)
    RasteriseSphere(worldPos, _frag->m_radius);

  // Recurse into all child fragments
  int numChildren = _frag->m_childFragments.Size();
  for (int i = 0; i < numChildren; ++i)
  {
    const ShapeFragment* child = _frag->m_childFragments.GetData(i);
    MarkUsedCells(child, total);
  }
#endif // USE_PIXEL_EFFECT_GRID_OPTIMISATION
}

void Renderer::MarkUsedCells(const Shape* _shape, const Matrix34& _transform)
{
  START_PROFILE(g_app->m_profiler, "MarkUsedCells");
  MarkUsedCells(_shape->m_rootFragment, _transform);
  END_PROFILE(g_app->m_profiler, "MarkUsedCells");
}
