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

namespace
{
  struct StartSeqVertex
  {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texcoord;
    DirectX::XMFLOAT3 normal;
  };

  struct StartSeqCB
  {
    DirectX::XMMATRIX worldViewProj;
  };

  static const int MAX_GRID_VERTICES = 1024;
}

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

  InitD3DResources();
}

StartSequence::~StartSequence()
{
  ShutdownD3DResources();
}

void StartSequence::InitD3DResources()
{
  m_overlayVB = nullptr;
  m_cursorVB = nullptr;
  m_gridVB = nullptr;
  m_cbPerDraw = nullptr;

  auto* device = g_renderDevice->GetDevice();
  HRESULT hr;

  D3D11_BUFFER_DESC vbd = {};
  vbd.Usage = D3D11_USAGE_DYNAMIC;
  vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  vbd.ByteWidth = 6 * sizeof(StartSeqVertex);
  hr = device->CreateBuffer(&vbd, nullptr, &m_overlayVB);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create start sequence overlay VB");

  hr = device->CreateBuffer(&vbd, nullptr, &m_cursorVB);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create start sequence cursor VB");

  vbd.ByteWidth = MAX_GRID_VERTICES * sizeof(StartSeqVertex);
  hr = device->CreateBuffer(&vbd, nullptr, &m_gridVB);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create start sequence grid VB");

  D3D11_BUFFER_DESC cbd = {};
  cbd.ByteWidth = sizeof(StartSeqCB);
  cbd.Usage = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  hr = device->CreateBuffer(&cbd, nullptr, &m_cbPerDraw);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create start sequence CB");
}

