#pragma once

#include "RootSignature.h"
#include "FrameListener.h"
#include "ShaderConstants.h"

namespace OpenGLD3D
{
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
