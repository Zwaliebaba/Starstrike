#include "pch.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "filesys_utils.h"
#include "resource.h"
#include "LegacyVector3.h"
#include "app.h"
#include "camera.h"
#include "text_renderer.h"
#include "sprite_batch.h"
#include "im_renderer.h"
#include "render_device.h"
#include "render_states.h"

TextRenderer g_gameFont;
TextRenderer g_editorFont;

// Horizontal size as a proportion of vertical size
#define HORIZONTAL_SIZE		0.6f

#define TEX_MARGIN			0.003f
#define TEX_STRETCH			(1.0f - (26.0f * TEX_MARGIN))

#define TEX_WIDTH			((1.0f / 16.0f) * TEX_STRETCH * 0.9f)
#define TEX_HEIGHT			((1.0f / 14.0f) * TEX_STRETCH)

void TextRenderer::Initialise(const char* _filename)
{
  m_filename = strdup(_filename);
  BuildRenderState();
  m_renderShadow = false;
  m_renderOutline = false;
  m_colour = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void TextRenderer::BuildRenderState()
{
  BinaryReader* reader = g_app->m_resource->GetBinaryReader(m_filename);
  const char* extension = GetExtensionPart(m_filename);
  BitmapRGBA bmp(reader, extension);
  delete reader;

  m_bitmapWidth = bmp.m_width * 2;
  m_bitmapHeight = bmp.m_height * 2;

  BitmapRGBA scaledUp(m_bitmapWidth, m_bitmapHeight);
  scaledUp.Blit(0, 0, bmp.m_width, bmp.m_height, &bmp, 0, 0, scaledUp.m_width, scaledUp.m_height, false);
  m_textureID = scaledUp.ConvertToTexture();
}

void TextRenderer::BeginText2D()
{
  // SpriteBatch handles its own ortho projection internally for text draws.
  // This method is retained for callers that also use g_imRenderer between
  // BeginText2D/EndText2D — we still set the ortho projection on ImRenderer
  // so that any non-text ImRenderer draws in that bracket work correctly.
  if (g_imRenderer)
  {
    m_savedProjMatrix = g_imRenderer->GetProjectionMatrix();
    m_savedViewMatrix = g_imRenderer->GetViewMatrix();
    m_savedWorldMatrix = g_imRenderer->GetWorldMatrix();

    int w = g_renderDevice->GetBackBufferWidth();
    int h = g_renderDevice->GetBackBufferHeight();
    float left   = -0.325f;
    float right  = static_cast<float>(w) - 0.325f;
    float bottom = static_cast<float>(h) - 0.325f;
    float top    = -0.325f;
    g_imRenderer->SetProjectionMatrix(XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1.0f, 1.0f));
    g_imRenderer->SetViewMatrix(XMMatrixIdentity());
    g_imRenderer->SetWorldMatrix(XMMatrixIdentity());
  }
}

void TextRenderer::EndText2D()
{
  if (g_imRenderer)
  {
    g_imRenderer->SetProjectionMatrix(m_savedProjMatrix);
    g_imRenderer->SetViewMatrix(m_savedViewMatrix);
    g_imRenderer->SetWorldMatrix(m_savedWorldMatrix);
  }
}

float TextRenderer::GetTexCoordX(unsigned char theChar)
{
  constexpr float charWidth = 1.0f / 16.0f; // In OpenGL texture UV space
  float xPos = (theChar % 16);
  float texX = xPos * charWidth + TEX_MARGIN + 0.002f;
  return texX;
}

float TextRenderer::GetTexCoordY(unsigned char theChar)
{
  constexpr float charHeight = 1.0f / 14.0f;
  float yPos = (theChar >> 4) - 2;
  float texY = yPos * charHeight; // In OpenGL texture UV space
  texY = 1.0f - texY - charHeight + TEX_MARGIN + 0.001f;
  return texY;
}