void StartSequence::ShutdownD3DResources()
{
  if (m_overlayVB) { m_overlayVB->Release(); m_overlayVB = nullptr; }
  if (m_cursorVB)  { m_cursorVB->Release();  m_cursorVB = nullptr; }
  if (m_gridVB)    { m_gridVB->Release();    m_gridVB = nullptr; }
  if (m_cbPerDraw) { m_cbPerDraw->Release(); m_cbPerDraw = nullptr; }
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

  XMMATRIX orthoProj = XMMatrixOrthographicOffCenterRH(0, 800, static_cast<float>(screenH), 0, -1, 1);

  // Set ImRenderer matrices for g_gameFont compatibility
  g_imRenderer->SetProjectionMatrix(orthoProj);
  g_imRenderer->SetViewMatrix(XMMatrixIdentity());
  g_imRenderer->LoadIdentity();

  auto* ctx = g_renderDevice->GetContext();

  g_renderStates->SetRasterState(ctx, RASTER_CULL_NONE);
  g_renderStates->SetDepthState(ctx, DEPTH_DISABLED);

  float timeNow = GetHighResTime() - m_startTime;

  // Helper: update constant buffer with WVP matrix
  auto updateCB = [&](const XMMATRIX& wvp)
  {
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_cbPerDraw, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
      static_cast<StartSeqCB*>(mapped.pData)->worldViewProj = XMMatrixTranspose(wvp);
      ctx->Unmap(m_cbPerDraw, 0);
    }
  };

  // Helper: bind pipeline for colored (non-textured) direct draws
  auto bindColoredPipeline = [&](ID3D11Buffer* vb, D3D11_PRIMITIVE_TOPOLOGY topology)
  {
    UINT stride = static_cast<UINT>(sizeof(StartSeqVertex));
    UINT vbOffset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &vbOffset);
    ctx->IASetInputLayout(g_imRenderer->GetInputLayout());
    ctx->IASetPrimitiveTopology(topology);
    ctx->VSSetShader(g_imRenderer->GetVertexShader(), nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &m_cbPerDraw);
    ctx->PSSetShader(g_imRenderer->GetColoredPixelShader(), nullptr, 0);
  };

  // Helper: fill a 6-vertex quad (2 triangles) into a dynamic VB
  auto fillQuadVB = [&](ID3D11Buffer* vb, float x0, float y0, float x1, float y1,
                         float cr, float cg, float cb, float ca) -> bool
  {
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(ctx->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
      return false;

    auto* v = static_cast<StartSeqVertex*>(mapped.pData);
    XMFLOAT4 col = {cr, cg, cb, ca};
    XMFLOAT2 uv = {0.0f, 0.0f};
    XMFLOAT3 nrm = {0.0f, 0.0f, 1.0f};

    v[0] = {{x0, y0, 0.0f}, col, uv, nrm};
    v[1] = {{x1, y0, 0.0f}, col, uv, nrm};
    v[2] = {{x1, y1, 0.0f}, col, uv, nrm};
    v[3] = {{x0, y0, 0.0f}, col, uv, nrm};
    v[4] = {{x1, y1, 0.0f}, col, uv, nrm};
    v[5] = {{x0, y1, 0.0f}, col, uv, nrm};

    ctx->Unmap(vb, 0);
    return true;
  };

  // Fade-in overlay
  if (timeNow < 3.0f)
  {
    float alpha = 1.0f - timeNow / 3.0f;
    if (fillQuadVB(m_overlayVB, 0, 0, 800, static_cast<float>(screenH), 0.0f, 0.0f, 0.0f, alpha))
    {
      updateCB(orthoProj);
      bindColoredPipeline(m_overlayVB, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ctx->Draw(6, 0);
    }
  }

  // Fade-out overlay
  if (timeNow > 87)
  {
    float alpha = (timeNow - 87) / 2.0f;
    if (fillQuadVB(m_overlayVB, 0, 0, 800, static_cast<float>(screenH), 1.0f, 1.0f, 1.0f, alpha))
    {
      updateCB(orthoProj);
      bindColoredPipeline(m_overlayVB, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ctx->Draw(6, 0);
    }
  }

  // Captions (rendered through g_gameFont -> ImRenderer)
  g_imRenderer->Color4f(1.0f, 1.0f, 1.0f, 1.0f);

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

      g_gameFont.SetColour(1.0f, 1.0f, 1.0f, 0.8f);
      g_gameFont.DrawText2D(caption->m_x, caption->m_y, caption->m_size, theString);

      int finishedLen = strlen(theString);
      int texW = g_gameFont.GetTextWidth(finishedLen, caption->m_size);
      cursorPos.Set(caption->m_x + texW, caption->m_y - 7.25f);
      cursorFlash = maxTimeLength > stringLength;
      cursorSize = caption->m_size;
    }
  }

  // Cursor quad
  if (cursorPos != Vector2(0, 0))
  {
    if (!cursorFlash || fmod(timeNow, 1) < 0.5f)
    {
      float cx = cursorPos.x;
      float cy = cursorPos.y;
      float cw = cursorSize * 0.7f;
      float ch = cursorSize * 0.88f;
      if (fillQuadVB(m_cursorVB, cx, cy, cx + cw, cy + ch, 1.0f, 1.0f, 1.0f, 0.8f))
      {
        updateCB(orthoProj);
        bindColoredPipeline(m_cursorVB, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->Draw(6, 0);
      }
    }
  }

  g_app->m_renderer->SetupMatricesFor3D();

  //
  // Render grid

  float scale = 1000.0f;
  float gridR = 2.0f;
  float height = -400.0f;
  float gridSize = 100.0f;

  float xStart = -4000.0f * gridR;
  float xEnd = 4000.0f + 4000.0f * gridR;
  float zStart = -4000.0f * gridR;
  float zEnd = 4000.0f + 4000.0f * gridR;

  if (timeNow > 50.0f)
  {
    g_renderStates->SetBlendState(ctx, BLEND_ADDITIVE);
    g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_WRITE);

    float percentDrawn = 1.0f - (timeNow - 50.0f) / 10.0f;
    percentDrawn = max(percentDrawn, 0.0f);
    xEnd -= (8000 + 4000 * gridR * percentDrawn);
    zEnd -= (8000 + 4000 * gridR * percentDrawn);

    // Batch all grid lines into a single VB
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_gridVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
      auto* v = static_cast<StartSeqVertex*>(mapped.pData);
      int vertCount = 0;
      XMFLOAT4 gridCol = {0.5f, 0.5f, 1.0f, 0.5f};
      XMFLOAT2 uv = {0.0f, 0.0f};
      XMFLOAT3 nrm = {0.0f, 0.0f, 1.0f};

      for (float gx = xStart; gx < xEnd && vertCount + 4 <= MAX_GRID_VERTICES; gx += gridSize)
      {
        v[vertCount++] = {{gx, height, zStart}, gridCol, uv, nrm};
        v[vertCount++] = {{gx, height, zEnd}, gridCol, uv, nrm};
        v[vertCount++] = {{xStart, height, gx}, gridCol, uv, nrm};
        v[vertCount++] = {{xEnd, height, gx}, gridCol, uv, nrm};
      }

      ctx->Unmap(m_gridVB, 0);

      if (vertCount > 0)
      {
        XMMATRIX gridWorld = XMMatrixScaling(scale, scale, scale);
        XMMATRIX gridWVP = gridWorld * g_imRenderer->GetViewMatrix() * g_imRenderer->GetProjectionMatrix();
        updateCB(gridWVP);
        bindColoredPipeline(m_gridVB, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        ctx->Draw(vertCount, 0);
      }
    }

    g_renderStates->SetBlendState(ctx, BLEND_DISABLED);
    g_renderStates->SetDepthState(ctx, DEPTH_ENABLED_WRITE);

    g_app->m_globalWorld->SetupFog();
  }
}
