#include "pch.h"
#include "input.h"

#include "hi_res_time.h"
#include "math_utils.h"
#include "ogl_extensions.h"
#include "preferences.h"
#include "profiler.h"
#include "resource.h"
#include "text_renderer.h"
#include "window_manager.h"
#include "language_table.h"
#include "GameApp.h"
#include "camera.h"
#include "explosion.h"
#include "global_world.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "taskmanager_interface.h"
#include "user_input.h"
#include "gamecursor.h"
#include "startsequence.h"
#include "soundsystem.h"

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
  m_screenW = static_cast<int>(ClientEngine::OutputSize().Width);
  m_screenH = static_cast<int>(ClientEngine::OutputSize().Height);

  InitialiseOGLExtensions();

  BuildOpenGlState();
}

void Renderer::Restart() { BuildOpenGlState(); }

void Renderer::BuildOpenGlState() { glGenTextures(1, &m_pixelEffectTexId); }

void Renderer::RenderFlatTexture()
{
  glColor3ubv(g_colourWhite.GetData());
  glEnable(GL_TEXTURE_2D);
  int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, true);
  if (textureId == -1)
    return;
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  float size = m_nearPlane * 0.3f;
  LegacyVector3 up = g_app->m_camera->GetUp() * 1.0f * size;
  LegacyVector3 right = g_app->m_camera->GetRight() * 1.0f * size;
  LegacyVector3 pos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * m_nearPlane * 1.01f;

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.02f);

  glBegin(GL_QUADS);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3fv((pos + up - right).GetData());
  glTexCoord2f(0.0f, 1.0f);
  glVertex3fv((pos + up + right).GetData());
  glTexCoord2f(0.0f, 0.0f);
  glVertex3fv((pos - up + right).GetData());
  glTexCoord2f(1.0f, 0.0f);
  glVertex3fv((pos - up - right).GetData());
  glEnd();

  glAlphaFunc(GL_GREATER, 0.01);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);

  glDisable(GL_TEXTURE_2D);

  glLineWidth(1.0f);
  glBegin(GL_LINE_LOOP);
  glVertex3fv((pos + up - right).GetData());
  glVertex3fv((pos + up + right).GetData());
  glVertex3fv((pos - up + right).GetData());
  glVertex3fv((pos - up - right).GetData());
  glEnd();
}