void TextRenderer::DrawText2DUp(float _x, float _y, float _size, const char* _text, ...)
{
  float horiSize = _size * HORIZONTAL_SIZE;
  int screenW = g_renderDevice->GetBackBufferWidth();
  int screenH = g_renderDevice->GetBackBufferHeight();
  BlendStateId blend = m_renderShadow ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE;

  g_spriteBatch->Begin2D(screenW, screenH, blend);
  g_spriteBatch->SetTexture(m_textureID);
  g_spriteBatch->SetColor(m_colour.x, m_colour.y, m_colour.z, m_colour.w);

  unsigned numChars = strlen(_text);
  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      // Rotated 90 CCW — vertices map UV corners to rotated screen positions
      DirectX::XMFLOAT3 v0 = { _x,         _y + horiSize, 0.0f };
      DirectX::XMFLOAT3 v1 = { _x,         _y,            0.0f };   // uv: texX+W,       texY+H
      DirectX::XMFLOAT3 v2 = { _x + _size, _y,            0.0f };   // uv: texX+W,       texY
      DirectX::XMFLOAT3 v3 = { _x + _size, _y + horiSize, 0.0f };   // uv: texX,         texY
      g_spriteBatch->AddQuad3D(v0, v1, v2, v3,
                               texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
    }

    _y -= horiSize;
  }

  g_spriteBatch->End2D();
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
}

void TextRenderer::DrawText2DDown(float _x, float _y, float _size, const char* _text, ...)
{
  float horiSize = _size * HORIZONTAL_SIZE;
  int screenW = g_renderDevice->GetBackBufferWidth();
  int screenH = g_renderDevice->GetBackBufferHeight();
  BlendStateId blend = (m_renderShadow || m_renderOutline) ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE;

  g_spriteBatch->Begin2D(screenW, screenH, blend);
  g_spriteBatch->SetTexture(m_textureID);
  g_spriteBatch->SetColor(m_colour.x, m_colour.y, m_colour.z, m_colour.w);

  unsigned numChars = strlen(_text);
  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      if (m_renderOutline)
      {
        for (int x = -1; x <= 1; ++x)
        {
          for (int y = -1; y <= 1; ++y)
          {
            if (x != 0 || y != 0)
            {
              // Rotated 90 CW — same vertex layout as original
              DirectX::XMFLOAT3 v0 = { _x + x + _size, _y + (float)y,            0.0f };
              DirectX::XMFLOAT3 v1 = { _x + x + _size, _y + (float)y + horiSize, 0.0f };
              DirectX::XMFLOAT3 v2 = { _x + (float)x,  _y + (float)y + horiSize, 0.0f };
              DirectX::XMFLOAT3 v3 = { _x + (float)x,  _y + (float)y,            0.0f };
              g_spriteBatch->AddQuad3D(v0, v1, v2, v3,
                                       texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
            }
          }
        }
      }
      else
      {
        DirectX::XMFLOAT3 v0 = { _x + _size, _y,            0.0f };
        DirectX::XMFLOAT3 v1 = { _x + _size, _y + horiSize, 0.0f };
        DirectX::XMFLOAT3 v2 = { _x,         _y + horiSize, 0.0f };
        DirectX::XMFLOAT3 v3 = { _x,         _y,            0.0f };
        g_spriteBatch->AddQuad3D(v0, v1, v2, v3,
                                 texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
      }
    }

    _y += horiSize;
  }

  g_spriteBatch->End2D();
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
}

void TextRenderer::SetRenderShadow(bool _renderShadow) { m_renderShadow = _renderShadow; }

void TextRenderer::SetRenderOutline(bool _renderOutline) { m_renderOutline = _renderOutline; }

void TextRenderer::SetColour(float _r, float _g, float _b, float _a) { m_colour = { _r, _g, _b, _a }; }

