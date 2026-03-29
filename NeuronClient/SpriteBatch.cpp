#include "pch.h"

#include "SpriteBatch.h"
#include "d3d12_backend.h"
#include "opengl_directx.h"
#include "PipelineState.h"
#include "UploadRingBuffer.h"

#include "CompiledShaders/SpriteVertexShader.h"
#include "CompiledShaders/SpritePixelShader.h"

using namespace DirectX;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

SpriteBatch& SpriteBatch::Get()
{
    static SpriteBatch s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void SpriteBatch::Init()
{
    CreatePSOs();
    m_vertices.reserve(6 * 256); // Pre-allocate for ~256 quads
    DebugTrace("SpriteBatch initialised\n");
}

void SpriteBatch::Shutdown()
{
    for (auto*& pso : m_psos)
    {
        if (pso)
        {
            pso->Destroy();
            delete pso;
            pso = nullptr;
        }
    }
    m_vertices.clear();
    m_vertices.shrink_to_fit();
    DebugTrace("SpriteBatch shut down\n");
}

// ---------------------------------------------------------------------------
// PSO creation — one per blend mode
// ---------------------------------------------------------------------------

void SpriteBatch::CreatePSOs()
{
    // --- Input Layout: float2 pos + uint32 color + float2 uv ---
    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_INPUT_LAYOUT_DESC inputLayout = {inputElements, _countof(inputElements)};

    // --- Rasterizer: No culling, solid fill ---
    D3D12_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    rasterDesc.FrontCounterClockwise = FALSE;
    rasterDesc.DepthClipEnable = TRUE;

    // --- Depth/Stencil: Disabled for 2D rendering ---
    D3D12_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    // --- Blend state presets ---
    struct BlendPreset
    {
        D3D12_BLEND srcBlend;
        D3D12_BLEND destBlend;
    };

    static constexpr BlendPreset presets[] = {
        // Alpha:    SrcAlpha / InvSrcAlpha
        { D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA },
        // Additive: SrcAlpha / One
        { D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE },
        // Shadow:   SrcAlpha / InvSrcColor
        { D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_COLOR },
    };
    static_assert(_countof(presets) == static_cast<size_t>(SpriteBlendMode::Count));

    for (size_t i = 0; i < static_cast<size_t>(SpriteBlendMode::Count); ++i)
    {
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = presets[i].srcBlend;
        blendDesc.RenderTarget[0].DestBlend = presets[i].destBlend;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        auto* pso = new GraphicsPSO();
        pso->SetRootSignature(OpenGLD3D::g_glState.GetRootSignature());
        pso->SetInputLayout(&inputLayout);
        pso->SetBlendState(blendDesc);
        pso->SetRasterizerState(rasterDesc);
        pso->SetDepthStencilState(depthDesc);
        pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        pso->SetRenderTargetFormat(Graphics::Core::Get().GetBackBufferFormat(),
                                   DXGI_FORMAT_UNKNOWN);
        pso->SetVertexShader(g_pSpriteVertexShader, sizeof(g_pSpriteVertexShader));
        pso->SetPixelShader(g_pSpritePixelShader, sizeof(g_pSpritePixelShader));
        pso->Finalize();

        m_psos[i] = pso;
    }
}

// ---------------------------------------------------------------------------
// Begin / End
// ---------------------------------------------------------------------------

void SpriteBatch::Begin(FXMMATRIX _orthoProjection)
{
    DEBUG_ASSERT(!m_begun);
    XMStoreFloat4x4(&m_projection, _orthoProjection);
    m_currentBlendMode = SpriteBlendMode::Alpha;
    m_currentTexture = OpenGLD3D::g_glState.GetDefaultTextureSRVGPUHandle();
    m_begun = true;
}

void SpriteBatch::End()
{
    DEBUG_ASSERT(m_begun);
    Flush();
    m_begun = false;
}

// ---------------------------------------------------------------------------
// State changes (auto-flush on change)
// ---------------------------------------------------------------------------

void SpriteBatch::SetBlendMode(SpriteBlendMode _mode)
{
    DEBUG_ASSERT(m_begun);
    if (_mode != m_currentBlendMode)
    {
        Flush();
        m_currentBlendMode = _mode;
    }
}

void SpriteBatch::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE _srvHandle)
{
    DEBUG_ASSERT(m_begun);
    // Treat zero handle as "use default white texture"
    if (_srvHandle.ptr == 0)
        _srvHandle = OpenGLD3D::g_glState.GetDefaultTextureSRVGPUHandle();

    if (_srvHandle.ptr != m_currentTexture.ptr)
    {
        Flush();
        m_currentTexture = _srvHandle;
    }
}

