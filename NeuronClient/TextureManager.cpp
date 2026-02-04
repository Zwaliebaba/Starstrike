#include "pch.h"
#include "TextureManager.h"
#include "DDSTextureLoader.h"
#include "opengl_directx_internals.h"

color_t Bitmap::GetPixel(const float _x, const float _y) const
{
  DEBUG_ASSERT(m_data.size() != 0);
  const uint8_t* data = GetBitData();
  const int index = static_cast<int>(_y * static_cast<float>(header->width) + _x) * 4;
  return Color::FromRGBA(data[index], data[index + 1], data[index + 2], data[index + 3]);
}

void Bitmap::LoadFromFile(const std::wstring& _filename)
{
  m_data = BinaryFile::ReadFile(_filename);
  check_hresult(LoaderHelpers::LoadTextureDataFromMemory(m_data, &header, &bitData, &bitSize));
}

void TextureManager::Shutdown()
{
  sm_mutex.lock();
  sm_textures.clear();
  sm_mutex.unlock();
}

void TextureManager::LoadTexture(const std::wstring& _name, TextureRef* _texture, bool _keepData)
{
  if (const auto it = sm_textures.find(_name); it == sm_textures.end())
  {
    auto texture = NEW Texture();
    *_texture = texture;

    texture->StartLoading();

    texture->m_bitmap = std::make_unique<Bitmap>();
    texture->m_bitmap->LoadFromFile(_name);

    sm_mutex.lock();
    check_hresult(CreateDDSTextureFromBitmap(OpenGLD3D::g_pd3dDevice, texture->m_bitmap.get(), texture->m_texture.put()));
    texture->m_ref = glGenTextures(texture->m_texture.get());

    D3DSURFACE_DESC surfaceDesc;
    check_hresult(texture->m_texture->GetLevelDesc(0, &surfaceDesc));
    texture->m_width = static_cast<float>(surfaceDesc.Width);
    texture->m_height = static_cast<float>(surfaceDesc.Height);

    // Lock should not go around co_await
    sm_textures.try_emplace(_name, texture);
    sm_mutex.unlock();

    if (!_keepData)
      texture->m_bitmap = nullptr;

    texture->FinishLoading();
  }
  else
    *_texture = it->second.get();
}

void TextureManager::RemoveTexture(const Texture* _texture)
{
  for (auto it = sm_textures.begin(); it != sm_textures.end(); ++it)
  {
    if (it->second.get() == _texture && --it->second->m_referenceCount == 0)
    {
      sm_textures.erase(it);
      return;
    }
  }
}

TextureRef::TextureRef(const TextureRef& _ref)
  : m_ref(_ref.m_ref)
{
  if (m_ref != nullptr)
    ++m_ref->m_referenceCount;
}

TextureRef::TextureRef(Texture* _tex)
  : m_ref(_tex)
{
  if (m_ref != nullptr)
    ++m_ref->m_referenceCount;
}

TextureRef::~TextureRef()
{
  if (m_ref != nullptr)
    TextureManager::RemoveTexture(m_ref);
}

TextureRef& TextureRef::operator=(std::nullptr_t)
{
  if (m_ref != nullptr)
    --m_ref->m_referenceCount;

  m_ref = nullptr;
  return *this;
}

TextureRef& TextureRef::operator=(const TextureRef& _rhs)
{
  if (&_rhs == this)
    return *this;

  if (m_ref != nullptr)
    --m_ref->m_referenceCount;

  m_ref = _rhs.m_ref;

  if (m_ref != nullptr)
    ++m_ref->m_referenceCount;
  return *this;
}

TextureRef& TextureRef::operator=(Texture* _rhs)
{
  m_ref = _rhs;
  if (m_ref != nullptr)
    ++m_ref->m_referenceCount;
  return *this;
}

bool TextureRef::IsValid() const { return m_ref && m_ref->IsValid(); }

GLuint TextureRef::Get() const
{
  DEBUG_ASSERT(m_ref != nullptr);
  m_ref->WaitForLoad();
  return m_ref->m_ref;
}

const Texture* TextureRef::operator->() const
{
  DEBUG_ASSERT(m_ref != nullptr);
  m_ref->WaitForLoad();
  return m_ref;
}
