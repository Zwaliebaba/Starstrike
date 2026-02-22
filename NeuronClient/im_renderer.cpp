#include "pch.h"
#include "im_renderer.h"
#include "texture_manager.h"
#include "debug_utils.h"

#include "CompiledShaders/im_defaultVS.h"
#include "CompiledShaders/im_coloredPS.h"
#include "CompiledShaders/im_texturedPS.h"

ImRenderer* g_imRenderer = nullptr;

// ---------------------------------------------------------------------------
// ImRenderer
// ---------------------------------------------------------------------------

ImRenderer::ImRenderer()
  : m_device(nullptr),
    m_context(nullptr),
    m_vertexBuffer(nullptr),
    m_constantBuffer(nullptr),
    m_vsDefault(nullptr),
    m_psColored(nullptr),
    m_psTextured(nullptr),
    m_inputLayout(nullptr),
    m_defaultSampler(nullptr),
    m_currentPrimitive(PRIM_TRIANGLES),
    m_inBeginEnd(false),
    m_drawEnabled(true),
    m_currentColor(1.0f, 1.0f, 1.0f, 1.0f),
    m_currentTexCoord(0.0f, 0.0f),
    m_currentNormal(0.0f, 0.0f, 1.0f),
    m_boundSRV(nullptr)
{
  m_projMatrix  = DirectX::XMMatrixIdentity();
  m_viewMatrix  = DirectX::XMMatrixIdentity();
  m_worldMatrix = DirectX::XMMatrixIdentity();
}

ImRenderer::~ImRenderer()
{
  Shutdown();
}