// ---------------------------------------------------------------------------
// Emit — decompose quad into 2 triangles
// ---------------------------------------------------------------------------

void SpriteBatch::Emit(
    const SpriteVertex& _v0, const SpriteVertex& _v1,
    const SpriteVertex& _v2, const SpriteVertex& _v3)
{
    DEBUG_ASSERT(m_begun);
    // Triangle 1: v0, v1, v2
    // Triangle 2: v0, v2, v3
    // Matches GL_QUADS winding convention.
    m_vertices.push_back(_v0);
    m_vertices.push_back(_v1);
    m_vertices.push_back(_v2);
    m_vertices.push_back(_v0);
    m_vertices.push_back(_v2);
    m_vertices.push_back(_v3);
}

void SpriteBatch::EmitTriangle(
    const SpriteVertex& _v0, const SpriteVertex& _v1,
    const SpriteVertex& _v2)
{
    DEBUG_ASSERT(m_begun);
    m_vertices.push_back(_v0);
    m_vertices.push_back(_v1);
    m_vertices.push_back(_v2);
}

// ---------------------------------------------------------------------------
// Flush — upload vertices + DrawConstants, issue one DrawInstanced
// ---------------------------------------------------------------------------

void SpriteBatch::Flush()
{
    if (m_vertices.empty())
        return;

    const UINT vertexCount = static_cast<UINT>(m_vertices.size());
    const UINT vbSize = vertexCount * sizeof(SpriteVertex);

    // --- Upload vertices to ring buffer ---
    auto vbAlloc = Graphics::GetCurrentUploadBuffer().Allocate(vbSize, sizeof(float));
    memcpy(vbAlloc.cpuPtr, m_vertices.data(), vbSize);

    D3D12_VERTEX_BUFFER_VIEW vbView{};
    vbView.BufferLocation = vbAlloc.gpuAddr;
    vbView.SizeInBytes = vbSize;
    vbView.StrideInBytes = sizeof(SpriteVertex);

    // --- Upload DrawConstants (mostly zeroed — only ProjectionMatrix + TexturingEnabled0) ---
    OpenGLD3D::DrawConstants dc = {};
    // WorldMatrix stays identity (zeroed is identity for pos already in screen space? No —
    // zeroed is NOT identity).  The sprite VS only uses ProjectionMatrix, but the root
    // signature binds b1 which the shader includes.  We use identity for WorldMatrix
    // since the VS multiplies pos by ProjectionMatrix only.
    XMStoreFloat4x4(&dc.WorldMatrix, XMMatrixIdentity());
    // Read projection from the GL stack at flush time so we always match
    // whatever coordinate system the caller has set up (pixel coords from
    // BeginText2D, or virtual coords from SetupRenderMatrices, etc.).
    dc.ProjectionMatrix = OpenGLD3D::GetProjectionStack().GetTop();
    dc.TexturingEnabled0 = 1;

    auto cbAlloc = Graphics::GetCurrentUploadBuffer().Allocate(sizeof(OpenGLD3D::DrawConstants), 256);
    memcpy(cbAlloc.cpuPtr, &dc, sizeof(dc));

    // --- Scene constants (for FadeAlpha) ---
    OpenGLD3D::EnsureSceneConstantsUploaded();

    // --- Issue draw ---
    auto* cmdList = Graphics::Core::Get().GetCommandList();

    const size_t blendIdx = static_cast<size_t>(m_currentBlendMode);
    cmdList->SetPipelineState(m_psos[blendIdx]->GetPipelineStateObject());

    // b0 = SceneConstants (for FadeAlpha)
    cmdList->SetGraphicsRootConstantBufferView(0, OpenGLD3D::GetSceneConstantsGPUAddr());
    // b1 = DrawConstants
    cmdList->SetGraphicsRootConstantBufferView(1, cbAlloc.gpuAddr);
    // t0 = texture SRV
    cmdList->SetGraphicsRootDescriptorTable(2, m_currentTexture);
    // Sampler table — index 0 = linear/wrap
    cmdList->SetGraphicsRootDescriptorTable(4, OpenGLD3D::g_glState.GetSamplerBaseGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    cmdList->DrawInstanced(vertexCount, 1, 0, 0);

    // --- Track stats ---
    OpenGLD3D::RecordBatchedDraw(vbSize);

    m_vertices.clear();
}
