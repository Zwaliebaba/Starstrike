#ifndef INCLUDED_SPRITE_BATCH_H
#define INCLUDED_SPRITE_BATCH_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

enum BlendStateId;   // forward declare from render_states.h

struct SpriteVertex
{
  DirectX::XMFLOAT3 pos;       // 12 bytes
  DirectX::XMFLOAT2 uv;        //  8 bytes
  DirectX::XMFLOAT4 color;     // 16 bytes
};                               // 36 bytes total

class SpriteBatch
{
public:
  SpriteBatch();
  ~SpriteBatch();

  void Initialise(ID3D11Device* device, ID3D11DeviceContext* context);
  void Shutdown();

  // --- 2D helpers (set up ortho projection internally) ---
  void Begin2D(int screenW, int screenH, BlendStateId blend);
  void End2D();

  // --- 3D helpers (caller provides view*proj) ---
  void Begin3D(const DirectX::XMMATRIX& viewProj, BlendStateId blend);
  void End3D();

  // --- Primitive operations (valid between Begin/End) ---
  void SetTexture(int textureId);
  void SetTexture(ID3D11ShaderResourceView* srv);
  void SetBlendState(BlendStateId blend);
  void SetColor(float r, float g, float b, float a = 1.0f);

  // Add a 2D textured quad (z = 0)
  void AddQuad2D(float x, float y, float w, float h,
                 float u0, float v0, float u1, float v1);

  // Add a 3D billboard quad (4 corners, CCW winding)
  void AddQuad3D(const DirectX::XMFLOAT3& topLeft,
                 const DirectX::XMFLOAT3& topRight,
                 const DirectX::XMFLOAT3& bottomRight,
                 const DirectX::XMFLOAT3& bottomLeft,
                 float u0, float v0, float u1, float v1);

private:
  struct CBSprite
  {
    DirectX::XMMATRIX viewProj;   // 64 bytes
  };

  void Flush();

  static const int MAX_QUADS    = 4096;
  static const int MAX_VERTICES = MAX_QUADS * 4;   // 16 384
  static const int MAX_INDICES  = MAX_QUADS * 6;   // 24 576

  ID3D11Device*            m_device;
  ID3D11DeviceContext*     m_context;

  ID3D11Buffer*            m_vertexBuffer;
  ID3D11Buffer*            m_indexBuffer;
  ID3D11Buffer*            m_constantBuffer;

  ID3D11VertexShader*      m_vertexShader;
  ID3D11PixelShader*       m_pixelShader;
  ID3D11InputLayout*       m_inputLayout;

  std::vector<SpriteVertex> m_batch;
  int                      m_quadCount;
  bool                     m_inBeginEnd;

  DirectX::XMMATRIX       m_viewProj;
  DirectX::XMFLOAT4       m_currentColor;

  ID3D11ShaderResourceView* m_boundSRV;
  BlendStateId              m_currentBlend;
};

extern SpriteBatch* g_spriteBatch;

#endif
