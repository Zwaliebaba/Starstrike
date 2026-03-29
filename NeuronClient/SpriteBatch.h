#pragma once

// ---------------------------------------------------------------------------
// SpriteBatch — 2D sprite/UI quad batcher with dedicated PSOs (Phase 3).
//
// Replaces per-quad glBegin/glEnd calls for 2D rendering (UI, text, cursors,
// HUD) with batched draws using lightweight sprite-specific shaders and a
// compact 20-byte vertex format.
//
// Unlike QuadBatcher (Phase 1), SpriteBatch owns its own PSOs and does NOT
// use PrepareDrawState() or the GL emulation layer for pipeline setup.
//
// Usage:
//   auto& sb = SpriteBatch::Get();
//   sb.Begin(orthoMatrix);                // set projection, default blend
//   sb.SetTexture(srvHandle);             // bind texture SRV
//   sb.SetBlendMode(BlendMode::Alpha);    // optional — Alpha is default
//   sb.Emit(v0, v1, v2, v3);             // quad -> 2 triangles (6 vertices)
//   sb.Flush();                           // single draw call
//   sb.End();                             // optional, calls Flush()
//
// Constraints:
//   - All quads between Flush() calls share the same texture and blend mode.
//   - Call Flush() before changing texture or blend mode.
//   - Begin() must be called once per frame/pass to set the ortho projection.
// ---------------------------------------------------------------------------

#include <DirectXMath.h>

namespace OpenGLD3D { struct DrawConstants; }

// ---------------------------------------------------------------------------
// SpriteVertex — compact 2D vertex (20 bytes)
// ---------------------------------------------------------------------------

struct SpriteVertex
{
    float    x, y;       // Screen-space position
    uint32_t color;      // BGRA packed (R8G8B8A8_UNORM in shader)
    float    u, v;       // Texture coordinates
};
static_assert(sizeof(SpriteVertex) == 20, "SpriteVertex must be 20 bytes");

// ---------------------------------------------------------------------------
// Blend mode presets — each maps to a pre-built PSO
// ---------------------------------------------------------------------------

enum class SpriteBlendMode : uint8_t
{
    Alpha,       // SrcAlpha / InvSrcAlpha — standard UI/text
    Additive,    // SrcAlpha / One         — glow effects
    Shadow,      // SrcAlpha / InvSrcColor — shadow overlays
    Count
};

// ---------------------------------------------------------------------------
// SpriteBatch
// ---------------------------------------------------------------------------

class GraphicsPSO;

class SpriteBatch
{
  public:
    [[nodiscard]] static SpriteBatch& Get();

    // Lifecycle — call Init() after D3D12 device + root signature are ready.
    void Init();
    void Shutdown();

    // --- Per-pass setup ---

    // Begin a 2D pass with the given orthographic projection matrix.
    // Sets blend mode to Alpha by default.
    void Begin(DirectX::FXMMATRIX _orthoProjection);

    // End the current 2D pass (flushes any remaining quads).
    void End();

    // --- State ---

    // Set the blend mode.  Flushes accumulated quads if the mode changes.
    void SetBlendMode(SpriteBlendMode _mode);

    // Set the active texture SRV.  Flushes accumulated quads if the texture changes.
    // Pass a zero-initialized handle (ptr==0) to use the default white texture.
    void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE _srvHandle);

    // --- Quad emission ---

    // Emit a quad from 4 pre-computed SpriteVertex values.
    // Decomposes into 2 triangles: (v0,v1,v2) and (v0,v2,v3).
    void Emit(const SpriteVertex& _v0, const SpriteVertex& _v1,
              const SpriteVertex& _v2, const SpriteVertex& _v3);

    // Emit a single triangle from 3 pre-computed SpriteVertex values.
    void EmitTriangle(const SpriteVertex& _v0, const SpriteVertex& _v1,
                      const SpriteVertex& _v2);

    // --- Flush ---

    // Upload and draw all accumulated quads.  No-op if empty.
    void Flush();

    // --- Query ---

    // True if Begin() has been called and End() has not yet been called.
    [[nodiscard]] bool IsActive() const noexcept { return m_begun; }

    // --- Vertex construction helpers ---

    [[nodiscard]] static constexpr uint32_t PackColorBGRA(
        unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255) noexcept
    {
        return static_cast<uint32_t>(_b)
             | (static_cast<uint32_t>(_g) << 8)
             | (static_cast<uint32_t>(_r) << 16)
             | (static_cast<uint32_t>(_a) << 24);
    }

    [[nodiscard]] static uint32_t PackColorF(
        float _r, float _g, float _b, float _a = 1.0f) noexcept
    {
        auto toByte = [](float val) -> unsigned char {
            return val < 0.f ? 0 : (val > 1.f ? 255 : static_cast<unsigned char>(val * 255.f));
        };
        return PackColorBGRA(toByte(_r), toByte(_g), toByte(_b), toByte(_a));
    }

    [[nodiscard]] static SpriteVertex MakeVertex(
        float _x, float _y, uint32_t _colorBGRA, float _u = 0.f, float _v = 0.f) noexcept
    {
        return { _x, _y, _colorBGRA, _u, _v };
    }

    // Number of quads accumulated since last Flush.
    [[nodiscard]] unsigned int GetCount() const noexcept
    {
        return static_cast<unsigned int>(m_vertices.size()) / 6;
    }

  private:
    SpriteBatch() = default;

    void CreatePSOs();

    // Pre-built PSOs — one per blend mode
    GraphicsPSO* m_psos[static_cast<size_t>(SpriteBlendMode::Count)] = {};

    // Current state
    SpriteBlendMode                m_currentBlendMode = SpriteBlendMode::Alpha;
    D3D12_GPU_DESCRIPTOR_HANDLE    m_currentTexture   = {};
    DirectX::XMFLOAT4X4           m_projection        = {};
    bool                           m_begun             = false;

    // Accumulated vertices (6 per quad: 2 triangles)
    std::vector<SpriteVertex>      m_vertices;
};