void ImRenderer::Initialise(ID3D11Device* device, ID3D11DeviceContext* context)
{
  m_device  = device;
  m_context = context;

  m_batch.reserve(4096);

  // --- Create shaders from pre-compiled bytecode ---
  HRESULT hr;

  hr = m_device->CreateVertexShader(g_pim_defaultVS, sizeof(g_pim_defaultVS), nullptr, &m_vsDefault);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create vertex shader");

  hr = m_device->CreatePixelShader(g_pim_coloredPS, sizeof(g_pim_coloredPS), nullptr, &m_psColored);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create colored pixel shader");
      
  hr = m_device->CreatePixelShader(g_pim_texturedPS, sizeof(g_pim_texturedPS), nullptr, &m_psTextured);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create textured pixel shader");

  // --- Input layout ---
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(ImVertex, pos),      D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, offsetof(ImVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, offsetof(ImVertex, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,     0, offsetof(ImVertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  hr = m_device->CreateInputLayout(layout, _countof(layout), g_pim_defaultVS, sizeof(g_pim_defaultVS), &m_inputLayout);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create input layout");

  // --- Dynamic vertex buffer ---
  D3D11_BUFFER_DESC vbd;
  ZeroMemory(&vbd, sizeof(vbd));
  vbd.ByteWidth      = MAX_VERTICES * sizeof(ImVertex);
  vbd.Usage           = D3D11_USAGE_DYNAMIC;
  vbd.BindFlags       = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;

  hr = m_device->CreateBuffer(&vbd, nullptr, &m_vertexBuffer);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create ImRenderer vertex buffer");

  // --- Constant buffer ---
  D3D11_BUFFER_DESC cbd;
  ZeroMemory(&cbd, sizeof(cbd));
  cbd.ByteWidth      = sizeof(CBPerDraw);
  cbd.Usage           = D3D11_USAGE_DYNAMIC;
  cbd.BindFlags       = D3D11_BIND_CONSTANT_BUFFER;
  cbd.CPUAccessFlags  = D3D11_CPU_ACCESS_WRITE;

  hr = m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create ImRenderer constant buffer");

  // --- Default sampler state ---
  D3D11_SAMPLER_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sd.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sd.MaxLOD         = D3D11_FLOAT32_MAX;

  hr = m_device->CreateSamplerState(&sd, &m_defaultSampler);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create ImRenderer default sampler");

  DebugTrace("ImRenderer initialised\n");
}

void ImRenderer::Shutdown()
{
  SAFE_RELEASE(m_defaultSampler);
  SAFE_RELEASE(m_constantBuffer);
  SAFE_RELEASE(m_vertexBuffer);
  SAFE_RELEASE(m_inputLayout);
  SAFE_RELEASE(m_psTextured);
  SAFE_RELEASE(m_psColored);
  SAFE_RELEASE(m_vsDefault);
  m_device  = nullptr;
  m_context = nullptr;
}

// ---------------------------------------------------------------------------
// Matrix state
// ---------------------------------------------------------------------------

void ImRenderer::SetProjectionMatrix(const DirectX::XMMATRIX& proj) { m_projMatrix = proj; }
void ImRenderer::SetViewMatrix(const DirectX::XMMATRIX& view)       { m_viewMatrix = view; }
void ImRenderer::SetWorldMatrix(const DirectX::XMMATRIX& world)     { m_worldMatrix = world; }

// ---------------------------------------------------------------------------
// Modelview matrix stack (mirrors OpenGL GL_MODELVIEW stack)
// ---------------------------------------------------------------------------

void ImRenderer::PushMatrix()
{
  DarwiniaDebugAssert(m_matrixStack.size() < MAX_MATRIX_STACK_DEPTH);
  m_matrixStack.push(m_worldMatrix);
}

void ImRenderer::PopMatrix()
{
  DarwiniaDebugAssert(!m_matrixStack.empty());
  m_worldMatrix = m_matrixStack.top();
  m_matrixStack.pop();
}

void ImRenderer::LoadIdentity()
{
  m_worldMatrix = DirectX::XMMatrixIdentity();
}

void ImRenderer::MultMatrixf(const float* m)
{
  // OpenGL glMultMatrixf takes column-major; XMMATRIX is row-major.
  // Reading column-major data sequentially into row-major storage
  // naturally transposes: M_dx = M_gl^T, then pre-multiply so that
  // the result matches OpenGL semantics (current = current * new).
  DirectX::XMMATRIX mat(
    m[0],  m[1],  m[2],  m[3],
    m[4],  m[5],  m[6],  m[7],
    m[8],  m[9],  m[10], m[11],
    m[12], m[13], m[14], m[15]);
  m_worldMatrix = mat * m_worldMatrix;
}

void ImRenderer::Translatef(float x, float y, float z)
{
  DirectX::XMMATRIX t = DirectX::XMMatrixTranslation(x, y, z);
  m_worldMatrix = t * m_worldMatrix;
}

void ImRenderer::Rotatef(float angleDeg, float x, float y, float z)
{
  float rad = DirectX::XMConvertToRadians(angleDeg);
  DirectX::XMVECTOR axis = DirectX::XMVectorSet(x, y, z, 0.0f);
  DirectX::XMMATRIX r = DirectX::XMMatrixRotationAxis(axis, rad);
  m_worldMatrix = r * m_worldMatrix;
}

void ImRenderer::Scalef(float sx, float sy, float sz)
{
  DirectX::XMMATRIX s = DirectX::XMMatrixScaling(sx, sy, sz);
  m_worldMatrix = s * m_worldMatrix;
}

// ---------------------------------------------------------------------------
// Texture binding
// ---------------------------------------------------------------------------

void ImRenderer::BindTexture(ID3D11ShaderResourceView* srv)   { m_boundSRV = srv; }

void ImRenderer::BindTexture(int textureId)
{
  if (g_textureManager)
    m_boundSRV = g_textureManager->GetSRV(textureId);
  else
    m_boundSRV = nullptr;
}

void ImRenderer::UnbindTexture()                               { m_boundSRV = nullptr; }

void ImRenderer::SetSampler(SamplerId id)
{
  if (g_textureManager)
  {
    ID3D11SamplerState* sampler = g_textureManager->GetSampler(id);
    if (sampler)
      m_context->PSSetSamplers(0, 1, &sampler);
  }
}

// ---------------------------------------------------------------------------
// Immediate-mode vertex attribute setters
// ---------------------------------------------------------------------------

void ImRenderer::Color3f(float r, float g, float b)                                                       { m_currentColor = { r, g, b, 1.0f }; }
void ImRenderer::Color3ub(unsigned char r, unsigned char g, unsigned char b)                               { m_currentColor = { r / 255.0f, g / 255.0f, b / 255.0f, 1.0f }; }
void ImRenderer::Color3ubv(const unsigned char* c)                                                         { m_currentColor = { c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, 1.0f }; }
void ImRenderer::Color4f(float r, float g, float b, float a)                                               { m_currentColor = { r, g, b, a }; }
void ImRenderer::Color4fv(const float* c)                                                                  { m_currentColor = { c[0], c[1], c[2], c[3] }; }
void ImRenderer::Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a)               { m_currentColor = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f }; }
void ImRenderer::Color4ubv(const unsigned char* c)                                                         { m_currentColor = { c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, c[3] / 255.0f }; }

void ImRenderer::TexCoord2f(float u, float v) { m_currentTexCoord = { u, v }; }
void ImRenderer::TexCoord2i(int u, int v)     { m_currentTexCoord = { static_cast<float>(u), static_cast<float>(v) }; }

void ImRenderer::Normal3f(float x, float y, float z) { m_currentNormal = { x, y, z }; }
void ImRenderer::Normal3fv(const float* n)            { m_currentNormal = { n[0], n[1], n[2] }; }

// ---------------------------------------------------------------------------
// Begin / Vertex / End
// ---------------------------------------------------------------------------

void ImRenderer::Begin(PrimitiveType type)
{
  DarwiniaDebugAssert(!m_inBeginEnd);
  m_currentPrimitive = type;
  m_inBeginEnd = true;
  m_batch.clear();
}

void ImRenderer::Vertex2f(float x, float y)
{
  ImVertex v;
  v.pos      = { x, y, 0.0f };
  v.color    = m_currentColor;
  v.texcoord = m_currentTexCoord;
  v.normal   = m_currentNormal;
  m_batch.push_back(v);
}

void ImRenderer::Vertex2fv(const float* p)
{
  Vertex2f(p[0], p[1]);
}

void ImRenderer::Vertex2i(int x, int y)
{
  Vertex2f(static_cast<float>(x), static_cast<float>(y));
}

void ImRenderer::Vertex3f(float x, float y, float z)
{
  ImVertex v;
  v.pos      = { x, y, z };
  v.color    = m_currentColor;
  v.texcoord = m_currentTexCoord;
  v.normal   = m_currentNormal;
  m_batch.push_back(v);
}

void ImRenderer::Vertex3fv(const float* p)
{
  Vertex3f(p[0], p[1], p[2]);
}

void ImRenderer::End()
{
  DarwiniaDebugAssert(m_inBeginEnd);
  m_inBeginEnd = false;

  if (m_batch.empty())
    return;

  // During transition: accept vertices (exercises the API) but don't draw
  if (!m_drawEnabled)
  {
    m_batch.clear();
    return;
  }

  switch (m_currentPrimitive)
  {
    case PRIM_TRIANGLES:
      Flush(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_batch);
      break;

    case PRIM_TRIANGLE_STRIP:
      Flush(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, m_batch);
      break;

    case PRIM_TRIANGLE_FAN:
    {
      // Convert fan to triangle list: for N verts, emit (N-2) triangles
      std::vector<ImVertex> tris;
      tris.reserve((m_batch.size() - 2) * 3);
      for (size_t i = 1; i + 1 < m_batch.size(); ++i)
      {
        tris.push_back(m_batch[0]);
        tris.push_back(m_batch[i]);
        tris.push_back(m_batch[i + 1]);
      }
      Flush(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, tris);
      break;
    }

    case PRIM_QUADS:
    {
      // Convert quads to triangle list: each group of 4 → 2 triangles (0-1-2, 0-2-3)
      std::vector<ImVertex> tris;
      tris.reserve((m_batch.size() / 4) * 6);
      for (size_t i = 0; i + 3 < m_batch.size(); i += 4)
      {
        tris.push_back(m_batch[i]);
        tris.push_back(m_batch[i + 1]);
        tris.push_back(m_batch[i + 2]);

        tris.push_back(m_batch[i]);
        tris.push_back(m_batch[i + 2]);
        tris.push_back(m_batch[i + 3]);
      }
      Flush(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, tris);
      break;
    }

    case PRIM_QUAD_STRIP:
    {
      // Quad strip: verts 0-1-3-2 form first quad, 2-3-5-4 form second, etc.
      std::vector<ImVertex> tris;
      tris.reserve(((m_batch.size() - 2) / 2) * 6);
      for (size_t i = 0; i + 3 < m_batch.size(); i += 2)
      {
        tris.push_back(m_batch[i]);
        tris.push_back(m_batch[i + 1]);
        tris.push_back(m_batch[i + 3]);

        tris.push_back(m_batch[i]);
        tris.push_back(m_batch[i + 3]);
        tris.push_back(m_batch[i + 2]);
      }
      Flush(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, tris);
      break;
    }

    case PRIM_LINES:
      Flush(D3D11_PRIMITIVE_TOPOLOGY_LINELIST, m_batch);
      break;

    case PRIM_LINE_STRIP:
      Flush(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, m_batch);
      break;

    case PRIM_LINE_LOOP:
    {
      // Line loop = line strip + closing segment
      std::vector<ImVertex> loop = m_batch;
      if (loop.size() >= 2)
        loop.push_back(loop[0]);
      Flush(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, loop);
      break;
    }

    case PRIM_POINTS:
      Flush(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, m_batch);
      break;
  }

  m_batch.clear();
}

// ---------------------------------------------------------------------------
// Flush: upload vertices and draw
// ---------------------------------------------------------------------------

void ImRenderer::Flush(D3D11_PRIMITIVE_TOPOLOGY topology, const std::vector<ImVertex>& verts)
{
  if (verts.empty())
    return;

  UINT vertCount = static_cast<UINT>(verts.size());
  if (vertCount > MAX_VERTICES)
    vertCount = MAX_VERTICES;

  // Upload vertex data
  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hr = m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr))
    return;
  memcpy(mapped.pData, verts.data(), vertCount * sizeof(ImVertex));
  m_context->Unmap(m_vertexBuffer, 0);

  // Upload WVP matrix
  DirectX::XMMATRIX wvp = m_worldMatrix * m_viewMatrix * m_projMatrix;
  wvp = DirectX::XMMatrixTranspose(wvp); // HLSL expects column-major

  hr = m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr))
    return;
  auto* cb = static_cast<CBPerDraw*>(mapped.pData);
  cb->worldViewProj = wvp;
  m_context->Unmap(m_constantBuffer, 0);

  // Bind pipeline state
  UINT stride = sizeof(ImVertex);
  UINT offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
  m_context->IASetInputLayout(m_inputLayout);
  m_context->IASetPrimitiveTopology(topology);

  m_context->VSSetShader(m_vsDefault, nullptr, 0);
  m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);

  if (m_boundSRV)
  {
    m_context->PSSetShader(m_psTextured, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &m_boundSRV);
    m_context->PSSetSamplers(0, 1, &m_defaultSampler);
  }
  else
  {
    m_context->PSSetShader(m_psColored, nullptr, 0);
  }

  m_context->Draw(vertCount, 0);

  // Unbind SRV to avoid resource hazards
  if (m_boundSRV)
  {
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullSRV);
  }
}