void TextRenderer::DrawText2DSimple(float _x, float _y, float _size, const char* _text)
{
  // Compatibility wank - needed to achieve the same behaviour the old code had
  _y -= 7.0f;
  _x -= 3.0f;

  float horiSize = _size * HORIZONTAL_SIZE;
  int screenW = g_renderDevice->GetBackBufferWidth();
  int screenH = g_renderDevice->GetBackBufferHeight();
  BlendStateId blend = (m_renderShadow || m_renderOutline) ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE;

  g_spriteBatch->Begin2D(screenW, screenH, blend);
  g_spriteBatch->SetTexture(m_textureID);
  g_spriteBatch->SetColor(m_colour.x, m_colour.y, m_colour.z, m_colour.w);

  unsigned numChars = strlen(_text);
  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      if (m_renderOutline)
      {
        for (int x = -1; x <= 1; ++x)
        {
          for (int y = -1; y <= 1; ++y)
          {
            if (x != 0 || y != 0)
            {
              g_spriteBatch->AddQuad2D(_x + x, _y + y, horiSize, _size,
                                       texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
            }
          }
        }
      }
      else
      {
        g_spriteBatch->AddQuad2D(_x, _y, horiSize, _size,
                                 texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
      }
    }

    _x += horiSize;
  }

  g_spriteBatch->End2D();
  g_renderStates->SetBlendState(g_renderDevice->GetContext(), BLEND_ALPHA);
}

// Draw the text, justified depending on the _xJustification parameter.
//		_xJustification < 0		Right justified text
//	    _xJustification == 0	Centred text
//		_xJustification > 0		Left justified text
void TextRenderer::DrawText2DJustified(float _x, float _y, float _size, int _xJustification, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  if (_xJustification > 0)
    DrawText2DSimple(_x, _y, _size, buf); // Left Justification
  else
  {
    float width = GetTextWidth(strlen(buf), _size);

    if (_xJustification < 0)
      DrawText2DSimple(_x - width, _y, _size, buf); // Right Justification
    else
      DrawText2DSimple(_x - width / 2, _y, _size, buf); // Centre
  }
}

void TextRenderer::DrawText2D(float _x, float _y, float _size, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);
  DrawText2DSimple(_x, _y, _size, buf);
}

void TextRenderer::DrawText2DRight(float _x, float _y, float _size, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  float width = GetTextWidth(strlen(buf), _size);
  DrawText2DSimple(_x - width, _y, _size, buf);
}

void TextRenderer::DrawText2DCentre(float _x, float _y, float _size, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  float width = GetTextWidth(strlen(buf), _size);
  DrawText2DSimple(_x - width / 2, _y, _size, buf);
}

namespace
{
  // RAII helper that saves and restores D3D11 render states around text drawing
  class SaveFontDrawAttributes
  {
    public:
      SaveFontDrawAttributes()
      {
        m_savedBlend = g_renderStates->GetCurrentBlendState();
        m_savedDepth = g_renderStates->GetCurrentDepthState();
        m_savedRaster = g_renderStates->GetCurrentRasterState();
      }

      ~SaveFontDrawAttributes()
      {
        auto* ctx = g_renderDevice->GetContext();
        g_renderStates->SetBlendState(ctx, m_savedBlend);
        g_renderStates->SetDepthState(ctx, m_savedDepth);
        g_renderStates->SetRasterState(ctx, m_savedRaster);
      }

    private:
      BlendStateId m_savedBlend;
      DepthStateId m_savedDepth;
      RasterStateId m_savedRaster;
  };
}

void TextRenderer::DrawText3DSimple(const LegacyVector3& _pos, float _size, const char* _text)
{
  // Store render states
  SaveFontDrawAttributes saveAttribs;

  Camera* cam = g_app->m_camera;
  LegacyVector3 pos(_pos);
  LegacyVector3 vertSize = cam->GetUp() * _size;
  LegacyVector3 horiSize = -cam->GetRight() * _size * HORIZONTAL_SIZE;
  unsigned int numChars = strlen(_text);
  pos += vertSize * 0.5f;

  //
  // Render the text

  BlendStateId blend = m_renderShadow ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE;
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);
  g_renderStates->SetDepthState(g_renderDevice->GetContext(), DEPTH_DISABLED);

  DirectX::XMMATRIX viewProj = g_imRenderer->GetViewMatrix() * g_imRenderer->GetProjectionMatrix();
  g_spriteBatch->Begin3D(viewProj, blend);
  g_spriteBatch->SetTexture(m_textureID);
  g_spriteBatch->SetColor(m_colour.x, m_colour.y, m_colour.z, m_colour.w);

  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = _text[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      LegacyVector3 p0 = pos;
      LegacyVector3 p1 = pos + horiSize;
      LegacyVector3 p2 = pos + horiSize - vertSize;
      LegacyVector3 p3 = pos - vertSize;

      DirectX::XMFLOAT3 v0 = { p0.x, p0.y, p0.z };
      DirectX::XMFLOAT3 v1 = { p1.x, p1.y, p1.z };
      DirectX::XMFLOAT3 v2 = { p2.x, p2.y, p2.z };
      DirectX::XMFLOAT3 v3 = { p3.x, p3.y, p3.z };

      g_spriteBatch->AddQuad3D(v0, v1, v2, v3,
                               texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
    }

    pos += horiSize;
  }

  g_spriteBatch->End3D();
}

