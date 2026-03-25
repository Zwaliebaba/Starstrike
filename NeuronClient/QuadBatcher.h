#pragma once

#include "opengl_directx_internals.h"

// ---------------------------------------------------------------------------
// QuadBatcher — accumulates quads and flushes as a single draw call.
//
// Phase 1 of the DX12 migration.  Replaces per-quad glBegin/glEnd calls with
// batched rendering.  Callers still manage GL state (blend, texture, depth);
// the batcher only accumulates vertices and issues one DrawInstanced per Flush.
//
// Usage:
//   // ... set GL state (texture, blend, etc.) ...
//   auto& b = QuadBatcher::Get();
//   b.Emit(v0, v1, v2, v3);   // quad -> 2 triangles (6 vertices)
//   b.Emit(v0, v1, v2, v3);   // another quad
//   b.Flush();                 // single draw call for all accumulated quads
//   // ... change GL state if needed ...
//   b.Emit(...);
//   b.Flush();
//
// Constraint: all quads between Flush() calls must share the same GL state
// (blend mode, bound texture, depth write, etc.) because Flush() uses
// PrepareDrawState() which reads the current GL state.
// ---------------------------------------------------------------------------

class QuadBatcher
{
  public:
    [[nodiscard]] static QuadBatcher& Get();

    // Emit a quad from 4 pre-computed vertices.
    // Decomposes into 2 triangles: (v0,v1,v2) and (v0,v2,v3).
    // Matches the GL_QUADS winding convention used by the GL emulation layer.
    void Emit(const OpenGLD3D::CustomVertex& _v0, const OpenGLD3D::CustomVertex& _v1,
              const OpenGLD3D::CustomVertex& _v2, const OpenGLD3D::CustomVertex& _v3);

    // Flush accumulated quads as a single draw call.
    // Uses PrepareDrawState() for PSO/CB/texture — GL state must be set before calling.
    // No-op if no quads accumulated.
    void Flush();

    // Number of quads accumulated since last Flush.
    [[nodiscard]] unsigned int GetCount() const noexcept
    {
        return static_cast<unsigned int>(m_vertices.size()) / 6;
    }

    // --- Vertex construction helpers ---

    // Pack RGBA bytes into CustomVertex.diffuse (BGRA byte order).
    [[nodiscard]] static constexpr uint32_t PackColorBGRA(
        unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a = 255) noexcept
    {
        return static_cast<uint32_t>(_b)
             | (static_cast<uint32_t>(_g) << 8)
             | (static_cast<uint32_t>(_r) << 16)
             | (static_cast<uint32_t>(_a) << 24);
    }

    // Pack float RGBA [0..1] into CustomVertex.diffuse (BGRA byte order).
    [[nodiscard]] static uint32_t PackColorF(
        float _r, float _g, float _b, float _a = 1.0f) noexcept
    {
        auto toByte = [](float x) -> unsigned char {
            return x < 0.f ? 0 : (x > 1.f ? 255 : static_cast<unsigned char>(x * 255.f));
        };
        return PackColorBGRA(toByte(_r), toByte(_g), toByte(_b), toByte(_a));
    }

    // Build a CustomVertex with position, color, and UVs.  Normal is zeroed.
    [[nodiscard]] static OpenGLD3D::CustomVertex MakeVertex(
        float _x, float _y, float _z,
        uint32_t _colorBGRA,
        float _u = 0.f, float _v = 0.f) noexcept
    {
        OpenGLD3D::CustomVertex vtx{};
        vtx.x = _x;  vtx.y = _y;  vtx.z = _z;
        // nx, ny, nz zeroed — no lighting for billboards/quads
        vtx.diffuse = _colorBGRA;
        vtx.u = _u;  vtx.v = _v;
        // u2, v2 zeroed
        return vtx;
    }

  private:
    QuadBatcher() { m_vertices.reserve(6 * 256); }
    ~QuadBatcher() = default;

    std::vector<OpenGLD3D::CustomVertex> m_vertices;
};
