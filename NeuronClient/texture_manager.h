#ifndef INCLUDED_TEXTURE_MANAGER_H
#define INCLUDED_TEXTURE_MANAGER_H

#include <d3d11.h>
#include <vector>

// Sampler state identifiers
enum SamplerId
{
  SAMPLER_LINEAR_CLAMP,
  SAMPLER_LINEAR_WRAP,
  SAMPLER_NEAREST_CLAMP,
  SAMPLER_NEAREST_WRAP,
  SAMPLER_COUNT
};

class TextureManager
{
public:
  TextureManager();
  ~TextureManager();

  void Initialise(ID3D11Device* device, ID3D11DeviceContext* context);
  void Shutdown();

  // Create a D3D11 texture from RGBA pixel data.
  // Returns an integer handle compatible with the existing Resource system.
  int CreateTexture(const void* pixels, int width, int height, bool mipmapping);

  // Create a D3D11 texture stored at a specific ID (used during transition
  // when OpenGL assigns the texture ID and we mirror it in the D3D11 registry).
  void CreateTextureAt(int id, const void* pixels, int width, int height, bool mipmapping);

  // Release a previously created texture.
  void DestroyTexture(int id);

  // Release all textures.
  void DestroyAll();

  // Look up the SRV for a given texture handle.
  ID3D11ShaderResourceView* GetSRV(int id) const;

  // Look up a sampler state.
  ID3D11SamplerState* GetSampler(SamplerId id) const { return m_samplers[id]; }

private:
  struct TextureEntry
  {
    ID3D11Texture2D*          texture;
    ID3D11ShaderResourceView* srv;
  };

  ID3D11Device*            m_device;
  ID3D11DeviceContext*     m_context;

  std::vector<TextureEntry> m_entries;    // index 0 is unused (id 0 = invalid)
  std::vector<int>          m_freeList;

  ID3D11SamplerState*      m_samplers[SAMPLER_COUNT];
};

extern TextureManager* g_textureManager;

#endif