void TextRenderer::DrawText3D(const LegacyVector3& _pos, float _size, const char* _text, ...)
{
  // Convert the variable argument list into a single string
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  DrawText3DSimple(_pos, _size, buf);
}

void TextRenderer::DrawText3DCentre(const LegacyVector3& _pos, float _size, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  LegacyVector3 pos = _pos;
  float amount = HORIZONTAL_SIZE * static_cast<float>(strlen(buf)) * 0.5f * _size;
  pos += g_app->m_camera->GetRight() * amount;

  DrawText3DSimple(pos, _size, buf);
}

void TextRenderer::DrawText3DRight(const LegacyVector3& _pos, float _size, const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  LegacyVector3 pos = _pos;
  float amount = HORIZONTAL_SIZE * static_cast<float>(strlen(buf)) * _size;
  pos += g_app->m_camera->GetRight() * amount;

  DrawText3DSimple(pos, _size, buf);
}

void TextRenderer::DrawText3D(const LegacyVector3& _pos, const LegacyVector3& _front, const LegacyVector3& _up, float _size,
                              const char* _text, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, _text);
  vsprintf(buf, _text, ap);

  SaveFontDrawAttributes saveAttribs;

  LegacyVector3 pos = _pos;
  LegacyVector3 vertSize = _up * _size;
  LegacyVector3 horiSize = (_up ^ _front) * _size * HORIZONTAL_SIZE;
  unsigned int numChars = strlen(buf);
  pos -= horiSize * numChars * 0.5f;
  pos += vertSize * 0.5f;

  //
  // Render the text

  BlendStateId blend = m_renderShadow ? BLEND_SUBTRACTIVE_COLOR : BLEND_ADDITIVE;
  g_renderStates->SetRasterState(g_renderDevice->GetContext(), RASTER_CULL_NONE);

  DirectX::XMMATRIX viewProj = g_imRenderer->GetViewMatrix() * g_imRenderer->GetProjectionMatrix();
  g_spriteBatch->Begin3D(viewProj, blend);
  g_spriteBatch->SetTexture(m_textureID);
  g_spriteBatch->SetColor(m_colour.x, m_colour.y, m_colour.z, m_colour.w);

  for (unsigned int i = 0; i < numChars; ++i)
  {
    unsigned char thisChar = buf[i];

    if (thisChar > 32)
    {
      float texX = GetTexCoordX(thisChar);
      float texY = GetTexCoordY(thisChar);

      LegacyVector3 p0 = pos;
      LegacyVector3 p1 = pos + horiSize;
      LegacyVector3 p2 = pos + horiSize - vertSize;
      LegacyVector3 p3 = pos - vertSize;

      DirectX::XMFLOAT3 v0 = { p0.x, p0.y, p0.z };
      DirectX::XMFLOAT3 v1 = { p1.x, p1.y, p1.z };
      DirectX::XMFLOAT3 v2 = { p2.x, p2.y, p2.z };
      DirectX::XMFLOAT3 v3 = { p3.x, p3.y, p3.z };

      g_spriteBatch->AddQuad3D(v0, v1, v2, v3,
                               texX, texY + TEX_HEIGHT, texX + TEX_WIDTH, texY);
    }

    pos += horiSize;
  }

  g_spriteBatch->End3D();
}

float TextRenderer::GetTextWidth(unsigned int _numChars, float _size) { return _numChars * _size * HORIZONTAL_SIZE; }
