#ifndef INCLUDED_IM_RENDERER_H
#define INCLUDED_IM_RENDERER_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <stack>

enum SamplerId;  // forward declare from texture_manager.h

enum PrimitiveType
{
  PRIM_POINTS,
  PRIM_LINES,
  PRIM_LINE_STRIP,
  PRIM_LINE_LOOP,
  PRIM_TRIANGLES,
  PRIM_TRIANGLE_STRIP,
  PRIM_TRIANGLE_FAN,
  PRIM_QUADS,
  PRIM_QUAD_STRIP
};

class ImRenderer
{
public:
  ImRenderer();
  ~ImRenderer();

  void Initialise(ID3D11Device* device, ID3D11DeviceContext* context);
  void Shutdown();

  // Immediate-mode emulation
  void Begin(PrimitiveType type);
  void End();

  void Vertex2f(float x, float y);
  void Vertex2fv(const float* v);
  void Vertex2i(int x, int y);
  void Vertex3f(float x, float y, float z);
  void Vertex3fv(const float* v);

  void Color3f(float r, float g, float b);
  void Color3ub(unsigned char r, unsigned char g, unsigned char b);
  void Color3ubv(const unsigned char* c);
  void Color4f(float r, float g, float b, float a);
  void Color4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void Color4ubv(const unsigned char* c);

  void TexCoord2f(float u, float v);
  void TexCoord2i(int u, int v);

  void Normal3f(float x, float y, float z);
  void Normal3fv(const float* n);

  // Matrix state — projection
  void SetProjectionMatrix(const DirectX::XMMATRIX& proj);
  const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projMatrix; }

  // Matrix state — modelview (replaces GL_MODELVIEW stack)
  void SetViewMatrix(const DirectX::XMMATRIX& view);
  const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
  void SetWorldMatrix(const DirectX::XMMATRIX& world);

  void PushMatrix();
  void PopMatrix();
  void LoadIdentity();
  void MultMatrixf(const float* m);   // column-major 4x4 (OpenGL layout)
  void Translatef(float x, float y, float z);
  void Scalef(float sx, float sy, float sz);
  const DirectX::XMMATRIX& GetWorldMatrix() const { return m_worldMatrix; }

  // Texture binding
  void BindTexture(ID3D11ShaderResourceView* srv);
  void BindTexture(int textureId);          // looks up from g_textureManager
  void UnbindTexture();

  // Sampler state
  void SetSampler(SamplerId id);            // selects sampler from g_textureManager

  // Lighting
  void SetLightingEnabled(bool enabled);
  void SetLightParams(int numLights,
                      const DirectX::XMFLOAT4 dirs[],
                      const DirectX::XMFLOAT4 colors[],
                      const DirectX::XMFLOAT4& ambient);

  // Fog
  void SetFogEnabled(bool enabled);
  void SetFogParams(float start, float end, float r, float g, float b);

  // Camera position (for fog distance calculation)
  void SetCameraPos(float x, float y, float z);

  // Upload constant buffer with current matrices and state (for external renderers)
  void UpdateConstantBuffer();

  // Accessors for direct rendering (landscape, water static VBs)
  ID3D11Buffer*        GetConstantBuffer()       { return m_constantBuffer; }
  ID3D11InputLayout*   GetInputLayout()          { return m_inputLayout; }
  ID3D11VertexShader*  GetVertexShader()          { return m_vsDefault; }
  ID3D11PixelShader*   GetColoredPixelShader()    { return m_psColored; }
  ID3D11PixelShader*   GetTexturedPixelShader()   { return m_psTextured; }

private:
  struct ImVertex
  {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT2 texcoord;
    DirectX::XMFLOAT3 normal;
  };

  struct CBPerDraw
  {
    DirectX::XMMATRIX worldViewProj;        // 64
    DirectX::XMMATRIX world;                // 64
    DirectX::XMFLOAT4 lightDir[2];          // 32
    DirectX::XMFLOAT4 lightColor[2];        // 32
    DirectX::XMFLOAT4 ambientColor;         // 16
    DirectX::XMFLOAT4 fogColor;             // 16
    DirectX::XMFLOAT4 cameraPos;            // 16
    float fogStart;                          // 4
    float fogEnd;                            // 4
    float alphaClipThreshold;                // 4
    int numLights;                           // 4
    int lightingEnabled;                     // 4
    int fogEnabled;                          // 4
    float _pad[2];                           // 8
  };                                         // Total: 272

  void Flush(D3D11_PRIMITIVE_TOPOLOGY topology, const std::vector<ImVertex>& verts);

  static const int MAX_VERTICES = 65536;
  static const int MAX_MATRIX_STACK_DEPTH = 32;

  ID3D11Device*            m_device;
  ID3D11DeviceContext*     m_context;

  ID3D11Buffer*            m_vertexBuffer;
  ID3D11Buffer*            m_constantBuffer;

  ID3D11VertexShader*      m_vsDefault;
  ID3D11PixelShader*       m_psColored;
  ID3D11PixelShader*       m_psTextured;
  ID3D11InputLayout*       m_inputLayout;
  ID3D11SamplerState*      m_defaultSampler;

  std::vector<ImVertex>    m_batch;
  PrimitiveType            m_currentPrimitive;
  bool                     m_inBeginEnd;

  // Current vertex attributes (set before each Vertex call)
  DirectX::XMFLOAT4       m_currentColor;
  DirectX::XMFLOAT2       m_currentTexCoord;
  DirectX::XMFLOAT3       m_currentNormal;

  // Matrices
  DirectX::XMMATRIX       m_projMatrix;
  DirectX::XMMATRIX       m_viewMatrix;
  DirectX::XMMATRIX       m_worldMatrix;

  // Modelview matrix stack (mirrors OpenGL's GL_MODELVIEW stack)
  std::stack<DirectX::XMMATRIX> m_matrixStack;

  // Bound texture
  ID3D11ShaderResourceView* m_boundSRV;

  // Lighting state
  bool                     m_lightingEnabled;
  int                      m_numLights;
  DirectX::XMFLOAT4        m_lightDir[2];
  DirectX::XMFLOAT4        m_lightColor[2];
  DirectX::XMFLOAT4        m_ambientColor;

  // Fog state
  bool                     m_fogEnabled;
  float                    m_fogStart;
  float                    m_fogEnd;
  DirectX::XMFLOAT4        m_fogColor;

  // Alpha clip
  float                    m_alphaClipThreshold;

  // Camera position
  DirectX::XMFLOAT3        m_cameraPos;
};

extern ImRenderer* g_imRenderer;

#endif
