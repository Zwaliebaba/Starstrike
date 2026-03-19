#pragma once

#include "RootSignature.h"
#include "FrameListener.h"

namespace OpenGLD3D
{
  // --- Constant buffer layouts (must match HLSL exactly) ---

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

  // --- Per-frame scene constants (b0) — uploaded once per frame ---
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

  // --- Per-draw constants (b1) — uploaded per draw call ---
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

  // --- PSO key for caching pipeline states ---

  struct PSOKey
  {
    // Rasterizer
    UINT8 cullMode; // 0=NONE, 1=FRONT, 2=BACK
    UINT8 fillMode; // 0=SOLID, 1=WIREFRAME
    // Depth
    UINT8 depthEnable;
    UINT8 depthWriteEnable;
    UINT8 depthFunc; // maps to D3D12_COMPARISON_FUNC
    // Blend
    UINT8 blendEnable;
    UINT8 srcBlend;
    UINT8 dstBlend;
    // Topology type for PSO
    UINT8 topologyType; // 0=POINT, 1=LINE, 2=TRIANGLE
    UINT8 _pad[3];

    bool operator==(const PSOKey& o) const { return memcmp(this, &o, sizeof(PSOKey)) == 0; }
  };

  struct PSOKeyHash
  {
    size_t operator()(const PSOKey& k) const
    {
      // Simple FNV-1a over the struct bytes
      size_t hash = 14695981039346656037ULL;
      const auto* p = reinterpret_cast<const uint8_t*>(&k);
      for (size_t i = 0; i < sizeof(PSOKey); i++)
      {
        hash ^= p[i];
        hash *= 1099511628211ULL;
      }
      return hash;
    }
  };

  // --- OpenGL Translation State ---
  // Owns the GL→D3D12 translation concerns: root signature, PSO cache,
  // default white texture, static samplers.  Engine infrastructure (device,
  // command list, descriptor heaps, upload buffers) lives in Graphics::Core
  // and DescriptorAllocator.

  class OpenGLTranslationState : public Neuron::Graphics::IFrameListener
  {
    public:
      bool Init();
      void Shutdown();

      // IFrameListener — sets descriptor heaps, root signature, and render targets.
      void OnFrameBegin(ID3D12GraphicsCommandList* cmdList) override;

      // Accessors
      ID3D12RootSignature* GetRawRootSignature() const { return m_rootSignature.GetSignature(); }
      const RootSignature& GetRootSignature() const { return m_rootSignature; }

      // PSO management
      ID3D12PipelineState* GetOrCreatePSO(const PSOKey& key);

      // Allocate an SRV slot from the unified DescriptorAllocator heap.
      Graphics::DescriptorHandle AllocateSRVSlot();

      // Pre-built sampler descriptors: [0]=linear/wrap, [1]=point/wrap, [2]=linear/clamp, [3]=point/clamp
      // + mip variants: [4]=linear/wrap/mipLinear, ...
      static constexpr UINT NUM_STATIC_SAMPLERS = 8;

      // Default (white) texture GPU handle
      D3D12_GPU_DESCRIPTOR_HANDLE GetDefaultTextureSRVGPUHandle() const { return m_defaultTextureSRVHandle; }

      // Sampler base GPU handle (index 0 of the contiguous sampler block)
      D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerBaseGPUHandle() const { return m_samplerBaseHandle; }

    private:
      void CreateRootSignature();
      void CreateDefaultTexture();
      void CreateSamplers();

      // Root signature
      RootSignature m_rootSignature;

      // PSO cache
      std::unordered_map<PSOKey, com_ptr<ID3D12PipelineState>, PSOKeyHash> m_psoCache;

      // Default 1x1 white texture
      com_ptr<ID3D12Resource> m_defaultTexture;
      Graphics::DescriptorHandle m_defaultTextureSRVHandle;

      // Sampler base handle (contiguous block of NUM_STATIC_SAMPLERS)
      Graphics::DescriptorHandle m_samplerBaseHandle;
  };

  // Global OpenGL translation state instance
  extern OpenGLTranslationState g_glState;
} // namespace OpenGLD3D
