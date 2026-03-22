#pragma once

#include <DirectXMath.h>
#include <cmath>

// ---------------------------------------------------------------------------
// GameVector3 — pure storage type backed by XMFLOAT3.
//
// Stores three floats for persistent data (struct members, serialization).
// For math operations, load into XMVECTOR via Load() and use Neuron::Math
// free functions or native DirectXMath.  No arithmetic operators.
// ---------------------------------------------------------------------------

namespace Neuron::Math
{
  struct GameVector3
  {
    union
    {
      DirectX::XMFLOAT3 v;
      struct { float x, y, z; };
    };

    // --- Construction -------------------------------------------------------

    constexpr GameVector3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}

    constexpr GameVector3(float _x, float _y, float _z) noexcept
      : x(_x), y(_y), z(_z) {}

    constexpr GameVector3(const DirectX::XMFLOAT3& _v) noexcept : v(_v) {}

    GameVector3(DirectX::FXMVECTOR _v) noexcept
    {
      DirectX::XMStoreFloat3(&v, _v);
    }

    // --- Load / Store -------------------------------------------------------

    [[nodiscard]] DirectX::XMVECTOR Load() const noexcept
    {
      return DirectX::XMLoadFloat3(&v);
    }

    // --- Data access (OpenGL shim interop, serialization) -------------------

    [[nodiscard]] float* GetData() noexcept { return &x; }
    [[nodiscard]] const float* GetDataConst() const noexcept { return &x; }

    // --- Mutators (legacy compatibility) ------------------------------------

    void Zero() noexcept { x = y = z = 0.0f; }

    void Set(float _x, float _y, float _z) noexcept
    {
      x = _x;
      y = _y;
      z = _z;
    }

    // --- Comparison (per-component approximate, matches LegacyVector3) ------

    [[nodiscard]] bool operator==(const GameVector3& _b) const noexcept
    {
      return std::fabsf(x - _b.x) < 1e-6f
          && std::fabsf(y - _b.y) < 1e-6f
          && std::fabsf(z - _b.z) < 1e-6f;
    }

    [[nodiscard]] bool operator!=(const GameVector3& _b) const noexcept
    {
      return !(*this == _b);
    }
  };
}