void Renderer::RenderLogo()
{
  glColor3ubv(g_colourWhite.GetData());
  glEnable(GL_BLEND);
  glDepthMask(false);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4ub(0, 0, 0, 200);
  float logoW = 200;
  float logoH = 35;
  glBegin(GL_QUADS);
  glVertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
  glVertex2f(m_screenW - 10, m_screenH - logoH - 10);
  glVertex2f(m_screenW - 10, m_screenH - 10);
  glVertex2f(m_screenW - logoW - 10, m_screenH - 10);
  glEnd();

  glColor4ub(255, 255, 255, 255);
  glEnable(GL_TEXTURE_2D);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  int textureId = g_app->m_resource->GetTexture("textures/privatedemo.bmp", true, false);
  if (textureId == -1)
    return;
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(m_screenW - logoW - 10, m_screenH - logoH - 10);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(m_screenW - 10, m_screenH - logoH - 10);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(m_screenW - 10, m_screenH - 10);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(m_screenW - logoW - 10, m_screenH - 10);
  glEnd();

  glDepthMask(true);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0f, 0.75f, 0.75f, 1.0f);
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
    glEnable(GL_BLEND);
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4ub(0, 0, 0, static_cast<int>(m_fadedness * 255.0f));
    glBegin(GL_QUADS);
    glVertex2i(-1, -1);
    glVertex2i(m_screenW, -1);
    glVertex2i(m_screenW, m_screenH);
    glVertex2i(-1, m_screenH);
    glEnd();

    glDisable(GL_BLEND);
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
  glColor4f(0.3f, 0.3f, 0.3f, 0.0f);
  font.DrawText2DCentre(x, y, 80, msg);

  // White Foreground
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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
  if (g_app->m_location)
    glClearColor(col->r / 255.0f, col->g / 255.0f, col->b / 255.0f, col->a / 255.0f);
  else
    glClearColor(0.05f, 0.0f, 0.05f, 0.1f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  END_PROFILE(g_app->m_profiler, "Render Clear");

  if (g_app->m_locationId != -1)
    g_app->m_location->Render();
  else
    g_app->m_globalWorld->Render();

  g_explosionManager.Render();
  g_app->m_particleSystem->Render();
  g_app->m_userInput->Render();
  g_app->m_gameCursor->Render();
  g_app->m_taskManagerInterface->Render();
  g_app->m_camera->Render();

  g_editorFont.BeginText2D();

  RenderFadeOut();

  if (m_displayFPS)
  {
    glColor4f(0, 0, 0, 0.6);
    glBegin(GL_QUADS);
    glVertex2f(8.0, 1.0f);
    glVertex2f(70.0, 1.0f);
    glVertex2f(70.0, 15.0f);
    glVertex2f(8.0, 15.0f);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(12, 10, DEF_FONT_SIZE, "FPS: %d", m_fps);
    //		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "TFPS: %2.0f", g_targetFrameRate);
    //		LegacyVector3 const camPos = g_app->m_camera->GetPos();
    //		g_editorFont.DrawText2D( 150, 10, DEF_FONT_SIZE, "cam: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
  }

  if (m_displayInputMode)
  {
    glColor4f(0, 0, 0, 0.6);
    glBegin(GL_QUADS);
    glVertex2f(80.0, 1.0f);
    glVertex2f(230.0, 1.0f);
    glVertex2f(230.0, 18.0f);
    glVertex2f(80.0, 18.0f);
    glEnd();

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

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    g_editorFont.DrawText2D(84, 10, DEF_FONT_SIZE, "InputMode: %s", inmode.c_str());
  }

  if (g_app->m_server)
  {
    int latency = g_app->m_server->m_sequenceId - g_lastProcessedSequenceId;
    if (latency > 10)
    {
      glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
      glBegin(GL_QUADS);
      glVertex2f(m_screenW / 2 - 200, 120);
      glVertex2f(m_screenW / 2 + 200, 120);
      glVertex2f(m_screenW / 2 + 200, 80);
      glVertex2f(m_screenW / 2 - 200, 80);
      glEnd();
      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
      g_editorFont.DrawText2DCentre(m_screenW / 2, 100, 20, "Client LAG %dms behind Server ", latency * 100);
    }
  }

  //
  // Personalisation information

  if (!g_app->m_taskManagerInterface->m_visible && !g_app->m_camera->IsInMode(Camera::ModeSphereWorldIntro) && !g_app->m_camera->IsInMode(
    Camera::ModeSphereWorldOutro))
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

    glBindTexture(GL_TEXTURE_2D, textureId);
    glEnable(GL_TEXTURE_2D);

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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glEnable(GL_BLEND);
    glColor4f(alpha, alpha, alpha, 0.0f);

    for (float dx = -4; dx <= 4; dx += 4)
    {
      for (float dy = -4; dy <= 4; dy += 4)
      {
        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex2f(x + dx, y + dy);
        glTexCoord2i(1, 1);
        glVertex2f(x + w + dx, y + dy);
        glTexCoord2i(1, 0);
        glVertex2f(x + w + dx, y + h + dy);
        glTexCoord2i(0, 0);
        glVertex2f(x + dx, y + h + dy);
        glEnd();
      }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    glBegin(GL_QUADS);
    glTexCoord2i(0, 1);
    glVertex2f(x, y);
    glTexCoord2i(1, 1);
    glVertex2f(x + w, y);
    glTexCoord2i(1, 0);
    glVertex2f(x + w, y + h);
    glTexCoord2i(0, 0);
    glVertex2f(x, y + h);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    float textSize = m_screenH / 9.0f;
    g_gameFont.SetRenderOutline(true);
    glColor4f(alpha, alpha, alpha, 0.0f);
    g_gameFont.DrawText2DCentre(m_screenW / 2, m_screenH * 0.8f, textSize, "DARWINIA");
    g_gameFont.SetRenderOutline(false);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    g_gameFont.DrawText2DCentre(m_screenW / 2, m_screenH * 0.8f, textSize, "DARWINIA");
  }

  g_editorFont.EndText2D();

  START_PROFILE(g_app->m_profiler, "GL Flip");

  if (withFlip)
    g_windowManager->Flip();

  END_PROFILE(g_app->m_profiler, "GL Flip");

  CHECK_OPENGL_STATE();
}

