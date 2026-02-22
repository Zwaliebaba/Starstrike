#include "pch.h"
#include "sprite_batch.h"
#include "render_device.h"
#include "render_states.h"
#include "texture_manager.h"
#include "debug_utils.h"

#include "CompiledShaders/sprite_vs.h"
#include "CompiledShaders/sprite_ps.h"

SpriteBatch* g_spriteBatch = nullptr;

// ---------------------------------------------------------------------------
// SpriteBatch
// ---------------------------------------------------------------------------

SpriteBatch::SpriteBatch()
  : m_device(nullptr),
    m_context(nullptr),
    m_vertexBuffer(nullptr),
    m_indexBuffer(nullptr),
    m_constantBuffer(nullptr),
    m_vertexShader(nullptr),
    m_pixelShader(nullptr),
    m_inputLayout(nullptr),
    m_quadCount(0),
    m_inBeginEnd(false),
    m_currentColor(1.0f, 1.0f, 1.0f, 1.0f),
    m_boundSRV(nullptr),
    m_currentBlend(BLEND_ALPHA)
{
  m_viewProj = DirectX::XMMatrixIdentity();
}

SpriteBatch::~SpriteBatch()
{
  Shutdown();
}

void SpriteBatch::Initialise(ID3D11Device* device, ID3D11DeviceContext* context)
{
  m_device  = device;
  m_context = context;

  m_batch.reserve(MAX_VERTICES);

  HRESULT hr;

  // --- Create shaders from pre-compiled bytecode ---
  hr = m_device->CreateVertexShader(g_psprite_vs, sizeof(g_psprite_vs), nullptr, &m_vertexShader);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch vertex shader");

  hr = m_device->CreatePixelShader(g_psprite_ps, sizeof(g_psprite_ps), nullptr, &m_pixelShader);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch pixel shader");

  // --- Input layout ---
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, offsetof(SpriteVertex, pos),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(SpriteVertex, uv),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SpriteVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  hr = m_device->CreateInputLayout(layout, _countof(layout), g_psprite_vs, sizeof(g_psprite_vs), &m_inputLayout);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch input layout");

  // --- Dynamic vertex buffer ---
  D3D11_BUFFER_DESC vbd = {};
  vbd.ByteWidth     = MAX_VERTICES * sizeof(SpriteVertex);
  vbd.Usage          = D3D11_USAGE_DYNAMIC;
  vbd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  hr = m_device->CreateBuffer(&vbd, nullptr, &m_vertexBuffer);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch vertex buffer");

  // --- Static index buffer (pre-built quad pattern) ---
  std::vector<UINT16> indices(MAX_INDICES);
  for (int i = 0; i < MAX_QUADS; ++i)
  {
    int vi = i * 4;
    int ii = i * 6;
    indices[ii + 0] = static_cast<UINT16>(vi + 0);
    indices[ii + 1] = static_cast<UINT16>(vi + 1);
    indices[ii + 2] = static_cast<UINT16>(vi + 2);
    indices[ii + 3] = static_cast<UINT16>(vi + 0);
    indices[ii + 4] = static_cast<UINT16>(vi + 2);
    indices[ii + 5] = static_cast<UINT16>(vi + 3);
  }

  D3D11_BUFFER_DESC ibd = {};
  ibd.ByteWidth     = MAX_INDICES * sizeof(UINT16);
  ibd.Usage          = D3D11_USAGE_IMMUTABLE;
  ibd.BindFlags      = D3D11_BIND_INDEX_BUFFER;

  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = indices.data();

  hr = m_device->CreateBuffer(&ibd, &initData, &m_indexBuffer);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch index buffer");

  // --- Constant buffer ---
  D3D11_BUFFER_DESC cbd = {};
  cbd.ByteWidth     = sizeof(CBSprite);
  cbd.Usage          = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  hr = m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SpriteBatch constant buffer");

  DebugTrace("SpriteBatch initialised\n");
}

void SpriteBatch::Shutdown()
{
  SAFE_RELEASE(m_constantBuffer);
  SAFE_RELEASE(m_indexBuffer);
  SAFE_RELEASE(m_vertexBuffer);
  SAFE_RELEASE(m_inputLayout);
  SAFE_RELEASE(m_pixelShader);
  SAFE_RELEASE(m_vertexShader);
  m_device  = nullptr;
  m_context = nullptr;
}

// ---------------------------------------------------------------------------
// Begin / End
// ---------------------------------------------------------------------------

void SpriteBatch::Begin2D(int screenW, int screenH, BlendStateId blend)
{
  DarwiniaReleaseAssert(!m_inBeginEnd, "SpriteBatch::Begin2D called while already in Begin/End");

  // Ortho projection: origin top-left, Y down, matching the 2D text coordinate system
  float left   = -0.325f;
  float right  = static_cast<float>(screenW) - 0.325f;
  float bottom = static_cast<float>(screenH) - 0.325f;
  float top    = -0.325f;
  m_viewProj = DirectX::XMMatrixOrthographicOffCenterRH(left, right, bottom, top, -1.0f, 1.0f);

  m_currentBlend = blend;
  m_inBeginEnd   = true;
  m_quadCount    = 0;
  m_batch.clear();
}

