#pragma once

// Shader constant buffer layouts — must match HLSL exactly.
// Shared by the uber-shader pipeline and any native D3D12 renderer
// that uses the same compiled shaders.

namespace OpenGLD3D
{
  struct LightConstants
  {
    XMFLOAT4 Position;
    XMFLOAT4 Direction;
    XMFLOAT4 Diffuse;
    XMFLOAT4 Specular;
    XMFLOAT4 Ambient;
    UINT Enabled;
    float _pad[3];
  };

  // Per-frame scene constants (b0) — uploaded once per frame.
  struct alignas(256) SceneConstants
  {
    XMFLOAT4 GlobalAmbient;
    XMFLOAT4 FogColor;
    float    FogStart;
    float    FogEnd;
    float    FogDensity;
    UINT     FogMode;
    float    FadeAlpha;       // 0 = fully visible, 1 = fully black
    UINT     _pad[3];
  };

  // Per-draw constants (b1) — uploaded per draw call.
  struct alignas(256) DrawConstants
  {
    XMFLOAT4X4 WorldMatrix;
    XMFLOAT4X4 ProjectionMatrix;

    LightConstants Lights[8];

    XMFLOAT4 MatAmbient;
    XMFLOAT4 MatDiffuse;
    XMFLOAT4 MatSpecular;
    XMFLOAT4 MatEmissive;
    float    MatShininess;
    float    _matPad[3];

    UINT LightingEnabled;
    UINT FogEnabled;
    UINT TexturingEnabled0;
    UINT TexturingEnabled1;

    UINT TexEnvMode0;
    UINT TexEnvMode1;

    UINT ColorMaterialEnabled;
    UINT ColorMaterialMode;

    UINT  AlphaTestEnabled;
    UINT  AlphaTestFunc;
    float AlphaTestRef;

    UINT  NormalizeNormals;
    UINT  FlatShading;
    float PointSize;
    float _drawPad;
  };
} // namespace OpenGLD3D