int Renderer::ScreenW() const { return m_screenW; }

int Renderer::ScreenH() const { return m_screenH; }

void Renderer::SetupProjMatrixFor3D() const
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  gluPerspective(g_app->m_camera->GetFov(), static_cast<float>(m_screenW) / static_cast<float>(m_screenH), // Aspect ratio
                 m_nearPlane, m_farPlane);
}

void Renderer::SetupMatricesFor3D() const
{
  Camera* camera = g_app->m_camera;

  SetupProjMatrixFor3D();
  camera->SetupModelviewMatrix();
}

void Renderer::SetupMatricesFor2D() const
{
  int v[4];
  glGetIntegerv(GL_VIEWPORT, &v[0]);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float left = v[0];
  float right = v[0] + v[2];
  float bottom = v[1] + v[3];
  float top = v[1];
  //	glOrtho(left, right, bottom, top, -m_nearPlane, -m_farPlane);
  gluOrtho2D(left, right, bottom, top);
  glMatrixMode(GL_MODELVIEW);
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
  DEBUG_ASSERT(_nearPlane < _farPlane);
  DEBUG_ASSERT(_nearPlane > 0.0f);
  m_nearPlane = _nearPlane;
  m_farPlane = _farPlane;
}

int Renderer::GetGLStateInt(int pname) const
{
  int returnVal;
  glGetIntegerv(pname, &returnVal);
  return returnVal;
}

float Renderer::GetGLStateFloat(int pname) const
{
  float returnVal;
  glGetFloatv(pname, &returnVal);
  return returnVal;
}

