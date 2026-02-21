#include "pch.h"
#include "texture_manager.h"
#include "debug_utils.h"

TextureManager* g_textureManager = nullptr;

TextureManager::TextureManager()
  : m_device(nullptr),
    m_context(nullptr)
{
  ZeroMemory(m_samplers, sizeof(m_samplers));

  // Reserve index 0 as invalid (OpenGL texture ID 0 is "no texture")
  TextureEntry empty = { nullptr, nullptr };
  m_entries.push_back(empty);
}

TextureManager::~TextureManager()
{
  Shutdown();
}

// ---------------------------------------------------------------------------
// Helper: create a sampler state
// ---------------------------------------------------------------------------

static ID3D11SamplerState* CreateSampler(ID3D11Device* device,
  D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
{
  D3D11_SAMPLER_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.Filter         = filter;
  sd.AddressU        = addressMode;
  sd.AddressV        = addressMode;
  sd.AddressW        = addressMode;
  sd.ComparisonFunc  = D3D11_COMPARISON_NEVER;
  sd.MaxAnisotropy   = 1;
  sd.MinLOD          = 0;
  sd.MaxLOD          = D3D11_FLOAT32_MAX;

  ID3D11SamplerState* sampler = nullptr;
  HRESULT hr = device->CreateSamplerState(&sd, &sampler);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create sampler state");
  return sampler;
}

// ---------------------------------------------------------------------------
// Initialise / Shutdown
// ---------------------------------------------------------------------------

void TextureManager::Initialise(ID3D11Device* device, ID3D11DeviceContext* context)
{
  m_device  = device;
  m_context = context;

  m_samplers[SAMPLER_LINEAR_CLAMP]  = CreateSampler(device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
  m_samplers[SAMPLER_LINEAR_WRAP]   = CreateSampler(device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
  m_samplers[SAMPLER_NEAREST_CLAMP] = CreateSampler(device, D3D11_FILTER_MIN_MAG_MIP_POINT,  D3D11_TEXTURE_ADDRESS_CLAMP);
  m_samplers[SAMPLER_NEAREST_WRAP]  = CreateSampler(device, D3D11_FILTER_MIN_MAG_MIP_POINT,  D3D11_TEXTURE_ADDRESS_WRAP);

  DebugTrace("TextureManager initialised ({} samplers)\n", static_cast<int>(SAMPLER_COUNT));
}

void TextureManager::Shutdown()
{
  DestroyAll();

  for (int i = 0; i < SAMPLER_COUNT; ++i)
    SAFE_RELEASE(m_samplers[i]);

  m_device  = nullptr;
  m_context = nullptr;
}

// ---------------------------------------------------------------------------
// Texture creation / destruction
// ---------------------------------------------------------------------------

int TextureManager::CreateTexture(const void* pixels, int width, int height, bool mipmapping)
{
  DarwiniaReleaseAssert(m_device, "TextureManager not initialised");

  // --- Create texture ---
  D3D11_TEXTURE2D_DESC td;
  ZeroMemory(&td, sizeof(td));
  td.Width              = width;
  td.Height             = height;
  td.ArraySize          = 1;
  td.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count   = 1;
  td.Usage              = D3D11_USAGE_DEFAULT;
  td.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

  if (mipmapping)
  {
    td.MipLevels  = 0;  // full mip chain
    td.BindFlags |= D3D11_BIND_RENDER_TARGET;
    td.MiscFlags  = D3D11_RESOURCE_MISC_GENERATE_MIPS;
  }
  else
  {
    td.MipLevels = 1;
  }

  ID3D11Texture2D* tex = nullptr;
  HRESULT hr;

  if (mipmapping)
  {
    // Create empty texture, then upload mip 0 and generate the rest
    hr = m_device->CreateTexture2D(&td, nullptr, &tex);
    DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create D3D11 texture");

    m_context->UpdateSubresource(tex, 0, nullptr, pixels, width * 4, 0);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem          = pixels;
    initData.SysMemPitch      = width * 4;
    initData.SysMemSlicePitch = 0;
    hr = m_device->CreateTexture2D(&td, &initData, &tex);
    DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create D3D11 texture");
  }

  // --- Create SRV ---
  ID3D11ShaderResourceView* srv = nullptr;
  hr = m_device->CreateShaderResourceView(tex, nullptr, &srv);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SRV");

  if (mipmapping)
    m_context->GenerateMips(srv);

  // --- Allocate an ID ---
  int id;
  if (!m_freeList.empty())
  {
    id = m_freeList.back();
    m_freeList.pop_back();
    m_entries[id] = { tex, srv };
  }
  else
  {
    id = static_cast<int>(m_entries.size());
    m_entries.push_back({ tex, srv });
  }

  return id;
}

void TextureManager::CreateTextureAt(int id, const void* pixels, int width, int height, bool mipmapping)
{
  DarwiniaReleaseAssert(m_device, "TextureManager not initialised");
  DarwiniaReleaseAssert(id > 0, "Invalid texture ID for CreateTextureAt");

  // Grow the vector if needed to accommodate the given ID
  while (id >= static_cast<int>(m_entries.size()))
    m_entries.push_back({ nullptr, nullptr });

  // If a texture already exists at this slot, release it
  SAFE_RELEASE(m_entries[id].srv);
  SAFE_RELEASE(m_entries[id].texture);

  D3D11_TEXTURE2D_DESC td;
  ZeroMemory(&td, sizeof(td));
  td.Width              = width;
  td.Height             = height;
  td.ArraySize          = 1;
  td.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count   = 1;
  td.Usage              = D3D11_USAGE_DEFAULT;
  td.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

  if (mipmapping)
  {
    td.MipLevels  = 0;
    td.BindFlags |= D3D11_BIND_RENDER_TARGET;
    td.MiscFlags  = D3D11_RESOURCE_MISC_GENERATE_MIPS;
  }
  else
  {
    td.MipLevels = 1;
  }

  ID3D11Texture2D* tex = nullptr;
  HRESULT hr;

  if (mipmapping)
  {
    hr = m_device->CreateTexture2D(&td, nullptr, &tex);
    DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create D3D11 texture");
    m_context->UpdateSubresource(tex, 0, nullptr, pixels, width * 4, 0);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem          = pixels;
    initData.SysMemPitch      = width * 4;
    initData.SysMemSlicePitch = 0;
    hr = m_device->CreateTexture2D(&td, &initData, &tex);
    DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create D3D11 texture");
  }

  ID3D11ShaderResourceView* srv = nullptr;
  hr = m_device->CreateShaderResourceView(tex, nullptr, &srv);
  DarwiniaReleaseAssert(SUCCEEDED(hr), "Failed to create SRV");

  if (mipmapping)
    m_context->GenerateMips(srv);

  m_entries[id] = { tex, srv };
}

void TextureManager::DestroyTexture(int id)
{
  if (id <= 0 || id >= static_cast<int>(m_entries.size()))
    return;

  TextureEntry& entry = m_entries[id];
  SAFE_RELEASE(entry.srv);
  SAFE_RELEASE(entry.texture);
  entry = { nullptr, nullptr };

  m_freeList.push_back(id);
}

void TextureManager::DestroyAll()
{
  for (int i = 1; i < static_cast<int>(m_entries.size()); ++i)
  {
    SAFE_RELEASE(m_entries[i].srv);
    SAFE_RELEASE(m_entries[i].texture);
  }
  m_entries.resize(1);   // keep index 0
  m_freeList.clear();
}

ID3D11ShaderResourceView* TextureManager::GetSRV(int id) const
{
  if (id <= 0 || id >= static_cast<int>(m_entries.size()))
    return nullptr;

  return m_entries[id].srv;
}
