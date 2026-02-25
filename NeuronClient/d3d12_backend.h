#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <unordered_map>
#include <vector>
#include <array>

namespace OpenGLD3D {

    // --- Constants ---
    constexpr UINT FRAME_COUNT = 2;
    constexpr UINT UPLOAD_BUFFER_SIZE = 16 * 1024 * 1024; // 16 MB ring buffer
    constexpr UINT CBV_SIZE_ALIGNED = (sizeof(float) * 512 + 255) & ~255; // Aligned constant buffer

    // --- Per-frame constant buffer layout (must match HLSL exactly) ---

    struct LightConstants {
        DirectX::XMFLOAT4 Position;
        DirectX::XMFLOAT4 Direction;
        DirectX::XMFLOAT4 Diffuse;
        DirectX::XMFLOAT4 Specular;
        DirectX::XMFLOAT4 Ambient;
        UINT Enabled;
        float _pad[3];
    };

    struct alignas(256) PerFrameConstants {
        DirectX::XMFLOAT4X4 WorldMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;

        LightConstants Lights[8];

        DirectX::XMFLOAT4 MatAmbient;
        DirectX::XMFLOAT4 MatDiffuse;
        DirectX::XMFLOAT4 MatSpecular;
        DirectX::XMFLOAT4 MatEmissive;
        float MatShininess;
        float _matPad[3];

        DirectX::XMFLOAT4 GlobalAmbient;
        UINT LightingEnabled;
        UINT FogEnabled;
        UINT TexturingEnabled0;
        UINT TexturingEnabled1;

        DirectX::XMFLOAT4 FogColor;
        float FogStart;
        float FogEnd;
        float FogDensity;
        UINT FogMode;

        UINT TexEnvMode0;
        UINT TexEnvMode1;

        UINT ColorMaterialEnabled;
        UINT ColorMaterialMode;

        UINT AlphaTestEnabled;
        UINT AlphaTestFunc;
        float AlphaTestRef;

        UINT NormalizeNormals;
        UINT FlatShading;
        float PointSize;
        float _miscPad;
    };

    // --- PSO key for caching pipeline states ---

    struct PSOKey {
        // Rasterizer
        UINT8 cullMode;       // 0=NONE, 1=FRONT, 2=BACK
        UINT8 fillMode;       // 0=SOLID, 1=WIREFRAME
        // Depth
        UINT8 depthEnable;
        UINT8 depthWriteEnable;
        UINT8 depthFunc;      // maps to D3D12_COMPARISON_FUNC
        // Blend
        UINT8 blendEnable;
        UINT8 srcBlend;
        UINT8 dstBlend;
        // Topology type for PSO
        UINT8 topologyType;   // 0=POINT, 1=LINE, 2=TRIANGLE
        UINT8 _pad[3];

        bool operator==(const PSOKey& o) const { return memcmp(this, &o, sizeof(PSOKey)) == 0; }
    };

    struct PSOKeyHash {
        size_t operator()(const PSOKey& k) const {
            // Simple FNV-1a over the struct bytes
            size_t hash = 14695981039346656037ULL;
            const auto* p = reinterpret_cast<const uint8_t*>(&k);
            for (size_t i = 0; i < sizeof(PSOKey); i++) {
                hash ^= p[i];
                hash *= 1099511628211ULL;
            }
            return hash;
        }
    };

    // --- Upload ring buffer ---

    class UploadRingBuffer {
    public:
        void Init(ID3D12Device* device, SIZE_T size);
        void Shutdown();
        void Reset();

        // Allocate space and return CPU pointer + GPU virtual address
        struct Allocation {
            void* cpuPtr;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddr;
            SIZE_T offset;
        };
        Allocation Allocate(SIZE_T sizeBytes, SIZE_T alignment = 4);

        ID3D12Resource* GetResource() const { return m_resource.get(); }
        SIZE_T GetOffset() const { return m_offset; }

    private:
        com_ptr<ID3D12Resource> m_resource;
        UINT8* m_cpuBase = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS m_gpuBase = 0;
        SIZE_T m_size = 0;
        SIZE_T m_offset = 0;
    };

    // --- D3D12 Backend ---
    // After Phase 1: device, swap chain, command queue, command list, fence,
    // command allocators, render targets, and depth stencil are all owned by
    // Neuron::Graphics::Core.  This class retains descriptor heaps, root
    // signature, PSO cache, upload ring buffers, samplers, and default texture.

    class D3D12Backend {
    public:
        bool Init();
        void Shutdown();
        void BeginFrame();
        void EndFrame(); // Transition, present via Core, prepare next frame
        void WaitForGpu();

        // Accessors — device / command list now delegate to Core
        ID3D12Device* GetDevice() const;
        ID3D12GraphicsCommandList* GetCommandList() const;
        ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.get(); }
        UploadRingBuffer& GetUploadBuffer() { return m_uploadBuffers[Neuron::Graphics::Core::GetCurrentFrameIndex()]; }

        // PSO management
        ID3D12PipelineState* GetOrCreatePSO(const PSOKey& key);

        // Descriptor heap management
        ID3D12DescriptorHeap* GetSRVCBVHeap() const { return m_srvCbvHeap.get(); }
        ID3D12DescriptorHeap* GetSamplerHeap() const { return m_samplerHeap.get(); }
        UINT GetSRVDescriptorSize() const { return m_srvDescriptorSize; }
        UINT GetSamplerDescriptorSize() const { return m_samplerDescriptorSize; }

        // Allocate SRV/CBV slots
        D3D12_CPU_DESCRIPTOR_HANDLE AllocateSRVSlot(UINT& outIndex);
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(UINT index) const;

        // Pre-built sampler descriptors: [0]=linear/wrap, [1]=point/wrap, [2]=linear/clamp, [3]=point/clamp
        // + mip variants: [4]=linear/wrap/mipLinear, ...
        static constexpr UINT NUM_STATIC_SAMPLERS = 8;

        // Default (white) texture SRV index
        UINT GetDefaultTextureSRVIndex() const { return m_defaultTextureSRVIndex; }

        // RTV/DSV handle access (for glClear) — delegate to Core
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDSVHandle() const;

    private:
        void CreateRootSignature();
        void CreateDescriptorHeaps();
        void CreateUploadBuffer();
        void CreateDefaultTexture();
        void CreateSamplers();

        // SRV/CBV descriptor heap (shader-visible)
        com_ptr<ID3D12DescriptorHeap> m_srvCbvHeap;
        UINT m_srvDescriptorSize = 0;
        UINT m_srvNextFreeSlot = 0;
        static constexpr UINT MAX_SRV_DESCRIPTORS = 1024;

        // Sampler descriptor heap (shader-visible)
        com_ptr<ID3D12DescriptorHeap> m_samplerHeap;
        UINT m_samplerDescriptorSize = 0;

        // Root signature
        com_ptr<ID3D12RootSignature> m_rootSignature;

        // PSO cache
        std::unordered_map<PSOKey, com_ptr<ID3D12PipelineState>, PSOKeyHash> m_psoCache;

        // Upload ring buffers (one per frame to avoid GPU data races)
        UploadRingBuffer m_uploadBuffers[FRAME_COUNT];

        // Default 1x1 white texture
        com_ptr<ID3D12Resource> m_defaultTexture;
        UINT m_defaultTextureSRVIndex = 0;
    };

    // Global backend instance
    extern D3D12Backend g_backend;

} // namespace OpenGLD3D