void Renderer::CheckOpenGLState() const
{
  return;
  int results[10];
  float resultsf[10];

  DEBUG_ASSERT(glGetError() == GL_NO_ERROR);

  // Geometry
  //	DEBUG_ASSERT(glIsEnabled(GL_CULL_FACE));
  DEBUG_ASSERT(GetGLStateInt(GL_FRONT_FACE) == GL_CCW);
  glGetIntegerv(GL_POLYGON_MODE, results);
  DEBUG_ASSERT(results[0] == GL_FILL);
  DEBUG_ASSERT(results[1] == GL_FILL);
  DEBUG_ASSERT(GetGLStateInt(GL_SHADE_MODEL) == GL_FLAT);
  DEBUG_ASSERT(!glIsEnabled(GL_NORMALIZE));

  // Colour
  DEBUG_ASSERT(!glIsEnabled(GL_COLOR_MATERIAL));
  DEBUG_ASSERT(GetGLStateInt(GL_COLOR_MATERIAL_FACE) == GL_FRONT_AND_BACK);
  DEBUG_ASSERT(GetGLStateInt(GL_COLOR_MATERIAL_PARAMETER) == GL_AMBIENT_AND_DIFFUSE);

  // Lighting
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHTING));

  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT0));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT1));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT2));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT3));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT4));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT5));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT6));
  DEBUG_ASSERT(!glIsEnabled(GL_LIGHT7));

  glGetFloatv(GL_LIGHT_MODEL_AMBIENT, resultsf);
  DEBUG_ASSERT(resultsf[0] < 0.001f && resultsf[1] < 0.001f && resultsf[2] < 0.001f && resultsf[3] < 0.001f);

  if (g_app->m_location)
  {
    for (int i = 0; i < g_app->m_location->m_lights.Size(); i++)
    {
      Light* light = g_app->m_location->m_lights.GetData(i);

      float amb = 0.0f;
      GLfloat ambCol1[] = {amb, amb, amb, 1.0f};

      GLfloat pos1_actual[4];
      GLfloat ambient1_actual[4];
      GLfloat diffuse1_actual[4];
      GLfloat specular1_actual[4];

      glGetLightfv(GL_LIGHT0 + i, GL_POSITION, pos1_actual);
      glGetLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse1_actual);
      glGetLightfv(GL_LIGHT0 + i, GL_SPECULAR, specular1_actual);
      glGetLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient1_actual);

      for (int i = 0; i < 4; i++)
      {
        //			DEBUG_ASSERT(fabsf(lightPos1[i] - pos1_actual[i]) < 0.001f);
        //			DEBUG_ASSERT(fabsf(light->m_colour[i] - diffuse1_actual[i]) < 0.001f);
        //			DEBUG_ASSERT(fabsf(light->m_colour[i] - specular1_actual[i]) < 0.0001f);
        //			DEBUG_ASSERT(fabsf(ambCol1[i] - ambient1_actual[i]) < 0.001f);
      }
    }
  }

  // Blending, Anti-aliasing, Fog and Polygon Offset
  //	DEBUG_ASSERT(!glIsEnabled(GL_BLEND));
  DEBUG_ASSERT(GetGLStateInt(GL_BLEND_DST) == GL_ONE_MINUS_SRC_ALPHA);
  DEBUG_ASSERT(GetGLStateInt(GL_BLEND_SRC) == GL_SRC_ALPHA);
  DEBUG_ASSERT(!glIsEnabled(GL_ALPHA_TEST));
  DEBUG_ASSERT(GetGLStateInt(GL_ALPHA_TEST_FUNC) == GL_GREATER);
  DEBUG_ASSERT(GetGLStateFloat(GL_ALPHA_TEST_REF) == 0.01f);
  DEBUG_ASSERT(!glIsEnabled(GL_FOG));
  DEBUG_ASSERT(GetGLStateFloat(GL_FOG_DENSITY) == 1.0f);
  DEBUG_ASSERT(GetGLStateFloat(GL_FOG_END) >= 4000.0f);
  //DEBUG_ASSERT(GetGLStateFloat(GL_FOG_START) >= 1000.0f);
  glGetFloatv(GL_FOG_COLOR, resultsf);
  //	DEBUG_ASSERT(fabsf(resultsf[0] - g_app->m_location->m_backgroundColour.r/255.0f) < 0.001f);
  //	DEBUG_ASSERT(fabsf(resultsf[1] - g_app->m_location->m_backgroundColour.g/255.0f) < 0.001f);
  //	DEBUG_ASSERT(fabsf(resultsf[2] - g_app->m_location->m_backgroundColour.b/255.0f) < 0.001f);
  //	DEBUG_ASSERT(fabsf(resultsf[3] - g_app->m_location->m_backgroundColour.a/255.0f) < 0.001f);
  DEBUG_ASSERT(GetGLStateInt(GL_FOG_MODE) == GL_LINEAR);
  DEBUG_ASSERT(!glIsEnabled(GL_LINE_SMOOTH));
  DEBUG_ASSERT(!glIsEnabled(GL_POINT_SMOOTH));

  // Texture Mapping
  DEBUG_ASSERT(!glIsEnabled(GL_TEXTURE_2D));
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, results);
  DEBUG_ASSERT(results[0] == GL_CLAMP);
  glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, results);
  DEBUG_ASSERT(results[0] == GL_CLAMP);
  glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, results);
  DEBUG_ASSERT(results[0] == GL_MODULATE);
  glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, results);
  DEBUG_ASSERT(results[0] == 0);
  DEBUG_ASSERT(results[1] == 0);
  DEBUG_ASSERT(results[2] == 0);
  DEBUG_ASSERT(results[3] == 0);

  // Frame Buffer
  DEBUG_ASSERT(glIsEnabled(GL_DEPTH_TEST));
  DEBUG_ASSERT(GetGLStateInt(GL_DEPTH_WRITEMASK) != 0);
  DEBUG_ASSERT(GetGLStateInt(GL_DEPTH_FUNC) == GL_LEQUAL);
  DEBUG_ASSERT(glIsEnabled(GL_SCISSOR_TEST) == 0);

  // Hints
  DEBUG_ASSERT(GetGLStateInt(GL_FOG_HINT) == GL_DONT_CARE);
  DEBUG_ASSERT(GetGLStateInt(GL_POLYGON_SMOOTH_HINT) == GL_DONT_CARE);
}