void SpriteBatch::End2D()
{
  DarwiniaReleaseAssert(m_inBeginEnd, "SpriteBatch::End2D called without Begin2D");
  Flush();
  m_inBeginEnd = false;
}

void SpriteBatch::Begin3D(const DirectX::XMMATRIX& viewProj, BlendStateId blend)
{
  DarwiniaReleaseAssert(!m_inBeginEnd, "SpriteBatch::Begin3D called while already in Begin/End");

  m_viewProj     = viewProj;
  m_currentBlend = blend;
  m_inBeginEnd   = true;
  m_quadCount    = 0;
  m_batch.clear();
}

void SpriteBatch::End3D()
{
  DarwiniaReleaseAssert(m_inBeginEnd, "SpriteBatch::End3D called without Begin3D");
  Flush();
  m_inBeginEnd = false;
}

// ---------------------------------------------------------------------------
// State setters
// ---------------------------------------------------------------------------

void SpriteBatch::SetTexture(int textureId)
{
  ID3D11ShaderResourceView* srv = g_textureManager->GetSRV(textureId);
  if (srv != m_boundSRV)
  {
    Flush();
    m_boundSRV = srv;
  }
}

void SpriteBatch::SetTexture(ID3D11ShaderResourceView* srv)
{
  if (srv != m_boundSRV)
  {
    Flush();
    m_boundSRV = srv;
  }
}

void SpriteBatch::SetBlendState(BlendStateId blend)
{
  if (blend != m_currentBlend)
  {
    Flush();
    m_currentBlend = blend;
  }
}

void SpriteBatch::SetColor(float r, float g, float b, float a)
{
  m_currentColor = { r, g, b, a };
}

// ---------------------------------------------------------------------------
// AddQuad
// ---------------------------------------------------------------------------

void SpriteBatch::AddQuad2D(float x, float y, float w, float h,
                            float u0, float v0, float u1, float v1)
{
  if (m_quadCount >= MAX_QUADS)
    Flush();

  SpriteVertex tl = { { x,     y,     0.0f }, { u0, v0 }, m_currentColor };
  SpriteVertex tr = { { x + w, y,     0.0f }, { u1, v0 }, m_currentColor };
  SpriteVertex br = { { x + w, y + h, 0.0f }, { u1, v1 }, m_currentColor };
  SpriteVertex bl = { { x,     y + h, 0.0f }, { u0, v1 }, m_currentColor };

  m_batch.push_back(tl);
  m_batch.push_back(tr);
  m_batch.push_back(br);
  m_batch.push_back(bl);
  ++m_quadCount;
}

void SpriteBatch::AddQuad3D(const DirectX::XMFLOAT3& topLeft,
                            const DirectX::XMFLOAT3& topRight,
                            const DirectX::XMFLOAT3& bottomRight,
                            const DirectX::XMFLOAT3& bottomLeft,
                            float u0, float v0, float u1, float v1)
{
  if (m_quadCount >= MAX_QUADS)
    Flush();

  SpriteVertex v0v = { topLeft,     { u0, v0 }, m_currentColor };
  SpriteVertex v1v = { topRight,    { u1, v0 }, m_currentColor };
  SpriteVertex v2v = { bottomRight, { u1, v1 }, m_currentColor };
  SpriteVertex v3v = { bottomLeft,  { u0, v1 }, m_currentColor };

  m_batch.push_back(v0v);
  m_batch.push_back(v1v);
  m_batch.push_back(v2v);
  m_batch.push_back(v3v);
  ++m_quadCount;
}

// ---------------------------------------------------------------------------
// Flush
// ---------------------------------------------------------------------------

void SpriteBatch::Flush()
{
  if (m_quadCount == 0)
    return;

  UINT vertCount = static_cast<UINT>(m_batch.size());

  // Upload vertex data
  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hr = m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr))
    return;
  memcpy(mapped.pData, m_batch.data(), vertCount * sizeof(SpriteVertex));
  m_context->Unmap(m_vertexBuffer, 0);

  // Upload constant buffer
  DirectX::XMMATRIX vpT = DirectX::XMMatrixTranspose(m_viewProj);
  hr = m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr))
    return;
  auto* cb = static_cast<CBSprite*>(mapped.pData);
  cb->viewProj = vpT;
  m_context->Unmap(m_constantBuffer, 0);

  // Set render states
  g_renderStates->SetBlendState(m_context, m_currentBlend);

  // Bind pipeline state
  UINT stride = sizeof(SpriteVertex);
  UINT offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
  m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  m_context->VSSetShader(m_vertexShader, nullptr, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

  m_context->PSSetShader(m_pixelShader, nullptr, 0);
  if (m_boundSRV)
  {
    m_context->PSSetShaderResources(0, 1, &m_boundSRV);
    ID3D11SamplerState* sampler = g_textureManager->GetSampler(SAMPLER_LINEAR_CLAMP);
    m_context->PSSetSamplers(0, 1, &sampler);
  }

  // Draw
  UINT indexCount = static_cast<UINT>(m_quadCount) * 6;
  m_context->DrawIndexed(indexCount, 0, 0);

  // Unbind SRV to avoid resource hazards
  if (m_boundSRV)
  {
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullSRV);
  }

  // Reset batch
  m_batch.clear();
  m_quadCount = 0;
}