void Renderer::SetOpenGLState() const
{
  // Geometry
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK, GL_FILL);
  glShadeModel(GL_FLAT);
  glDisable(GL_NORMALIZE);

  // Colour
  glDisable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  // Lighting
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHT2);
  glDisable(GL_LIGHT3);
  glDisable(GL_LIGHT4);
  glDisable(GL_LIGHT5);
  glDisable(GL_LIGHT6);
  glDisable(GL_LIGHT7);
  if (g_app->m_location)
    g_app->m_location->SetupLights();
  else
    g_app->m_globalWorld->SetupLights();
  float ambient[] = {0, 0, 0, 0};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

  // Blending, Anti-aliasing, Fog and Polygon Offset
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.01);
  if (g_app->m_location)
    g_app->m_location->SetupFog();
  else
    g_app->m_globalWorld->SetupFog();
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_POINT_SMOOTH);

  // Texture Mapping
  glDisable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  int colour[4] = {0, 0, 0, 0};
  glTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour);

  // Frame Buffer
  glEnable(GL_DEPTH_TEST);
  glDepthMask(true);
  glDepthFunc(GL_LEQUAL);
  //	glStencilMask	(0x00);
  //	glScissor		(400, 100, 400, 400);
  glDisable(GL_SCISSOR_TEST);

  // Hints
  glHint(GL_FOG_HINT, GL_DONT_CARE);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
}

void Renderer::SetObjectLighting() const
{
  float spec = 0.0f;
  float diffuse = 1.0f;
  float amb = 0.0f;
  GLfloat materialShininess[] = {127.0f};
  GLfloat materialSpecular[] = {spec, spec, spec, 0.0f};
  GLfloat materialDiffuse[] = {diffuse, diffuse, diffuse, 1.0f};
  GLfloat ambCol[] = {amb, amb, amb, 1.0f};

  glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
  glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
  glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
}

void Renderer::UnsetObjectLighting() const
{
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
}

void Renderer::UpdateTotalMatrix()
{
  double m[16];
  double p[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, m);
  glGetDoublev(GL_PROJECTION_MATRIX, p);

  DEBUG_ASSERT(m[3] == 0.0);
  DEBUG_ASSERT(m[7] == 0.0);
  DEBUG_ASSERT(m[11] == 0.0);
  DEBUG_ASSERT(NearlyEquals(m[15], 1.0));

  DEBUG_ASSERT(p[1] == 0.0);
  DEBUG_ASSERT(p[2] == 0.0);
  DEBUG_ASSERT(p[3] == 0.0);
  DEBUG_ASSERT(p[4] == 0.0);
  DEBUG_ASSERT(p[6] == 0.0);
  DEBUG_ASSERT(p[7] == 0.0);
  DEBUG_ASSERT(p[8] == 0.0);
  DEBUG_ASSERT(p[9] == 0.0);
  DEBUG_ASSERT(p[12] == 0.0);
  DEBUG_ASSERT(p[13] == 0.0);
  DEBUG_ASSERT(p[15] == 0.0);

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
  Matrix34 total = _frag->m_transform * _transform; LegacyVector3 worldPos = _frag->m_centre * total;

  // Return early if this shape fragment isn't on the screen
  {
    if (!g_app->m_camera->SphereInViewFrustum(worldPos, _frag->m_radius))
      return;
  } if (_frag->m_radius > 0.0f)
    RasteriseSphere(worldPos, _frag->m_radius);

  // Recurse into all child fragments
  int numChildren = _frag->m_childFragments.Size(); for (int i = 0; i < numChildren; ++i)
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
